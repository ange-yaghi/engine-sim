#include "../include/key_bindings.h"

#include "../include/json_util.h"

namespace {
    const InputActionInfo *findAction(InputAction action) {
        for (int i = 0; i < g_inputActionCount; ++i) {
            if (g_inputActionTable[i].action == action) return &g_inputActionTable[i];
        }

        return nullptr;
    }

    const InputActionInfo *findActionByScriptName(const std::string &name) {
        for (int i = 0; i < g_inputActionCount; ++i) {
            if (name == g_inputActionTable[i].scriptName) return &g_inputActionTable[i];
        }

        return nullptr;
    }

    struct KeyCodeName {
        ysKey::Code code;
        const char *name;
    };

    // Covers every key considered fair game for rebinding -- not the full
    // ysKey::Code enum (skips the browser/media/launch keys no one should
    // bind an engine control to).
    const KeyCodeName g_keyNames[] = {
        { ysKey::Code::A, "A" }, { ysKey::Code::B, "B" }, { ysKey::Code::C, "C" },
        { ysKey::Code::D, "D" }, { ysKey::Code::E, "E" }, { ysKey::Code::F, "F" },
        { ysKey::Code::G, "G" }, { ysKey::Code::H, "H" }, { ysKey::Code::I, "I" },
        { ysKey::Code::J, "J" }, { ysKey::Code::K, "K" }, { ysKey::Code::L, "L" },
        { ysKey::Code::M, "M" }, { ysKey::Code::N, "N" }, { ysKey::Code::O, "O" },
        { ysKey::Code::P, "P" }, { ysKey::Code::Q, "Q" }, { ysKey::Code::R, "R" },
        { ysKey::Code::S, "S" }, { ysKey::Code::T, "T" }, { ysKey::Code::U, "U" },
        { ysKey::Code::V, "V" }, { ysKey::Code::W, "W" }, { ysKey::Code::X, "X" },
        { ysKey::Code::Y, "Y" }, { ysKey::Code::Z, "Z" },

        { ysKey::Code::N0, "0" }, { ysKey::Code::N1, "1" }, { ysKey::Code::N2, "2" },
        { ysKey::Code::N3, "3" }, { ysKey::Code::N4, "4" }, { ysKey::Code::N5, "5" },
        { ysKey::Code::N6, "6" }, { ysKey::Code::N7, "7" }, { ysKey::Code::N8, "8" },
        { ysKey::Code::N9, "9" },

        { ysKey::Code::F1, "F1" }, { ysKey::Code::F2, "F2" }, { ysKey::Code::F3, "F3" },
        { ysKey::Code::F4, "F4" }, { ysKey::Code::F5, "F5" }, { ysKey::Code::F6, "F6" },
        { ysKey::Code::F7, "F7" }, { ysKey::Code::F8, "F8" }, { ysKey::Code::F9, "F9" },
        { ysKey::Code::F10, "F10" }, { ysKey::Code::F11, "F11" }, { ysKey::Code::F12, "F12" },

        { ysKey::Code::Up, "UP" }, { ysKey::Code::Down, "DOWN" },
        { ysKey::Code::Left, "LEFT" }, { ysKey::Code::Right, "RIGHT" },

        { ysKey::Code::Space, "SPACE" }, { ysKey::Code::Shift, "SHIFT" },
        { ysKey::Code::Control, "CTRL" }, { ysKey::Code::Tab, "TAB" },
        { ysKey::Code::Escape, "ESCAPE" }, { ysKey::Code::Return, "ENTER" },
        { ysKey::Code::Insert, "INSERT" }, { ysKey::Code::Delete, "DELETE" },
        { ysKey::Code::Back, "BACKSPACE" }, { ysKey::Code::Home, "HOME" },
        { ysKey::Code::End, "END" }, { ysKey::Code::PageUp, "PAGE UP" },
        { ysKey::Code::PageDown, "PAGE DOWN" }, { ysKey::Code::CapsLock, "CAPS LOCK" },

        { ysKey::Code::OEM_Comma, "," }, { ysKey::Code::OEM_Period, "." },
        { ysKey::Code::OEM_Minus, "-" }, { ysKey::Code::OEM_Plus, "=" },
    };
    const int g_keyNameCount = sizeof(g_keyNames) / sizeof(g_keyNames[0]);
}

KeyBindings::KeyBindings() {
    resetAllToDefaults();
}

bool KeyBindings::isDown(InputAction action, dbasic::DeltaEngine *engine) const {
    return engine->IsKeyDown(get(action));
}

bool KeyBindings::pressed(InputAction action, dbasic::DeltaEngine *engine) const {
    return engine->ProcessKeyDown(get(action));
}

ysKey::Code KeyBindings::get(InputAction action) const {
    return m_bindings[(size_t)action];
}

void KeyBindings::rebind(InputAction action, ysKey::Code key) {
    m_bindings[(size_t)action] = key;
}

void KeyBindings::resetToDefault(InputAction action) {
    const InputActionInfo *info = findAction(action);
    if (info != nullptr) {
        m_bindings[(size_t)action] = info->defaultKey;
    }
}

void KeyBindings::resetAllToDefaults() {
    for (int i = 0; i < g_inputActionCount; ++i) {
        const InputActionInfo &info = g_inputActionTable[i];
        m_bindings[(size_t)info.action] = info.defaultKey;
    }
}

bool KeyBindings::loadFromDisk(const std::string &path) {
    std::map<std::string, std::string> values;
    if (!json_util::readFlatStringMap(path, &values)) return false;

    for (const auto &pair : values) {
        const InputActionInfo *info = findActionByScriptName(pair.first);
        ysKey::Code key;
        if (info != nullptr && keyFromName(pair.second, &key)) {
            m_bindings[(size_t)info->action] = key;
        }
    }

    return true;
}

bool KeyBindings::saveToDisk(const std::string &path) const {
    std::map<std::string, std::string> values;
    for (int i = 0; i < g_inputActionCount; ++i) {
        const InputActionInfo &info = g_inputActionTable[i];
        values[info.scriptName] = keyName(m_bindings[(size_t)info.action]);
    }

    return json_util::writeFlatStringMap(path, values);
}

const std::vector<ysKey::Code> &KeyBindings::bindableKeys() {
    static const std::vector<ysKey::Code> keys = [] {
        std::vector<ysKey::Code> v;
        v.reserve(g_keyNameCount);
        for (int i = 0; i < g_keyNameCount; ++i) v.push_back(g_keyNames[i].code);
        return v;
    }();

    return keys;
}

std::string KeyBindings::keyName(ysKey::Code key) {
    for (int i = 0; i < g_keyNameCount; ++i) {
        if (g_keyNames[i].code == key) return g_keyNames[i].name;
    }

    return "UNKNOWN";
}

bool KeyBindings::keyFromName(const std::string &name, ysKey::Code *out) {
    for (int i = 0; i < g_keyNameCount; ++i) {
        if (name == g_keyNames[i].name) {
            *out = g_keyNames[i].code;
            return true;
        }
    }

    return false;
}
