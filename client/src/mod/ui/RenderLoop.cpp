#include "RenderLoop.h"

#include <imgui.h>

namespace ludistream::ui {

void init() {
    auto& style             = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(8.0f, 6.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
}

void render() {
    ImGui::ShowDemoWindow();

    ImGui::Begin("Test Window");

    ImGui::Text("Hello, ImGui!");
    ImGui::Separator();

    static int counter = 0;
    if (ImGui::Button("Click me")) {
        counter++;
    }

    ImGui::SameLine();
    ImGui::Text("count = %d", counter);

    static float value = 0.5f;
    ImGui::SliderFloat("Value", &value, 0.0f, 1.0f);

    ImGui::End();
}

} // namespace ludistream::ui
