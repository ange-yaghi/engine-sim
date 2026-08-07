#ifndef ATG_ENGINE_SIM_BOV_SOUND_H
#define ATG_ENGINE_SIM_BOV_SOUND_H

#include <string>

// A blow-off/wastegate flutter sample assigned to a turbocharger. Mirrors
// ImpulseResponse (filename + volume, resolved to an absolute path by the
// scripting layer) -- decoding and playback happen in the application
// layer, same as impulse responses are only decoded once they reach
// EngineSimApplication.
class BovSound {
public:
    BovSound();
    virtual ~BovSound();

    void initialize(const std::string &filename, double volume);
    std::string getFilename() const { return m_filename; }
    double getVolume() const { return m_volume; }

protected:
    std::string m_filename;
    double m_volume;
};

#endif /* ATG_ENGINE_SIM_BOV_SOUND_H */
