#include "TrackManager.hpp"
#include "common/Hungarian.hpp"
#include <algorithm>
#include <cmath>

// ---- IoU helper -------------------------------------------------------
float TrackManager::iou(const cv::Rect& a, const cv::Rect& b) {
    cv::Rect inter = a & b;
    if (inter.empty()) return 0.f;
    float interArea = static_cast<float>(inter.area());
    float unionArea = static_cast<float>(a.area() + b.area()) - interArea;
    return (unionArea > 0.f) ? interArea / unionArea : 0.f;
}

// ---- Main update loop -------------------------------------------------
void TrackManager::update(const DetectionList& detections) {
    const int N = static_cast<int>(m_tracks.size());
    const int M = static_cast<int>(detections.size());

    // 1. Predict all tracks
    for (int i = 0; i < N; ++i) {
        cv::Point2f pred = m_kalmanTrackers[i]->predict();
        // Update track position estimate from prediction
        m_tracks[i].position = pred;
    }

    // 2. Build NxM cost matrix (negated IoU so Hungarian minimises)
    std::vector<std::vector<float>> cost(N, std::vector<float>(M, 1.f));
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            float overlap = iou(m_tracks[i].boundingBox, detections[j].boundingBox);
            cost[i][j] = 1.f - overlap;  // cost = 0 → perfect match
        }
    }

    // 3. Hungarian assignment
    Hungarian hungarian;
    std::vector<int> assignment = (N > 0 && M > 0)
        ? hungarian.solve(cost)
        : std::vector<int>(N, -1);

    // Track which detections were matched
    std::vector<bool> detectionMatched(M, false);

    // 4. Process assignments
    for (int i = 0; i < N; ++i) {
        int j = (i < static_cast<int>(assignment.size())) ? assignment[i] : -1;

        // Reject low-overlap assignments
        if (j >= 0 && j < M && cost[i][j] > (1.f - IOU_THRESHOLD))
            j = -1;

        Track& t = m_tracks[i];

        if (j >= 0) {
            // Matched
            detectionMatched[j] = true;
            const Detection& det = detections[j];

            cv::Point2f corrected = m_kalmanTrackers[i]->correct(det.center);
            t.position    = corrected;
            t.velocity    = m_kalmanTrackers[i]->getVelocity();
            t.estimatedSpeed = std::hypot(t.velocity.x, t.velocity.y);
            t.boundingBox = det.boundingBox;
            t.lostFrameCount = 0;
            ++t.activeFrameCount;

            // Update trail
            t.trail.push_back(corrected);
            if (t.trail.size() > 30) t.trail.pop_front();

            // FSM transitions
            if (t.state == TrackState::NEW && t.activeFrameCount >= CONFIRM_HITS)
                t.state = TrackState::ACTIVE;
            else if (t.state == TrackState::LOST)
                t.state = TrackState::ACTIVE;
        } else {
            // Unmatched track
            ++t.lostFrameCount;
            if (t.state == TrackState::ACTIVE || t.state == TrackState::NEW)
                t.state = TrackState::LOST;
            if (t.lostFrameCount >= MAX_LOST)
                t.state = TrackState::DEAD;
        }
    }

    // 5. Spawn new tracks for unmatched detections
    for (int j = 0; j < M; ++j) {
        if (!detectionMatched[j])
            spawnTrack(detections[j]);
    }

    // 6. Remove dead tracks
    pruneDead();
}

// ---- Spawn a new track ------------------------------------------------
void TrackManager::spawnTrack(const Detection& det) {
    Track t;
    t.id              = m_nextId++;
    t.state           = TrackState::NEW;
    t.boundingBox     = det.boundingBox;
    t.position        = det.center;
    t.velocity        = { 0.f, 0.f };
    t.estimatedSpeed  = 0.f;
    t.threatLevel     = ThreatLevel::LOW;
    t.lostFrameCount  = 0;
    t.activeFrameCount = 1;
    t.trail.push_back(det.center);

    m_tracks.push_back(std::move(t));
    m_kalmanTrackers.push_back(std::make_unique<KalmanTracker>(det.center));
}

// ---- Remove DEAD tracks -----------------------------------------------
void TrackManager::pruneDead() {
    for (int i = static_cast<int>(m_tracks.size()) - 1; i >= 0; --i) {
        if (m_tracks[i].state == TrackState::DEAD) {
            m_tracks.erase(m_tracks.begin() + i);
            m_kalmanTrackers.erase(m_kalmanTrackers.begin() + i);
        }
    }
}
