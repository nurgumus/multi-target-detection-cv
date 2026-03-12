#pragma once
#include "KalmanTracker.hpp"
#include "common/Types.hpp"
#include <vector>
#include <memory>

// Manages the full track lifecycle:
//   predict → IoU cost matrix → Hungarian assignment → correct / FSM update
class TrackManager {
public:
    TrackManager() = default;

    // Call once per frame with the latest detections.
    // Mutates internal track list; call getTracks() to read results.
    void update(const DetectionList& detections);

    const TrackList& getTracks() const { return m_tracks; }

private:
    // FSM thresholds
    static constexpr int CONFIRM_HITS  = 3;   // NEW → ACTIVE
    static constexpr int MAX_LOST      = 15;  // LOST → DEAD

    static constexpr float IOU_THRESHOLD = 0.15f; // minimum IoU to consider a match

    // Parallel structures: one Kalman tracker per track (same index)
    TrackList                              m_tracks;
    std::vector<std::unique_ptr<KalmanTracker>> m_kalmanTrackers;
    int                                    m_nextId = 1;

    // Compute intersection-over-union between two bounding boxes.
    static float iou(const cv::Rect& a, const cv::Rect& b);

    // Spawn a new track for an unmatched detection.
    void spawnTrack(const Detection& det);

    // Remove DEAD tracks (and matching Kalman trackers).
    void pruneDead();
};
