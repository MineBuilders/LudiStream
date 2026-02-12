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
#include "mc/deps/input/HIDController.h"
#include "mc/deps/input/MouseDevice.h"
#include "mc/deps/input/win32/HIDControllerWin32.h"
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
    static ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    // builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    builder.BuildRanges(&ranges);
    io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\msyh.ttc)", 80.0f, nullptr, ranges.Data);

    // TODO: 上传超大 range 字形的 texture 会很吃显存！
    // TODO: 缩小字号，80.0f 太大了
    // TODO: builder.AddText(u8"在这里写会用到的字！就只会加载这些字到 texture！");
    // TODO: 将添加字体的代码提到 RenderLoop::init 里面，并让 init 先于 loadFontTexture 执行

    loadFontTexture(context);
    init();
}

static void updateMouseCursor() {
    const auto imguiCursor = ImGui::GetMouseCursor();
    if (imguiCursor == ImGuiMouseCursor_None || ImGui::GetIO().MouseDrawCursor) {
        SetCursor(nullptr);
        return;
    }

    HCURSOR cursor = nullptr;
    switch (imguiCursor) {
    case ImGuiMouseCursor_Arrow:
        cursor = LoadCursor(nullptr, IDC_ARROW);
        break;
    case ImGuiMouseCursor_TextInput:
        cursor = LoadCursor(nullptr, IDC_IBEAM);
        break;
    case ImGuiMouseCursor_ResizeAll:
        cursor = LoadCursor(nullptr, IDC_SIZEALL);
        break;
    case ImGuiMouseCursor_ResizeEW:
        cursor = LoadCursor(nullptr, IDC_SIZEWE);
        break;
    case ImGuiMouseCursor_ResizeNS:
        cursor = LoadCursor(nullptr, IDC_SIZENS);
        break;
    case ImGuiMouseCursor_ResizeNESW:
        cursor = LoadCursor(nullptr, IDC_SIZENESW);
        break;
    case ImGuiMouseCursor_ResizeNWSE:
        cursor = LoadCursor(nullptr, IDC_SIZENWSE);
        break;
    case ImGuiMouseCursor_Hand:
        cursor = LoadCursor(nullptr, IDC_HAND);
        break;
    case ImGuiMouseCursor_NotAllowed:
        cursor = LoadCursor(nullptr, IDC_NO);
        break;
    default:
        cursor = LoadCursor(nullptr, IDC_ARROW);
        break;
    }
    SetCursor(cursor);
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
    io.FontGlobalScale = scale * 0.6f / (/* fontSize */ 80.0f / 16.0f); // TODO: 别忘记这里也有 fontSize
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
    updateMouseCursor();
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

static ImGuiKey virtualKeyToImGuiKey(const int vk) {
    if (vk >= 'A' && vk <= 'Z') return static_cast<ImGuiKey>(ImGuiKey_A + (vk - 'A'));
    if (vk >= '0' && vk <= '9') return static_cast<ImGuiKey>(ImGuiKey_0 + (vk - '0'));
    switch (vk) {
    case VK_SPACE:
        return ImGuiKey_Space;
    case VK_RETURN:
        return ImGuiKey_Enter;
    case VK_ESCAPE:
        return ImGuiKey_Escape;
    case VK_TAB:
        return ImGuiKey_Tab;
    case VK_BACK:
        return ImGuiKey_Backspace;

    case VK_LEFT:
        return ImGuiKey_LeftArrow;
    case VK_RIGHT:
        return ImGuiKey_RightArrow;
    case VK_UP:
        return ImGuiKey_UpArrow;
    case VK_DOWN:
        return ImGuiKey_DownArrow;

    case VK_SHIFT:
        return ImGuiKey_LeftShift;
    case VK_CONTROL:
        return ImGuiKey_LeftCtrl;
    case VK_MENU:
        return ImGuiKey_LeftAlt;

    default:
        return ImGuiKey_None;
    }
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    HIDControllerOnKeyDown,
    ll::memory::HookPriority::Normal,
    HIDController,
    &HIDController::$onKeyDown,
    void,
    int keyCode
) {
    auto&      io  = ImGui::GetIO();
    const auto key = virtualKeyToImGuiKey(keyCode);
    if (key != ImGuiKey_None) io.AddKeyEvent(key, true);
    switch (key) {
    case ImGuiKey_LeftCtrl:
    case ImGuiKey_RightCtrl:
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        break;
    case ImGuiKey_LeftShift:
    case ImGuiKey_RightShift:
        io.AddKeyEvent(ImGuiMod_Shift, true);
        break;
    case ImGuiKey_LeftAlt:
    case ImGuiKey_RightAlt:
        io.AddKeyEvent(ImGuiMod_Alt, true);
        break;
    default:
        break;
    }
    if (io.WantCaptureKeyboard) return;
    return origin(keyCode);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    HIDControllerWin32OnKeyUp,
    ll::memory::HookPriority::Normal,
    HIDControllerWin32,
    &HIDControllerWin32::$onKeyUp,
    void,
    int keyCode
) {
    auto&      io  = ImGui::GetIO();
    const auto key = virtualKeyToImGuiKey(keyCode);
    if (key != ImGuiKey_None) io.AddKeyEvent(key, false);
    switch (key) {
    case ImGuiKey_LeftCtrl:
    case ImGuiKey_RightCtrl:
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        break;
    case ImGuiKey_LeftShift:
    case ImGuiKey_RightShift:
        io.AddKeyEvent(ImGuiMod_Shift, false);
        break;
    case ImGuiKey_LeftAlt:
    case ImGuiKey_RightAlt:
        io.AddKeyEvent(ImGuiMod_Alt, false);
        break;
    default:
        break;
    }
    if (io.WantCaptureKeyboard) return;
    return origin(keyCode);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    HIDControllerWin32OnTextInput,
    ll::memory::HookPriority::Normal,
    HIDControllerWin32,
    &HIDControllerWin32::$onTextInput,
    void,
    ::std::string const& utf8Text
) {
    auto& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(utf8Text.c_str());
    if (io.WantCaptureKeyboard) return;
    return origin(utf8Text);
}

void prepareImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
}

} // namespace ludistream::ui
