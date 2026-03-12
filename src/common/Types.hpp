#pragma once
#include <deque>
#include <vector>
#include <opencv2/core.hpp>

enum class ThreatLevel { LOW, MEDIUM, HIGH };
enum class TrackState  { NEW, ACTIVE, LOST, DEAD };

struct Detection {
    cv::Rect    boundingBox;
    cv::Point2f center;
    float       area;
};

struct Track {
    int         id;
    TrackState  state;
    cv::Rect    boundingBox;
    cv::Point2f position;           // Kalman-filtered centroid
    cv::Point2f velocity;           // Kalman-filtered velocity (px/frame)
    float       estimatedSpeed;
    ThreatLevel threatLevel;
    int         lostFrameCount;
    int         activeFrameCount;
    std::deque<cv::Point2f> trail;  // last 30 positions
};

using DetectionList = std::vector<Detection>;
using TrackList     = std::vector<Track>;
