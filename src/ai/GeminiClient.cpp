#include "ai/GeminiClient.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

// --------------------------------------------------------------------------
// libcurl write callback — appends received data into a std::string
// --------------------------------------------------------------------------
static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

// --------------------------------------------------------------------------
// Construction / destruction
// --------------------------------------------------------------------------
GeminiClient::GeminiClient(std::string apiKey,
                           int cooldownSecs,
                           int maxCalls,
                           int maxOutputTokens)
    : m_apiKey(std::move(apiKey))
    , m_cooldownSecs(cooldownSecs)
    , m_maxCalls(maxCalls)
    , m_maxOutputTokens(maxOutputTokens) {}

GeminiClient::~GeminiClient() {
    if (m_worker.joinable())
        m_worker.join();
}

// --------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------
bool GeminiClient::requestAnalysis(const TrackList& tracks, std::string* reason) {
    auto reject = [&](const char* msg) {
        if (reason) *reason = msg;
        return false;
    };

    if (m_isAnalyzing.load())
        return reject("Already analyzing.");

    if (m_callCount >= m_maxCalls)
        return reject("Session call limit reached.");

    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       now - m_lastCall).count();
    if (m_callCount > 0 && elapsed < m_cooldownSecs)
        return reject("Cooldown active.");

    // Serialize NOW on the render thread; worker only sees a plain string.
    std::string prompt = buildPrompt(tracks);

    if (m_worker.joinable())
        m_worker.join();

    m_lastCall = now;
    ++m_callCount;
    m_isAnalyzing.store(true);
    m_worker = std::thread(&GeminiClient::workerFunc, this, std::move(prompt));
    return true;
}

bool GeminiClient::pollResult(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_hasResult) return false;
    out         = std::move(m_responseText);
    m_hasResult = false;
    return true;
}

// --------------------------------------------------------------------------
// Background worker
// --------------------------------------------------------------------------
void GeminiClient::workerFunc(std::string prompt) {
    std::string result;

    if (m_apiKey.empty()) {
        result = "[Error] GOOGLE_API_KEY not set in .env file.";
    } else {
        std::string url =
            "https://generativelanguage.googleapis.com/v1beta/models/"
            "gemini-2.5-flash:generateContent?key=" + m_apiKey;

        // Build request JSON
        json body;
        body["contents"][0]["parts"][0]["text"]              = prompt;
        body["generationConfig"]["maxOutputTokens"]          = m_maxOutputTokens;
        std::string bodyStr = body.dump();

        // Set up curl
        CURL* curl = curl_easy_init();
        if (!curl) {
            result = "[Error] Failed to init curl.";
        } else {
            std::string responseBody;

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     bodyStr.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  (long)bodyStr.size());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curlWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &responseBody);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT,        15L);

            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                result = std::string("[Error] curl: ") + curl_easy_strerror(res);
            } else {
                result = extractText(responseBody);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_responseText = std::move(result);
        m_hasResult    = true;
    }
    m_isAnalyzing.store(false);
}

// --------------------------------------------------------------------------
// Prompt builder
// --------------------------------------------------------------------------
std::string GeminiClient::buildPrompt(const TrackList& tracks) {
    std::ostringstream ss;
    ss << "You are a tactical AI analyst embedded in a real-time computer vision "
          "tracking system. Analyze the following tracking data and give a concise "
          "2-3 sentence situational assessment. Focus on high-threat targets.\n\n"
          "Current tracking report:\n";

    int activeCnt = 0;
    for (const auto& t : tracks) {
        if (t.state == TrackState::DEAD) continue;
        ++activeCnt;

        const char* threat =
            t.threatLevel == ThreatLevel::HIGH   ? "HIGH" :
            t.threatLevel == ThreatLevel::MEDIUM ? "MEDIUM" : "LOW";

        const char* state =
            t.state == TrackState::ACTIVE ? "ACTIVE" :
            t.state == TrackState::LOST   ? "LOST"   : "NEW";

        ss << "- Target #" << t.id
           << " [" << state << "]"
           << ": pos=(" << (int)t.position.x << "," << (int)t.position.y << ")"
           << ", speed=" << t.estimatedSpeed << " px/frame"
           << ", threat=" << threat
           << ", tracked " << t.activeFrameCount << " frames\n";
    }

    if (activeCnt == 0)
        ss << "- No active targets currently detected.\n";

    ss << "\nProvide a brief tactical summary.";
    return ss.str();
}

// --------------------------------------------------------------------------
// JSON response parser
// --------------------------------------------------------------------------
std::string GeminiClient::extractText(const std::string& jsonBody) {
    try {
        auto j = json::parse(jsonBody);
        return j.at("candidates").at(0)
                .at("content").at("parts").at(0)
                .at("text").get<std::string>();
    } catch (const std::exception& e) {
        return std::string("[Parse error] ") + e.what() +
               "\nRaw: " + jsonBody.substr(0, 200);
    }
}
