#ifndef ATG_ENGINE_SIM_TURBOCHARGER_EDITOR_CLUSTER_H
#define ATG_ENGINE_SIM_TURBOCHARGER_EDITOR_CLUSTER_H

#include "ui_element.h"
#include "ui_number_field.h"
#include "ui_button.h"
#include "turbocharger.h"
#include "engine.h"

#include <vector>

// Phase 1+2 of docs/engine-editor-design.md: a read-only inspector that
// becomes a live editor once a turbocharger is found. Deliberately skips the
// design doc's full EditableEngineDocument -> .mr serializer -> recompile ->
// loadEngine() round trip (that machinery is needed once *every* node type
// is editable and engines need to be built from scratch/saved to disk). For
// a single already-loaded node type, Turbocharger::initialize() is already
// the live-mutation entry point the doc calls out in section 2.5 -- it only
// touches this turbo's own gas-system state, not rigid-body/audio topology,
// so it's safe and cheap to call on every drag frame. Editing therefore
// applies instantly with no recompile, no temp file, and no save-to-disk;
// closing/reloading the engine discards edits (see the design doc for the
// persistence phase this intentionally does not implement).
class TurbochargerEditorCluster : public UiElement {
    public:
        TurbochargerEditorCluster();
        virtual ~TurbochargerEditorCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void update(float dt);
        virtual void render();
        virtual void signal(UiElement *element, Event event);

        // Re-scans the engine for a turbocharger and (re)seeds the editor's
        // fields from it. Call whenever the loaded engine changes.
        void setEngine(Engine *engine);

    protected:
        struct FieldRow {
            const char *label;
            const char *unit;
            double toDisplay;      // internal-unit value -> display-unit value
            double displayMin, displayMax;
            int precision;
            double Turbocharger::Parameters::*member;
            UiNumberField *field;
        };

        void layoutRows();

        Engine *m_engine;
        Turbocharger *m_turbo;

        // Editor's own copy of the spec parameters -- the source of truth
        // for Apply, independent of Turbocharger's runtime m_parameters
        // (which initialize() overwrites in place with wheel-scaled
        // turbineRateK/turbineInertia; re-feeding that back in would
        // compound the scale factor every edit).
        Turbocharger::Parameters m_params;

        std::vector<FieldRow> m_rows;

        // Anti-lag has no per-value tuning UI (yet) -- just the one on/off
        // switch. A UiButton has no built-in toggle state, so `update()`
        // re-derives its label from Engine::getAntiLagEnabled() every frame.
        UiButton *m_antiLagButton;
};

#endif /* ATG_ENGINE_SIM_TURBOCHARGER_EDITOR_CLUSTER_H */
