#include "HudPanel.hpp"
#include <imgui.h>

static const char* threatStr(ThreatLevel lvl) {
    switch (lvl) {
        case ThreatLevel::HIGH:   return "HIGH";
        case ThreatLevel::MEDIUM: return "MED";
        default:                  return "LOW";
    }
}

static ImVec4 threatColor(ThreatLevel lvl) {
    switch (lvl) {
        case ThreatLevel::HIGH:   return ImVec4(1.f, 0.2f, 0.2f, 1.f);
        case ThreatLevel::MEDIUM: return ImVec4(1.f, 1.f,  0.2f, 1.f);
        default:                  return ImVec4(0.2f, 1.f, 0.2f, 1.f);
    }
}

void HudPanel::render(const TrackList& tracks, float fps, bool paused) {
    // Semi-transparent dark background
    ImGui::SetNextWindowBgAlpha(0.75f);
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("HUD", nullptr, flags);

    // FPS + status line
    ImGui::TextColored(ImVec4(0.8f, 1.f, 0.8f, 1.f),
                       "FPS: %.1f  |  Targets: %zu%s",
                       fps,
                       tracks.size(),
                       paused ? "  [PAUSED]" : "");

    ImGui::Separator();

    // Target table
    if (ImGui::BeginTable("targets", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("ID",     ImGuiTableColumnFlags_WidthFixed, 35.f);
        ImGui::TableSetupColumn("Speed",  ImGuiTableColumnFlags_WidthFixed, 70.f);
        ImGui::TableSetupColumn("Threat", ImGuiTableColumnFlags_WidthFixed, 55.f);
        ImGui::TableSetupColumn("Frames", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& t : tracks) {
            if (t.state == TrackState::DEAD) continue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", t.id);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.1f px/f", t.estimatedSpeed);

            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(threatColor(t.threatLevel),
                               "%s", threatStr(t.threatLevel));

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", t.activeFrameCount);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}
