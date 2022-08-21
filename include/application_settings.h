#ifndef ATG_ENGINE_SIM_APPLICATION_SETTINGS_H
#define ATG_ENGINE_SIM_APPLICATION_SETTINGS_H
#include <string>

struct ApplicationSettings {
    bool startFullscreen = false;
    std::string powerUnits = "HP";
    std::string torqueUnits = "FTLBS";
    std::string speedUnits = "MPH";
    std::string pressureUnits = "INHG";
    std::string boostUnits = "PSI";

};

#endif /* ATG_ENGINE_SIM_APPLICATION_SETTINGS_H */
