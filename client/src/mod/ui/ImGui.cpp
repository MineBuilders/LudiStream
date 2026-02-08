#include "ImGui.h"

#include "RenderLoop.h"
#include "ll/api/memory/Hook.h"
#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/Font.h"
#include "mc/client/gui/GuiData.h"
#include "mc/client/gui/controls/UIRenderContext.h"
#include "mc/client/gui/screens/ScreenContext.h"
#include "mc/client/gui/screens/ScreenView.h"
#include "mc/client/renderer/Tessellator.h"
#include "mc/client/renderer/TextureGroup.h"
#include "mc/common/Globals.h"
#include "mc/deps/core/container/Blob.h"
#include "mc/deps/core/file/PathView.h"
#include "mc/deps/core_graphics/ImageDescription.h"
#include "mc/deps/input/MouseDevice.h"
#include "mc/deps/minecraft_renderer/renderer/TexturePtr.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "mc/deps/minecraft_renderer/resources/ClientTexture.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "mc/deps/minecraft_renderer/resources/ServerTexture.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "mc/deps/minecraft_renderer/resources/UIActorOffscreenCaptureDescription.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "mc/deps/minecraft_renderer/resources/UIMeshOffscreenCaptureDescription.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "mc/deps/minecraft_renderer/resources/UIStructureVolumeOffscreenCaptureDescription.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "mc/deps/minecraft_renderer/resources/UIThumbnailMeshOffscreenCaptureDescription.h"
#include <imgui.h>
#include <windows.h>

namespace ludistream::ui {

static mce::TextureGroup* textureGroup;
static mce::TexturePtr    texturePtr;
static bool               hasUnloadedTextures = false;
static Tessellator*       tessellator;

static void loadFontTexture(const UIRenderContext& context) {
    const auto&    io = ImGui::GetIO();
    unsigned char* pixels;
    int            width, height, bytesPerPixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);
    mce::Blob blob(pixels, width * height * bytesPerPixel);
    // TODO: use MCAPI
    // cg::ImageDescription description(
    //     width,
    //     height,
    //     mce::TextureFormat::R8g8b8a8Unorm,
    //     cg::ColorSpace::SRGB,
    //     cg::ImageType::Texture2D,
    //     1
    // );
    struct ImageDescription {
        uint32             mWidth;
        uint32             mHeight;
        mce::TextureFormat mTextureFormat;
        cg::ColorSpace     mColorSpace;
        cg::ImageType      mImageType;
        uint32             mArraySizeOrDepth;
    };
    ImageDescription fakeDescription{
        .mWidth            = static_cast<uint32>(width),
        .mHeight           = static_cast<uint32>(height),
        .mTextureFormat    = mce::TextureFormat::R8g8b8a8Unorm,
        .mColorSpace       = cg::ColorSpace::SRGB,
        .mImageType        = cg::ImageType::Texture2D,
        .mArraySizeOrDepth = 1,
    };
    cg::ImageDescription& description = *reinterpret_cast<cg::ImageDescription*>(&fakeDescription);
    // const cg::ImageBuffer  imageBuffer(std::move(blob), std::move(description));
    const cg::ImageBuffer  imageBuffer(std::move(blob), description);
    const ResourceLocation resource(Core::PathView("imguiFont"), ResourceFileSystem::Raw);
    textureGroup->uploadTexture(resource, imageBuffer);
    texturePtr      = context.getTexture(resource, false);
    io.Fonts->TexID = static_cast<void*>(&texturePtr);
}

static void initImGui(UIRenderContext& context) {
    const auto* screenContext = *reinterpret_cast<ScreenContext**>(reinterpret_cast<std::byte*>(&context) + 16);
    tessellator               = &screenContext->tessellator;

    ImGui::StyleColorsDark();
    auto& style            = ImGui::GetStyle();
    style.AntiAliasedLines = false;
    style.AntiAliasedFill  = false;
    const auto& io         = ImGui::GetIO();
    // io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\msyh.ttc)", 80.0f, nullptr);

    loadFontTexture(context);
    init();
}

inline void setTessellatorColor(const ImU32 c) {
    constexpr float inv255 = 1.0f / 255.0f;
    tessellator->color(
        static_cast<float>(c & 0xFF) * inv255,
        static_cast<float>(c >> 8 & 0xFF) * inv255,
        static_cast<float>(c >> 16 & 0xFF) * inv255,
        static_cast<float>(c >> 24 & 0xFF) * inv255
    );
}
static void renderImGui(UIRenderContext& context) {
    auto*       screenContext   = *reinterpret_cast<ScreenContext**>(reinterpret_cast<std::byte*>(&context) + 16);
    auto*       guiData         = screenContext->guiData.get().get();
    const auto& totalScreenSize = guiData->mScreenSizeData.get().totalScreenSize.get();
    const float scale           = guiData->mGuiScale;
    auto&       io              = ImGui::GetIO();
    io.DisplaySize.x            = totalScreenSize.x;
    io.DisplaySize.y            = totalScreenSize.y;
    // io.FontGlobalScale          = scale * 0.6f;
    io.FontGlobalScale = scale * 0.6f / (/* fontSize */ 80.0f / 16.0f);
    ImGui::NewFrame();

    render();

    ImGui::EndFrame();
    ImGui::Render();

    static int frame = 0;
    if (hasUnloadedTextures || ++frame > 500) {
        loadFontTexture(context);
        frame               = 0;
        hasUnloadedTextures = false;
    }

    auto*      drawData = ImGui::GetDrawData();
    const auto invScale = 1.0f / scale;
    for (auto n = 0; n < drawData->CmdListsCount; n++)
        for (auto* cmdList = drawData->CmdLists[n]; const auto& cmd : cmdList->CmdBuffer) {
            if (cmd.UserCallback) {
                cmd.UserCallback(cmdList, &cmd);
                continue;
            }
            const auto* vertices = cmdList->VtxBuffer.Data + cmd.VtxOffset;
            const auto* indices  = cmdList->IdxBuffer.Data + cmd.IdxOffset;
            tessellator->begin({}, mce::PrimitiveMode::TriangleList, static_cast<int>(cmd.ElemCount), false);
            for (auto i = 0u; i < cmd.ElemCount; i += 3) {
                // ReSharper disable once CppUseStructuredBinding
                const auto& v0 = vertices[indices[i + 2]];
                // ReSharper disable once CppUseStructuredBinding
                const auto& v1 = vertices[indices[i + 1]];
                // ReSharper disable once CppUseStructuredBinding
                const auto& v2 = vertices[indices[i + 0]];
                setTessellatorColor(v0.col);
                tessellator->vertexUV(v0.pos.x * invScale, v0.pos.y * invScale, 0.0f, v0.uv.x, v0.uv.y);
                setTessellatorColor(v1.col);
                tessellator->vertexUV(v1.pos.x * invScale, v1.pos.y * invScale, 0.0f, v1.uv.x, v1.uv.y);
                setTessellatorColor(v2.col);
                tessellator->vertexUV(v2.pos.x * invScale, v2.pos.y * invScale, 0.0f, v2.uv.x, v2.uv.y);
            }
            context.saveCurrentClippingRectangle();
            context.setClippingRectangle(
                {cmd.ClipRect.x * invScale,
                 cmd.ClipRect.z * invScale,
                 cmd.ClipRect.y * invScale,
                 cmd.ClipRect.w * invScale}
            );
            auto mesh = tessellator->end({}, "imgui", {});
            mesh.renderMesh(
                *screenContext,
                guiData->mCursorMat.get(),
                texturePtr,
                0,
                cmd.ElemCount,
                std::monostate{},
                nullptr
            );
            context.restoreSavedClippingRectangle();
        }
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    ScreenViewRender,
    ll::memory::HookPriority::Normal,
    ScreenView,
    &ScreenView::render,
    void,
    ::UIRenderContext& uiRenderContext
) {
    static auto _ = (initImGui(uiRenderContext), true);
    origin(uiRenderContext);
    renderImGui(uiRenderContext);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    TextureGroupUploadTexture,
    ll::memory::HookPriority::Normal,
    mce::TextureGroup,
    &mce::TextureGroup::uploadTexture,
    ::BedrockTexture&,
    ::ResourceLocation const& resourceLocation,
    ::mce::TextureContainer&& textureContainer,
    ::std::string_view        optionalIdentifier
) {
    static auto _ = (textureGroup = this, true);
    return origin(resourceLocation, std::move(textureContainer), optionalIdentifier);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    TextureGroupUnloadAllTextures,
    ll::memory::HookPriority::Normal,
    mce::TextureGroup,
    &mce::TextureGroup::unloadAllTextures,
    void
) {
    hasUnloadedTextures = true;
    return origin();
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    MouseDeviceFeed,
    ll::memory::HookPriority::Normal,
    MouseDevice,
    &MouseDevice::feed,
    void,
    char  actionButtonId,
    schar buttonData,
    short x,
    short y,
    short dx,
    short dy,
    bool  forceMotionlessPointer
) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    return origin(actionButtonId, buttonData, x, y, dx, dy, forceMotionlessPointer);
}

LL_AUTO_STATIC_HOOK(
    WndProcHook,
    ll::memory::HookPriority::Normal,
    WndProc,
    int64,
    ::HWND__* hwnd,
    uint      uMsg,
    uint64    wParam,
    int64     lParam
) {
    auto& io = ImGui::GetIO();
    switch (uMsg) {
    case WM_MOUSEMOVE:
        io.AddMousePosEvent(static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)));
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        io.AddMouseButtonEvent(0, uMsg == WM_LBUTTONDOWN);
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        io.AddMouseButtonEvent(1, uMsg == WM_RBUTTONDOWN);
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        io.AddMouseButtonEvent(2, uMsg == WM_MBUTTONDOWN);
        break;
    case WM_MOUSEWHEEL:
        io.AddMouseWheelEvent(0.0f, GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:;
    }
    return origin(hwnd, uMsg, wParam, lParam);
}

void prepareImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
}

} // namespace ludistream::ui
