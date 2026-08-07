#include "../include/sim_frequency_cache.h"

#include <Windows.h>

#include <cstdlib>
#include <fstream>

std::string SimFrequencyCache::getFilePath() const {
    char appData[MAX_PATH] = {};
    const DWORD length = GetEnvironmentVariableA("APPDATA", appData, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return "";
    }

    const std::string dir = std::string(appData) + "\\engine-sim";
    CreateDirectoryA(dir.c_str(), nullptr);

    return dir + "\\sim_frequency_cache.txt";
}

void SimFrequencyCache::load() {
    m_entries.clear();

    const std::string path = getFilePath();
    if (path.empty()) return;

    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        const size_t tab = line.find('\t');
        if (tab == std::string::npos) continue;

        const std::string key = line.substr(0, tab);
        const double frequency = std::atof(line.substr(tab + 1).c_str());
        if (frequency > 0.0) {
            m_entries[key] = frequency;
        }
    }
}

bool SimFrequencyCache::get(const std::string &enginePath, double *frequency) const {
    const auto it = m_entries.find(enginePath);
    if (it == m_entries.end()) return false;

    if (frequency != nullptr) {
        *frequency = it->second;
    }

    return true;
}

void SimFrequencyCache::set(const std::string &enginePath, double frequency) {
    m_entries[enginePath] = frequency;

    const std::string path = getFilePath();
    if (path.empty()) return;

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) return;

    for (const auto &entry : m_entries) {
        file << entry.first << '\t' << entry.second << '\n';
    }
}
