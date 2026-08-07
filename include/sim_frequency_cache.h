#ifndef ATG_ENGINE_SIM_SIM_FREQUENCY_CACHE_H
#define ATG_ENGINE_SIM_SIM_FREQUENCY_CACHE_H

#include <string>
#include <unordered_map>

// Remembers the simulation frequency the auto-tuner settled on for each
// engine, keyed by the path of its .mr script, so loading an engine it's
// already seen can start from last time's answer instead of re-hunting for
// it from the floor every time. Persisted under %APPDATA% so it survives
// between runs.
class SimFrequencyCache {
    public:
        void load();

        bool get(const std::string &enginePath, double *frequency) const;
        void set(const std::string &enginePath, double frequency);

    protected:
        std::string getFilePath() const;

        std::unordered_map<std::string, double> m_entries;
};

#endif /* ATG_ENGINE_SIM_SIM_FREQUENCY_CACHE_H */
