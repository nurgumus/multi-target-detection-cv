#pragma once
#include <string>

// ImGui panel for the AI Tactical Analyst feature.
// Anchors to the bottom-left of the screen.
// Returns true if the user clicked "Analyze Now" (caller should trigger a request).
class AiPanel {
public:
    AiPanel() = default;

    // Call between ImGui::NewFrame() and ImGui::Render().
    // callsUsed/callsMax are shown as a counter in the panel header.
    bool render(bool isAnalyzing, const std::string& text,
                int winW, int winH, int callsUsed, int callsMax);
};
