#ifndef ATG_ENGINE_SIM_INPUT_ACTION_H
#define ATG_ENGINE_SIM_INPUT_ACTION_H

#include "delta.h"

enum class InputAction {
    // System
    Quit,
    ReloadScript,
    ScreenCycle,
    Fullscreen,
    RecordToggle,
    SingleStep,
    TurboEditorToggle,

    // Engine
    Ignition,
    Starter,
    DynoEnable,
    DynoHold,
    DynoHoldAdjust,
    DynoStatsLog,

    // Throttle
    ThrottleIdle,
    ThrottleLow,
    ThrottleMid,
    ThrottleFull,
    FineControl,

    // Drivetrain
    GearUp,
    GearDown,
    ClutchDecrease,
    ClutchIncrease,
    ClutchDepress,
    ClutchHold,
    Brake,

    // Audio Mix
    Volume,
    ConvolutionLevel,
    HighFreqGain,
    LowFreqNoise,
    HighFreqNoise,
    SimFrequency,

    // View
    ViewLayerUp,
    ViewLayerDown,
    DisplayAngleIncrease,
    DisplayAngleDecrease,
    DisplayAngleReset,

    // Simulation Speed
    SimSpeed1,
    SimSpeed2,
    SimSpeed3,
    SimSpeed4,
    SimSpeed5,

    Count
};

struct InputActionInfo {
    InputAction action;
    ysKey::Code defaultKey;
    const char *scriptName;    // stable identifier used in keybinds.json -- never rename once shipped
    const char *label;         // shown in the settings menu
    const char *group;         // settings-menu section header
};

// Single source of truth for every rebindable action: its default key, its
// persisted identifier, and how it's grouped/labeled in the settings menu.
// Order doesn't matter (KeyBindings looks these up by InputAction), but
// entries are kept grouped for readability here since the settings menu
// walks this table directly to build its sections.
extern const InputActionInfo g_inputActionTable[];
extern const int g_inputActionCount;

#endif /* ATG_ENGINE_SIM_INPUT_ACTION_H */
