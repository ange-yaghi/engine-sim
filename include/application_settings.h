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

    int colorBackground = 0x0E1012;
    int colorForeground = 0xFFFFFF;
    int colorShadow = 0x0E1012;
    int colorHighlight1 = 0xEF4545;
    int colorHighlight2 = 0xFFFFFF;
    int colorPink = 0xF394BE;
    int colorRed = 0xEE4445;
    int colorOrange = 0xF4802A;
    int colorYellow = 0xFDBD2E;
    int colorBlue = 0x77CEE0;
    int colorGreen = 0xBDD869;
};

#endif /* ATG_ENGINE_SIM_APPLICATION_SETTINGS_H */
