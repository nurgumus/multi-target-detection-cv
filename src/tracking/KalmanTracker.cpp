#include "KalmanTracker.hpp"

KalmanTracker::KalmanTracker(cv::Point2f initialPos)
    : m_kf(STATE_DIM, MEAS_DIM, 0, CV_32F)
{
    // Transition matrix: constant-velocity model (dt = 1 frame)
    // [ 1 0 1 0 ]
    // [ 0 1 0 1 ]
    // [ 0 0 1 0 ]
    // [ 0 0 0 1 ]
    m_kf.transitionMatrix = (cv::Mat_<float>(4, 4)
        << 1, 0, 1, 0,
           0, 1, 0, 1,
           0, 0, 1, 0,
           0, 0, 0, 1);

    // Measurement matrix: observe x and y only
    // [ 1 0 0 0 ]
    // [ 0 1 0 0 ]
    m_kf.measurementMatrix = (cv::Mat_<float>(2, 4)
        << 1, 0, 0, 0,
           0, 1, 0, 0);

    // Process noise covariance Q
    cv::setIdentity(m_kf.processNoiseCov,    cv::Scalar(1e-2));
    // Measurement noise covariance R
    cv::setIdentity(m_kf.measurementNoiseCov, cv::Scalar(1e-1));
    // Posterior error covariance P (must initialise or first predict gives NaN)
    cv::setIdentity(m_kf.errorCovPost,        cv::Scalar(1.0));

    // Set initial state
    m_kf.statePost.at<float>(0) = initialPos.x;
    m_kf.statePost.at<float>(1) = initialPos.y;
    m_kf.statePost.at<float>(2) = 0.f;
    m_kf.statePost.at<float>(3) = 0.f;
}

cv::Point2f KalmanTracker::predict() {
    cv::Mat pred = m_kf.predict();
    return { pred.at<float>(0), pred.at<float>(1) };
}

cv::Point2f KalmanTracker::correct(cv::Point2f measuredPos) {
    cv::Mat measurement = (cv::Mat_<float>(2, 1) << measuredPos.x, measuredPos.y);
    cv::Mat corrected   = m_kf.correct(measurement);
    return { corrected.at<float>(0), corrected.at<float>(1) };
}

cv::Point2f KalmanTracker::getPosition() const {
    return { m_kf.statePost.at<float>(0),
             m_kf.statePost.at<float>(1) };
}

cv::Point2f KalmanTracker::getVelocity() const {
    return { m_kf.statePost.at<float>(2),
             m_kf.statePost.at<float>(3) };
}
