#ifndef ATG_ENGINE_SIM_ENGINE_SIM_APPLICATION_H
#define ATG_ENGINE_SIM_ENGINE_SIM_APPLICATION_H

#include "geometry_generator.h"
#include "simulator.h"
#include "engine.h"
#include "simulation_object.h"
#include "ui_manager.h"
#include "dynamometer.h"
#include "oscilloscope.h"
#include "audio_buffer.h"
#include "convolution_filter.h"
#include "shaders.h"
#include "engine_view.h"
#include "right_gauge_cluster.h"
#include "cylinder_temperature_gauge.h"
#include "synthesizer.h"
#include "oscilloscope_cluster.h"
#include "performance_cluster.h"
#include "load_simulation_cluster.h"
#include "mixer_cluster.h"
#include "info_cluster.h"
#include "turbocharger_editor_cluster.h"
#include "application_settings.h"
#include "transmission.h"
#include "sim_frequency_cache.h"
#include "key_bindings.h"
#include "settings_cluster.h"
#include "dyno_cluster.h"

#include "delta.h"
#include "dtv.h"

#include <vector>

// The 3 screens Tab cycles through. Dyno and Settings are reached via
// toolbar buttons instead -- see m_dynoScreenVisible and m_settingsCluster's
// own visibility -- so they don't disturb the existing Tab/Escape muscle
// memory.
enum class ScreenId {
    Main,
    Telemetry,
    Cinematic
};

class EngineSimApplication {
    private:
        static std::string s_buildVersion;

    public:
        EngineSimApplication();
        virtual ~EngineSimApplication();

        static std::string getBuildVersion() { return s_buildVersion; }

        void initialize(void *instance, ysContextObject::DeviceAPI api);
        void run();
        void destroy();

        void loadEngine(Engine *engine, Vehicle *vehicle, Transmission *transmission);
        void drawGenerated(
                const GeometryGenerator::GeometryIndices &indices,
                int layer = 0);
        void drawGeneratedUi(
                const GeometryGenerator::GeometryIndices &indices,
                int layer = 0);
        void drawGenerated(
                const GeometryGenerator::GeometryIndices &indices,
                int layer,
                dbasic::StageEnableFlags flags);
        void configure(const ApplicationSettings &settings);
        GeometryGenerator *getGeometryGenerator() { return &m_geometryGenerator; }

        Shaders *getShaders() { return &m_shaders; }
        dbasic::TextRenderer *getTextRenderer() { return &m_textRenderer; }

        void createObjects(Engine *engine);
        void destroyObjects();
        dbasic::DeltaEngine *getEngine() { return &m_engine; }

        float pixelsToUnits(float pixels) const;
        float unitsToPixels(float units) const;

        ysVector getBackgroundColor() const { return m_background; }
        ysVector getForegroundColor() const { return m_foreground; }
        ysVector getHightlight1Color() const { return m_highlight1; }
        ysVector getPink() const { return m_pink; }
        ysVector getGreen() const { return m_green; }
        ysVector getYellow() const { return m_yellow; }
        ysVector getRed() const { return m_red; }
        ysVector getOrange() const { return m_orange; }
        ysVector getBlue() const { return m_blue; }

        const SimulationObject::ViewParameters &getViewParameters() const;
        void setViewLayer(int view) { m_viewParameters.Layer0 = view; }

        dbasic::AssetManager *getAssetManager() { return &m_assetManager; }

        int getScreenWidth() const { return m_screenWidth; }
        int getScreenHeight() const { return m_screenHeight; }

        Simulator *getSimulator() { return m_simulator; }
        InfoCluster *getInfoCluster() { return m_infoCluster; }
        ApplicationSettings* getAppSettings() { return &m_applicationSettings; }
        KeyBindings *getKeyBindings() { return &m_keyBindings; }
        void saveKeyBindings() { m_keyBindings.saveToDisk(m_keyBindingsPath); }

        void toggleSettingsScreen();
        void toggleDynoScreen();

        // Safe to call any time: performs the reload immediately.
        void reloadSimulation();
        void loadSimulationFromDialog();

        // Safe to call from inside a UI callback (e.g. a button click):
        // defers the actual reload to the top of the next frame, since
        // reloadSimulation()/loadSimulationFromDialog() destroy and rebuild
        // the whole UI tree, which must not happen while a UI element from
        // that tree is still executing a callback on the call stack.
        void requestReload() { m_reloadRequested = true; }
        void requestLoadSimulationFromDialog() { m_loadDialogRequested = true; }

        // Only applies colors/units from the script's `configure(...)` --
        // doesn't touch the engine/vehicle/simulator or rebuild the UI tree,
        // so unlike the above this is safe to call directly from a click.
        void loadThemeFromDialog();

    protected:
        void loadScript(const std::string &path);
        void loadTheme(const std::string &path);
        void processEngineInput();
        void renderScene();

        void refreshUserInterface();

    protected:
        double m_speedSetting = 1.0;
        double m_targetSpeedSetting = 1.0;

        double m_clutchPressure = 1.0;
        double m_targetClutchPressure = 1.0;
        int m_lastMouseWheel = 0;
        int m_lastLoggedGear = -1;

        // Ramps up automatically in process() until framerate starts
        // dipping below target; false once the user manually sets a
        // frequency with [N], so it stays out of the way.
        bool m_autoTuneSimFrequency = true;

        // Auto-tune settles at the midpoint of the oscillation band once
        // it's bracketed one, instead of continuously bang-bang-ing the
        // frequency forever (which was audible as pitch jitter). These
        // track that bracketing search; see process().
        bool m_simFrequencyPegged = false;
        bool m_simFrequencyRampingUp = true;
        double m_simFrequencyHigh = -1.0;
        double m_simFrequencyLow = -1.0;
        double m_simFrequencySearchTime = 0.0;

    protected:
        virtual void initialize();
        virtual void process(float dt);
        virtual void render();

        float m_displayAngle;
        float m_displayHeight;
        int m_gameWindowHeight;
        int m_screenWidth;
        int m_screenHeight;
        
        ApplicationSettings m_applicationSettings;
        KeyBindings m_keyBindings;
        std::string m_keyBindingsPath;
        dbasic::ShaderSet m_shaderSet;
        Shaders m_shaders;

        dbasic::DeltaEngine m_engine;
        dbasic::AssetManager m_assetManager;

        std::string m_assetPath;
        std::string m_targetScript;
        std::string m_currentEnginePath;
        SimFrequencyCache m_simFrequencyCache;
        bool m_reloadRequested = false;
        bool m_loadDialogRequested = false;

        ysRenderTarget *m_mainRenderTarget;
        ysGPUBuffer *m_geometryVertexBuffer;
        ysGPUBuffer *m_geometryIndexBuffer;

        GeometryGenerator m_geometryGenerator;
        dbasic::TextRenderer m_textRenderer;

        std::vector<SimulationObject *> m_objects;
        Engine *m_iceEngine;
        Vehicle *m_vehicle;
        Transmission *m_transmission;
        Simulator *m_simulator;
        double m_dynoSpeed;
        double m_torque;

        UiManager m_uiManager;
        EngineView *m_engineView;
        RightGaugeCluster *m_rightGaugeCluster;
        OscilloscopeCluster *m_oscCluster;
        CylinderTemperatureGauge *m_temperatureGauge;
        PerformanceCluster *m_performanceCluster;
        LoadSimulationCluster *m_loadSimulationCluster;
        MixerCluster *m_mixerCluster;
        InfoCluster *m_infoCluster;
        TurbochargerEditorCluster *m_turboEditorCluster;
        SettingsCluster *m_settingsCluster;
        DynoCluster *m_dynoCluster;
        SimulationObject::ViewParameters m_viewParameters;

        bool m_paused;
        bool m_turboEditorVisible = false;
        bool m_dynoScreenVisible = false;

    protected:
        void startRecording();
        void updateScreenSizeStability();
        bool readyToRecord();
        void stopRecording();
        void recordFrame();
        bool isRecording() const { return m_recording; }

        static constexpr int ScreenResolutionHistoryLength = 5;
        int m_screenResolution[ScreenResolutionHistoryLength][2];
        int m_screenResolutionIndex;
        bool m_recording;

        ysVector m_background;
        ysVector m_foreground;
        ysVector m_shadow;
        ysVector m_highlight1;
        ysVector m_highlight2;

        ysVector m_pink;
        ysVector m_orange;
        ysVector m_yellow;
        ysVector m_red;
        ysVector m_green;
        ysVector m_blue;

        ysAudioBuffer *m_outputAudioBuffer;
        AudioBuffer m_audioBuffer;
        ysAudioSource *m_audioSource;

        int m_oscillatorSampleOffset;
        ScreenId m_screen;

        // Turbine whine -- one phase accumulator per turbocharger, mixed
        // directly into the output samples in process(). Not routed
        // through Synthesizer since that pipeline is built around
        // convolving per-cylinder exhaust pulses with impulse responses,
        // not a frequency-tracking oscillator.
        std::vector<double> m_turbineWhinePhase;
        std::vector<double> m_turbineWhineFrequency;
        std::vector<double> m_turbineWhineAmplitude;

        // Compressor surge/flutter -- the "brrrt" chatter when the
        // throttle snaps shut while the turbo is still spooled and boost
        // has nowhere to go. Layered noise gated at a chatter rate rather
        // than a modeled surge cycle -- see updateTurbineWhineParameters().
        std::vector<double> m_flutterPhase;
        std::vector<double> m_flutterAmplitude;
        std::vector<double> m_flutterGate;
        std::vector<double> m_flutterIntensity;
        std::vector<double> m_flutterRate;
        std::vector<double> m_flutterBurstGain;
        std::vector<double> m_flutterEnvelope;
        std::vector<double> m_flutterFilterState;

        void updateTurbineWhineParameters(double dt);
        int sampleTurbineWhine();

        // Supercharger whine -- same "phase accumulator mixed directly
        // into output samples" approach as the turbine whine above (see
        // its comment), driven by Supercharger::getShaftSpeed() instead
        // of a turbo's. No flutter/BOV layer: surge/blow-off venting
        // isn't a thing for a belt-driven blower the way it is for a
        // turbo's compressor, so this is just the continuous tone. A
        // clutched, currently-disengaged supercharger's shaft speed is
        // already forced to 0 by Supercharger::process() (the rotor isn't
        // being driven), so this naturally goes silent on a twincharger's
        // blower-to-turbo handoff with no extra gating needed here.
        std::vector<double> m_superchargerWhinePhase;
        std::vector<double> m_superchargerWhineFrequency;
        std::vector<double> m_superchargerWhineAmplitude;

        void updateSuperchargerWhineParameters(double dt);
        int sampleSuperchargerWhine();

        // Recorded BOV sample playback -- one decoded buffer per
        // turbocharger (see loadBovSounds()). Played once (not looped) on
        // an actual off-throttle lift while that turbo has real boost to
        // vent -- not on surge/flutter chatter or on merely hitting the
        // wastegate's max_boost setpoint while still on throttle, both of
        // which are unrelated to a driver-triggered blow-off release. See
        // the throttle-edge check in updateTurbineWhineParameters() and
        // playback in sampleTurbineWhine(). A turbo with no bov_sound (or
        // one that failed to decode) gets an empty buffer here and falls
        // back to the synthesized flutter chuff instead.
        void loadBovSounds();
        std::vector<std::vector<int16_t>> m_bovSampleData;
        std::vector<double> m_bovSampleRate;
        std::vector<double> m_bovVolume;
        std::vector<double> m_bovPlaybackPosition;
        std::vector<bool> m_bovPlaying;

        // Per-trigger loudness multiplier on top of m_bovVolume's fixed
        // asset volume -- set once when a shot fires (see
        // updateTurbineWhineParameters()), scaled by how much boost was
        // actually trapped at that instant relative to that turbo's own
        // max_boost setpoint, so a bare-minimum trigger chirps quietly and
        // a full-boost lift cracks at full volume instead of every shot
        // sounding the same regardless of how much boost it's venting.
        std::vector<double> m_bovVolumeScale;

        // Latches true the moment the throttle is seen open (below the "was
        // open" threshold) and stays true across however many video frames
        // the lift-off takes to finish landing above the "closed" threshold
        // -- a plain previous-vs-current-sample comparison only catches a
        // lift-off that completes within a single frame, and smoothed
        // throttle input (see EngineSimApplication::processEngineInput()'s
        // m_speedSetting exponential filter) reliably steps across the
        // 0.5-0.9 gap over several frames instead, so that comparison
        // almost never actually fired.
        bool m_bovThrottleWasOpen = false;

        // Peak-held boost pressure per turbo -- the compressor's own
        // backflow/surge response to a closed throttle plays out over many
        // physics substeps within a single video frame, so by the time
        // this (once-per-frame) code samples getBoostPressure() straight,
        // it can already read close to zero even though there was real
        // boost to vent at the instant the throttle actually closed. This
        // holds the recent peak (fast attack, slow decay) so the
        // lift-off check above still sees it.
        std::vector<double> m_bovPeakBoost;

#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
        atg_dtv::Encoder m_encoder;
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
};

#endif /* ATG_ENGINE_SIM_ENGINE_SIM_APPLICATION_H */
