#include "../include/input_action.h"

const InputActionInfo g_inputActionTable[] = {
    // System
    { InputAction::Quit,              ysKey::Code::Escape,     "quit",                 "Quit",                    "System" },
    { InputAction::ReloadScript,      ysKey::Code::Return,     "reload_script",        "Reload Engine Script",   "System" },
    { InputAction::ScreenCycle,       ysKey::Code::Tab,        "screen_cycle",         "Change Screen",           "System" },
    { InputAction::Fullscreen,        ysKey::Code::F,          "fullscreen",           "Toggle Fullscreen",       "System" },
    { InputAction::RecordToggle,      ysKey::Code::Insert,     "record_toggle",        "Start/Stop Recording",    "System" },
    { InputAction::SingleStep,        ysKey::Code::Right,      "single_step",          "Single-Step (While Paused)", "System" },
    { InputAction::TurboEditorToggle, ysKey::Code::O,          "turbo_editor_toggle",  "Toggle Turbo Editor",     "System" },

    // Engine
    { InputAction::Ignition,          ysKey::Code::A,          "ignition",             "Toggle Ignition",         "Engine" },
    { InputAction::Starter,           ysKey::Code::S,          "starter",              "Starter (Hold)",          "Engine" },
    { InputAction::DynoEnable,        ysKey::Code::D,          "dyno_enable",          "Toggle Dyno",             "Engine" },
    { InputAction::DynoHold,          ysKey::Code::H,          "dyno_hold",            "Toggle RPM Hold",         "Engine" },
    { InputAction::DynoHoldAdjust,    ysKey::Code::G,          "dyno_hold_adjust",     "Adjust Hold Speed (+Scroll)", "Engine" },
    { InputAction::DynoStatsLog,      ysKey::Code::I,          "dyno_stats_log",       "Log Dyno Stats",          "Engine" },

    // Throttle
    { InputAction::ThrottleIdle,      ysKey::Code::Q,          "throttle_idle",        "Throttle - Idle",         "Throttle" },
    { InputAction::ThrottleLow,       ysKey::Code::W,          "throttle_low",         "Throttle - Low",          "Throttle" },
    { InputAction::ThrottleMid,       ysKey::Code::E,          "throttle_mid",         "Throttle - Mid",          "Throttle" },
    { InputAction::ThrottleFull,      ysKey::Code::R,          "throttle_full",        "Throttle - Full",         "Throttle" },
    { InputAction::FineControl,       ysKey::Code::Space,      "fine_control",         "Fine Control Modifier",   "Throttle" },

    // Drivetrain
    { InputAction::GearUp,            ysKey::Code::Up,         "gear_up",              "Shift Up",                "Drivetrain" },
    { InputAction::GearDown,          ysKey::Code::Down,       "gear_down",            "Shift Down",              "Drivetrain" },
    { InputAction::ClutchDecrease,    ysKey::Code::T,          "clutch_decrease",      "Clutch - Decrease",       "Drivetrain" },
    { InputAction::ClutchIncrease,    ysKey::Code::U,          "clutch_increase",      "Clutch - Increase",       "Drivetrain" },
    { InputAction::ClutchDepress,     ysKey::Code::Shift,      "clutch_depress",       "Clutch - Depress (Hold)", "Drivetrain" },
    { InputAction::ClutchHold,        ysKey::Code::Y,          "clutch_hold",          "Clutch - Hold Position",  "Drivetrain" },
    { InputAction::Brake,             ysKey::Code::L,          "brake",                "Brake",                   "Drivetrain" },

    // Audio Mix
    { InputAction::Volume,            ysKey::Code::Z,          "volume",               "Volume (+Scroll)",        "Audio Mix" },
    { InputAction::ConvolutionLevel,  ysKey::Code::X,          "convolution_level",    "Convolution Level (+Scroll)", "Audio Mix" },
    { InputAction::HighFreqGain,      ysKey::Code::C,          "high_freq_gain",       "High Freq. Gain (+Scroll)", "Audio Mix" },
    { InputAction::LowFreqNoise,      ysKey::Code::V,          "low_freq_noise",       "Low Freq. Noise (+Scroll)", "Audio Mix" },
    { InputAction::HighFreqNoise,     ysKey::Code::B,          "high_freq_noise",      "High Freq. Noise (+Scroll)", "Audio Mix" },
    { InputAction::SimFrequency,      ysKey::Code::N,          "sim_frequency",        "Simulation Frequency (+Scroll)", "Audio Mix" },

    // View
    { InputAction::ViewLayerUp,       ysKey::Code::M,          "view_layer_up",        "Increase Render Layer",   "View" },
    { InputAction::ViewLayerDown,     ysKey::Code::OEM_Comma,  "view_layer_down",      "Decrease Render Layer",   "View" },
    { InputAction::DisplayAngleIncrease, ysKey::Code::F1,      "display_angle_increase", "Rotate View +",         "View" },
    { InputAction::DisplayAngleDecrease, ysKey::Code::F2,      "display_angle_decrease", "Rotate View -",         "View" },
    { InputAction::DisplayAngleReset, ysKey::Code::F3,         "display_angle_reset",  "Reset View Angle",        "View" },

    // Simulation Speed
    { InputAction::SimSpeed1,         ysKey::Code::N1,         "sim_speed_1",          "Time Warp 1",             "Simulation Speed" },
    { InputAction::SimSpeed2,         ysKey::Code::N2,         "sim_speed_2",          "Time Warp 2",             "Simulation Speed" },
    { InputAction::SimSpeed3,         ysKey::Code::N3,         "sim_speed_3",          "Time Warp 3",             "Simulation Speed" },
    { InputAction::SimSpeed4,         ysKey::Code::N4,         "sim_speed_4",          "Time Warp 4",             "Simulation Speed" },
    { InputAction::SimSpeed5,         ysKey::Code::N5,         "sim_speed_5",          "Time Warp 5",             "Simulation Speed" },
};

const int g_inputActionCount = sizeof(g_inputActionTable) / sizeof(g_inputActionTable[0]);
