#pragma once
#include <fstream>
#include <string>

// Reads a .env file (KEY = VALUE format) and returns the value for `key`.
// Returns empty string if the key is not found or the file cannot be opened.
inline std::string loadEnvVar(const std::string& filePath, const std::string& key) {
    std::ifstream file(filePath);
    std::string   line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);

        auto trim = [](std::string& s) {
            const char* ws = " \t\r\n";
            s.erase(0, s.find_first_not_of(ws));
            auto last = s.find_last_not_of(ws);
            if (last != std::string::npos) s.erase(last + 1);
            else s.clear();
        };
        trim(k);
        trim(v);

        if (k == key) return v;
    }
    return {};
}
