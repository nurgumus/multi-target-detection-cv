#pragma once
#include "common/Types.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

// Async Gemini API client.
// requestAnalysis() spawns a background thread to call the API without
// blocking the render loop. Poll pollResult() each frame to collect the answer.
//
// Cost guards (tuneable via constructor):
//   cooldownSecs  — minimum seconds between calls (default 30)
//   maxCalls      — hard cap on total calls this session (default 20)
//   maxOutputTokens — max tokens in Gemini's reply (default 120)
class GeminiClient {
public:
    explicit GeminiClient(std::string apiKey,
                          int cooldownSecs    = 30,
                          int maxCalls        = 20,
                          int maxOutputTokens = 120);
    ~GeminiClient();  // joins any live worker thread

    // Non-blocking. Builds a prompt from `tracks` and fires an API request.
    // Silently ignored if: already analyzing, cooldown hasn't elapsed, or cap reached.
    // Returns false (and fills `reason`) when a limit blocks the request.
    bool requestAnalysis(const TrackList& tracks, std::string* reason = nullptr);

    // Non-blocking. Returns true (once) and fills `out` when a result is ready.
    bool pollResult(std::string& out);

    // True while a request is in flight.
    bool isAnalyzing() const { return m_isAnalyzing.load(); }

    int  callsUsed() const { return m_callCount; }
    int  callsMax()  const { return m_maxCalls; }

private:
    std::string m_apiKey;
    int         m_cooldownSecs;
    int         m_maxCalls;
    int         m_maxOutputTokens;

    int         m_callCount = 0;
    std::chrono::steady_clock::time_point m_lastCall {};

    mutable std::mutex m_mutex;
    std::string        m_responseText;
    bool               m_hasResult   = false;

    std::atomic<bool>  m_isAnalyzing { false };
    std::thread        m_worker;

    void        workerFunc(std::string prompt);
    static std::string buildPrompt(const TrackList& tracks);
    static std::string extractText(const std::string& jsonBody);
};
