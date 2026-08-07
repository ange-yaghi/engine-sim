#ifndef ATG_ENGINE_SIM_SETTINGS_CLUSTER_H
#define ATG_ENGINE_SIM_SETTINGS_CLUSTER_H

#include "ui_element.h"
#include "ui_button.h"
#include "input_action.h"
#include "key_bindings.h"

#include <string>
#include <vector>

// Modal keybind-remapping overlay. Rendered on top of whatever screen is
// active underneath (a solid backdrop fully obscures it, rather than
// attempting alpha-blended dimming). Rebinding is capture-based -- click
// REBIND, press the new key -- since this UI toolkit has no text-entry
// widget to type a key name into.
class SettingsCluster : public UiElement {
    public:
        SettingsCluster();
        virtual ~SettingsCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();
        virtual void signal(UiElement *element, Event event);

        // Opens the panel and snapshots current bindings (so Close can
        // discard unsaved edits even if nothing has ever been saved to disk).
        void open();

        // Restores the snapshot taken by open() (discarding unsaved edits)
        // and hides the panel.
        void close();

        bool isCapturing() const { return m_capturingAction != InputAction::Count; }
        void cancelCapture() { m_capturingAction = InputAction::Count; }

    protected:
        struct LayoutItem {
            bool isHeader;
            std::string headerLabel;
            int actionIndex; // valid when !isHeader
        };

        struct KeybindRow {
            int actionIndex;
            UiButton *rebindButton;
        };

        std::vector<LayoutItem> m_layout;
        std::vector<KeybindRow> m_rows;

        UiButton *m_saveButton;
        UiButton *m_resetButton;
        UiButton *m_closeButton;

        InputAction m_capturingAction;
        KeyBindings::Snapshot m_snapshot;
};

#endif /* ATG_ENGINE_SIM_SETTINGS_CLUSTER_H */
