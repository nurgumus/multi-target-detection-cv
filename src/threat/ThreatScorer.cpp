#include "ThreatScorer.hpp"
#include <algorithm>

void ThreatScorer::score(TrackList& tracks) const {
    for (auto& t : tracks) {
        if (t.state == TrackState::DEAD) continue;
        t.threatLevel = computeLevel(t);
    }
}

ThreatLevel ThreatScorer::computeLevel(const Track& t) const {
    float area = static_cast<float>(t.boundingBox.area());

    // Speed-based score (0–2)
    int speedScore = 0;
    if      (t.estimatedSpeed >= SPEED_HIGH)   speedScore = 2;
    else if (t.estimatedSpeed >= SPEED_MEDIUM) speedScore = 1;

    // Area-based score (0–2)
    int areaScore = 0;
    if      (area >= AREA_HIGH)   areaScore = 2;
    else if (area >= AREA_MEDIUM) areaScore = 1;

    int combined = std::clamp(speedScore + areaScore, 0, 4);

    if (combined >= 3) return ThreatLevel::HIGH;
    if (combined >= 1) return ThreatLevel::MEDIUM;
    return ThreatLevel::LOW;
}
