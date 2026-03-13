#include "AiPanel.hpp"
#include <imgui.h>

bool AiPanel::render(bool isAnalyzing, const std::string& text,
                     int /*winW*/, int winH, int callsUsed, int callsMax) {
    constexpr float PANEL_W = 420.f;
    constexpr float PANEL_H = 210.f;
    constexpr float MARGIN  = 10.f;

    ImGui::SetNextWindowBgAlpha(0.80f);
    ImGui::SetNextWindowPos(
        ImVec2(MARGIN, (float)winH - PANEL_H - MARGIN),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(PANEL_W, PANEL_H), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar  |
        ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("AiPanel", nullptr, flags);

    // Header + call counter
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.f, 1.f), "[ AI TACTICAL ANALYST ]");
    ImGui::SameLine();
    ImGui::TextDisabled("  calls: %d / %d", callsUsed, callsMax);
    ImGui::Separator();

    // Content area (scrollable in case text is long)
    ImGui::BeginChild("AiContent", ImVec2(0.f, PANEL_H - 72.f), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    if (isAnalyzing) {
        // Animated spinner
        const char* frames[] = { "|", "/", "-", "\\" };
        int idx = (int)(ImGui::GetTime() * 4.0) % 4;
        ImGui::TextColored(ImVec4(1.f, 1.f, 0.4f, 1.f),
                           "%s  Analyzing tactical situation...", frames[idx]);
    } else if (!text.empty()) {
        ImGui::TextWrapped("%s", text.c_str());
    } else {
        ImGui::TextDisabled("Press [G] or click the button to run analysis.");
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Button (disabled while a request is in flight)
    ImGui::BeginDisabled(isAnalyzing);
    bool clicked = ImGui::Button("Analyze Now", ImVec2(-1.f, 0.f));
    ImGui::EndDisabled();

    ImGui::End();

    return clicked;
}
