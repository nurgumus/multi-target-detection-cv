#pragma once
#include "common/Types.hpp"

// Dear ImGui HUD panel: target table + FPS counter.
// Does NOT own the ImGui context — that is set up by Renderer.
class HudPanel {
public:
    HudPanel() = default;

    // Call between ImGui::NewFrame() and ImGui::Render().
    void render(const TrackList& tracks, float fps, bool paused);
};
