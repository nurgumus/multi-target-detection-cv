#pragma once
#include "common/Types.hpp"

// Stateless, const-correct threat scorer.
// Mutates Track.threatLevel in-place based on speed and bounding-box area.
class ThreatScorer {
public:
    // Score all active/new tracks in the list.
    void score(TrackList& tracks) const;

private:
    // Thresholds (tunable)
    static constexpr float SPEED_HIGH   = 8.0f;   // px/frame
    static constexpr float SPEED_MEDIUM = 3.0f;

    static constexpr float AREA_HIGH    = 15000.f; // px²
    static constexpr float AREA_MEDIUM  = 5000.f;

    ThreatLevel computeLevel(const Track& t) const;
};
