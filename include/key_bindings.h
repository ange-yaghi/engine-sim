#ifndef ATG_ENGINE_SIM_KEY_BINDINGS_H
#define ATG_ENGINE_SIM_KEY_BINDINGS_H

#include "input_action.h"

#include "delta.h"

#include <array>
#include <string>
#include <vector>

namespace dbasic { class DeltaEngine; }

// Indirection layer between gameplay/UI code and raw ysKey::Code checks, so
// every action's key can be remapped (via the settings menu) and persisted,
// instead of being a literal ysKey::Code baked into each call site.
class KeyBindings {
    public:
        KeyBindings();

        // Level check (held) -- use for "while held" actions (starter, clutch, etc).
        bool isDown(InputAction action, dbasic::DeltaEngine *engine) const;

        // Edge check (just pressed this frame) -- use for toggles/one-shot actions.
        bool pressed(InputAction action, dbasic::DeltaEngine *engine) const;

        ysKey::Code get(InputAction action) const;
        void rebind(InputAction action, ysKey::Code key);
        void resetToDefault(InputAction action);
        void resetAllToDefaults();

        using Snapshot = std::array<ysKey::Code, (size_t)InputAction::Count>;
        Snapshot snapshot() const { return m_bindings; }
        void restore(const Snapshot &snap) { m_bindings = snap; }

        bool loadFromDisk(const std::string &path);
        bool saveToDisk(const std::string &path) const;

        // Every key considered "bindable" -- used both for display (settings
        // menu current-binding label) and for the settings menu's capture
        // scan (first one of these that's ProcessKeyDown() this frame wins).
        static const std::vector<ysKey::Code> &bindableKeys();
        static std::string keyName(ysKey::Code key);
        static bool keyFromName(const std::string &name, ysKey::Code *out);

    private:
        std::array<ysKey::Code, (size_t)InputAction::Count> m_bindings;
};

#endif /* ATG_ENGINE_SIM_KEY_BINDINGS_H */
