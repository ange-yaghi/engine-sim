#include "../include/engine_sim_application.h"

#include "../include/piston_object.h"
#include "../include/connecting_rod_object.h"
#include "../include/constants.h"
#include "../include/units.h"
#include "../include/crankshaft_object.h"
#include "../include/cylinder_bank_object.h"
#include "../include/cylinder_head_object.h"
#include "../include/ui_button.h"
#include "../include/combustion_chamber_object.h"
#include "../include/csv_io.h"
#include "../include/exhaust_system.h"
#include "../include/feedback_comb_filter.h"
#include "../include/utilities.h"
#include "../include/bov_sound.h"

#include "../scripting/include/compiler.h"

#include "stb_vorbis.h"

#include <chrono>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <regex>
#include <cstdio>

#include <Windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

namespace {
    // Most .mr files under assets/engines/ are part libraries: they define
    // an engine-building node (e.g. `public node M52B28 { ... engine engine(...) ... }`)
    // but never instantiate it, since that's normally main.mr's job. This
    // inspects the picked file's own text for that pattern -- or for a
    // self-contained `public node main { ... }` like the atg-video-* engines
    // define -- and if found, writes a tiny wrapper next to it that actually
    // invokes it, mirroring what a hand-written main.mr would do. Returns
    // true and fills wrapperPathOut if a wrapper was written; false if the
    // file has neither pattern (e.g. it's not an engine script at all), in
    // which case the caller should just compile the original path.
    bool autoWireEngineScript(const std::string &path, std::string &wrapperPathOut) {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::stringstream ss;
        ss << file.rdbuf();
        const std::string text = ss.str();

        static const std::regex mainNodeRe(R"(public\s+node\s+main\s*\{)");
        static const std::regex enginePosRe(R"(engine\s+engine\s*\()");
        static const std::regex publicNodeRe(R"(public\s+node\s+(\w+)\s*\{)");

        std::string invocation;
        bool needsEngineSimImport = false;

        if (std::regex_search(text, mainNodeRe)) {
            invocation = "main()";
        }
        else {
            std::smatch engineMatch;
            if (!std::regex_search(text, engineMatch, enginePosRe)) {
                return false;
            }

            const std::string before = text.substr(0, engineMatch.position(0));
            std::string entryNode;
            for (auto it = std::sregex_iterator(before.begin(), before.end(), publicNodeRe);
                 it != std::sregex_iterator(); ++it) {
                entryNode = (*it)[1].str();
            }
            if (entryNode.empty()) return false;

            invocation = "set_engine(" + entryNode + "())";
            needsEngineSimImport = true;
        }

        const size_t slash = path.find_last_of("/\\");
        const std::string dir = (slash == std::string::npos) ? "." : path.substr(0, slash);
        const std::string basename = (slash == std::string::npos) ? path : path.substr(slash + 1);

        wrapperPathOut = dir + "/__engine_sim_autoload.mr";
        std::ofstream wrapper(wrapperPathOut);
        if (!wrapper.is_open()) return false;

        if (needsEngineSimImport) {
            wrapper << "import \"engine_sim.mr\"\n";
        }
        wrapper << "import \"" << basename << "\"\n" << invocation << "\n";
        return true;
    }
}

#if ATG_ENGINE_SIM_DISCORD_ENABLED
#include "../discord/Discord.h"
#endif

std::string EngineSimApplication::s_buildVersion = "0.1.12a";

EngineSimApplication::EngineSimApplication() {
    m_assetPath = "";
    m_targetScript = "";
    m_currentEnginePath = "";
    m_simFrequencyCache.load();

    m_geometryVertexBuffer = nullptr;
    m_geometryIndexBuffer = nullptr;

    m_paused = false;
    m_recording = false;
    m_screenResolutionIndex = 0;
    for (int i = 0; i < ScreenResolutionHistoryLength; ++i) {
        m_screenResolution[i][0] = m_screenResolution[i][1] = 0;
    }

    m_background = ysColor::srgbiToLinear(0x0E1012);
    m_foreground = ysColor::srgbiToLinear(0xFFFFFF);
    m_shadow = ysColor::srgbiToLinear(0x0E1012);
    m_highlight1 = ysColor::srgbiToLinear(0xEF4545);
    m_highlight2 = ysColor::srgbiToLinear(0xFFFFFF);
    m_pink = ysColor::srgbiToLinear(0xF394BE);
    m_red = ysColor::srgbiToLinear(0xEE4445);
    m_orange = ysColor::srgbiToLinear(0xF4802A);
    m_yellow = ysColor::srgbiToLinear(0xFDBD2E);
    m_blue = ysColor::srgbiToLinear(0x77CEE0);
    m_green = ysColor::srgbiToLinear(0xBDD869);

    m_displayHeight = (float)units::distance(2.0, units::foot);
    m_outputAudioBuffer = nullptr;
    m_audioSource = nullptr;

    m_torque = 0;
    m_dynoSpeed = 0;

    m_simulator = nullptr;
    m_engineView = nullptr;
    m_rightGaugeCluster = nullptr;
    m_temperatureGauge = nullptr;
    m_oscCluster = nullptr;
    m_performanceCluster = nullptr;
    m_loadSimulationCluster = nullptr;
    m_mixerCluster = nullptr;
    m_infoCluster = nullptr;
    m_turboEditorCluster = nullptr;
    m_iceEngine = nullptr;
    m_mainRenderTarget = nullptr;

    m_vehicle = nullptr;
    m_transmission = nullptr;

    m_oscillatorSampleOffset = 0;
    m_gameWindowHeight = 256;
    m_screenWidth = 256;
    m_screenHeight = 256;
    m_screen = ScreenId::Main;
    m_viewParameters.Layer0 = 0;
    m_viewParameters.Layer1 = 0;

    m_displayAngle = 0.0f;
}

EngineSimApplication::~EngineSimApplication() {
    /* void */
}

void EngineSimApplication::initialize(void *instance, ysContextObject::DeviceAPI api) {
    dbasic::Path modulePath = dbasic::GetModulePath();
    dbasic::Path confPath = modulePath.Append("delta.conf");

    std::string enginePath = "../dependencies/submodules/delta-studio/engines/basic";
    m_assetPath = "../assets";
    if (confPath.Exists()) {
        std::fstream confFile(confPath.ToString(), std::ios::in);

        std::getline(confFile, enginePath);
        std::getline(confFile, m_assetPath);
        enginePath = modulePath.Append(enginePath).ToString();
        m_assetPath = modulePath.Append(m_assetPath).ToString();

        confFile.close();
    }

    m_keyBindingsPath = modulePath.Append("keybinds.json").ToString();
    m_keyBindings.loadFromDisk(m_keyBindingsPath);

    m_engine.GetConsole()->SetDefaultFontDirectory(enginePath + "/fonts/");

    const std::string shaderPath = enginePath + "/shaders/";
    const std::string winTitle = "Engine Sim | AngeTheGreat | UI by soli-dstate | v" + s_buildVersion;
    dbasic::DeltaEngine::GameEngineSettings settings;
    settings.API = api;
    settings.DepthBuffer = false;
    settings.Instance = instance;
    settings.ShaderDirectory = shaderPath.c_str();
    settings.WindowTitle = winTitle.c_str();
    settings.WindowPositionX = 0;
    settings.WindowPositionY = 0;
    settings.WindowStyle = ysWindow::WindowStyle::Windowed;
    settings.WindowWidth = 1920;
    settings.WindowHeight = 1080;

    m_engine.CreateGameWindow(settings);

    m_engine.GetDevice()->CreateSubRenderTarget(
        &m_mainRenderTarget,
        m_engine.GetScreenRenderTarget(),
        0,
        0,
        0,
        0);

    m_engine.InitializeShaderSet(&m_shaderSet);
    m_shaders.Initialize(
        &m_shaderSet,
        m_mainRenderTarget,
        m_engine.GetScreenRenderTarget(),
        m_engine.GetDefaultShaderProgram(),
        m_engine.GetDefaultInputLayout());
    m_engine.InitializeConsoleShaders(&m_shaderSet);
    m_engine.SetShaderSet(&m_shaderSet);

    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));

    m_assetManager.SetEngine(&m_engine);

    m_engine.GetDevice()->CreateIndexBuffer(
        &m_geometryIndexBuffer, sizeof(unsigned short) * 200000, nullptr);
    m_engine.GetDevice()->CreateVertexBuffer(
        &m_geometryVertexBuffer, sizeof(dbasic::Vertex) * 100000, nullptr);

    m_geometryGenerator.initialize(100000, 200000);

    initialize();
}

void EngineSimApplication::initialize() {
    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));
    m_assetManager.CompileInterchangeFile((m_assetPath + "/assets").c_str(), 1.0f, true);
    m_assetManager.LoadSceneFile((m_assetPath + "/assets").c_str(), true);

    m_textRenderer.SetEngine(&m_engine);
    m_textRenderer.SetRenderer(m_engine.GetUiRenderer());
    m_textRenderer.SetFont(m_engine.GetConsole()->GetFont());

    // Start with no engine loaded -- use the "LOAD ENGINE" button to pick one.
    refreshUserInterface();

    m_audioBuffer.initialize(44100, 44100);
    m_audioBuffer.m_writePointer = (int)(44100 * 0.1);

    ysAudioParameters params;
    params.m_bitsPerSample = 16;
    params.m_channelCount = 1;
    params.m_sampleRate = 44100;
    m_outputAudioBuffer =
        m_engine.GetAudioDevice()->CreateBuffer(&params, 44100);

    m_audioSource = m_engine.GetAudioDevice()->CreateSource(m_outputAudioBuffer);
    m_audioSource->SetMode((m_simulator != nullptr && m_simulator->getEngine() != nullptr)
        ? ysAudioSource::Mode::Loop
        : ysAudioSource::Mode::Stop);
    m_audioSource->SetPan(0.0f);
    m_audioSource->SetVolume(1.0f);

#ifdef ATG_ENGINE_SIM_DISCORD_ENABLED
    // Create a global instance of discord-rpc
    CDiscord::CreateInstance();

    // Enable it, this needs to be set via a config file of some sort. 
    GetDiscordManager()->SetUseDiscord(true);
    DiscordRichPresence passMe = { 0 };

    std::string engineName = (m_iceEngine != nullptr)
        ? m_iceEngine->getName()
        : "Broken Engine";

    GetDiscordManager()->SetStatus(passMe, engineName, s_buildVersion);
#endif /* ATG_ENGINE_SIM_DISCORD_ENABLED */
}

void EngineSimApplication::process(float frame_dt) {
    if (m_simulator == nullptr) return;

    frame_dt = static_cast<float>(clamp(frame_dt, 1 / 200.0f, 1 / 30.0f));

    double speed = 1.0 / 1.0;
    if (m_keyBindings.isDown(InputAction::SimSpeed1, &m_engine)) {
        speed = 1 / 10.0;
    }
    else if (m_keyBindings.isDown(InputAction::SimSpeed2, &m_engine)) {
        speed = 1 / 100.0;
    }
    else if (m_keyBindings.isDown(InputAction::SimSpeed3, &m_engine)) {
        speed = 1 / 200.0;
    }
    else if (m_keyBindings.isDown(InputAction::SimSpeed4, &m_engine)) {
        speed = 1 / 500.0;
    }
    else if (m_keyBindings.isDown(InputAction::SimSpeed5, &m_engine)) {
        speed = 1 / 1000.0;
    }

    if (m_keyBindings.isDown(InputAction::DisplayAngleIncrease, &m_engine)) {
        m_displayAngle += frame_dt * 1.0f;
    }
    else if (m_keyBindings.isDown(InputAction::DisplayAngleDecrease, &m_engine)) {
        m_displayAngle -= frame_dt * 1.0f;
    }
    else if (m_keyBindings.pressed(InputAction::DisplayAngleReset, &m_engine)) {
        m_displayAngle = 0.0f;
    }

    m_simulator->setSimulationSpeed(speed);

    const double avgFramerate = clamp(m_engine.GetAverageFramerate(), 30.0f, 1000.0f);

    if (m_autoTuneSimFrequency) {
        // ponytail: bang-bang control, not a PID loop. Climb by default --
        // with vsync on, avgFramerate sits right AT targetFramerate (never
        // above it) whenever there's still headroom, so "only climb when
        // strictly above target" would never fire and the frequency would
        // never leave the floor. Only back off once it's clearly below
        // target, i.e. the machine actually can't keep up. targetFramerate
        // should match your display's refresh/vsync cap; 240 is just this
        // dev's monitor.
        //
        // Left to run forever, this oscillates around the machine's real
        // equilibrium -- and since sim frequency feeds the synthesizer's
        // input sample rate, that oscillation is audible as constant pitch
        // jitter. So: bracket the oscillation once (record the high and low
        // turning points) and settle at the midpoint instead of continuing
        // to hunt. Only resume hunting if framerate falls off a cliff
        // (stallFramerateDrop) -- a normal marginal dip shouldn't undo the
        // peg, only something that actually changed the machine's budget
        // (window resize, scene load, alt-tab, etc).
        //
        // Turning points aren't trusted until huntWarmupTime has elapsed:
        // right after a fresh search starts (e.g. scene load just reset the
        // frequency to the floor), a loading stutter can drag avgFramerate
        // down for a frame or two, which would otherwise be misread as an
        // immediate high-then-low bracket sitting right at the floor,
        // pegging before it ever got to actually hunt.
        constexpr double targetFramerate = 240.0;
        constexpr double framerateMargin = 30.0;
        constexpr double simFrequencyRampRate = 5000.0; // Hz of sim frequency per second
        constexpr double minSimFrequency = 7000.0;
        constexpr double stallFramerateDrop = 100.0;
        constexpr double huntWarmupTime = 2.0; // seconds

        const bool climbing = avgFramerate >= targetFramerate - framerateMargin;

        if (m_simFrequencyPegged && avgFramerate < targetFramerate - stallFramerateDrop) {
            m_simFrequencyPegged = false;
            m_simFrequencyHigh = -1.0;
            m_simFrequencyLow = -1.0;
            m_simFrequencySearchTime = 0.0;
            m_simFrequencyRampingUp = climbing;
        }

        if (!m_simFrequencyPegged) {
            m_simFrequencySearchTime += frame_dt;

            if (climbing != m_simFrequencyRampingUp) {
                // Direction just reversed -- that's one edge of the band,
                // but only trust it once warm-up has elapsed.
                if (m_simFrequencySearchTime >= huntWarmupTime) {
                    const double turningPoint = m_simulator->getSimulationFrequency();
                    if (m_simFrequencyRampingUp) {
                        m_simFrequencyHigh = turningPoint;
                    }
                    else {
                        m_simFrequencyLow = turningPoint;
                    }
                }

                m_simFrequencyRampingUp = climbing;
            }

            if (m_simFrequencyHigh > 0.0 && m_simFrequencyLow > 0.0) {
                const double pegFrequency = clamp(
                    (m_simFrequencyHigh + m_simFrequencyLow) * 0.5,
                    minSimFrequency, 400000.0);
                m_simulator->setSimulationFrequency(pegFrequency);
                m_simFrequencyPegged = true;

                if (!m_currentEnginePath.empty()) {
                    m_simFrequencyCache.set(m_currentEnginePath, pegFrequency);
                }
            }
            else {
                double newFrequency = m_simulator->getSimulationFrequency();
                newFrequency += climbing
                    ? simFrequencyRampRate * frame_dt
                    : -simFrequencyRampRate * frame_dt;

                m_simulator->setSimulationFrequency(clamp(newFrequency, minSimFrequency, 400000.0));
            }
        }
    }

    m_simulator->startFrame(1 / avgFramerate);

    auto proc_t0 = std::chrono::steady_clock::now();
    const int iterationCount = m_simulator->getFrameIterationCount();
    while (m_simulator->simulateStep()) {
        m_oscCluster->sample();
    }

    auto proc_t1 = std::chrono::steady_clock::now();

    m_simulator->endFrame();

    auto duration = proc_t1 - proc_t0;
    if (iterationCount > 0) {
        m_performanceCluster->addTimePerTimestepSample(
            (duration.count() / 1E9) / iterationCount);
    }

    const SampleOffset safeWritePosition = m_audioSource->GetCurrentWritePosition();
    const SampleOffset writePosition = m_audioBuffer.m_writePointer;

    SampleOffset targetWritePosition =
        m_audioBuffer.getBufferIndex(safeWritePosition, (int)(44100 * 0.1));
    SampleOffset maxWrite = m_audioBuffer.offsetDelta(writePosition, targetWritePosition);

    SampleOffset currentLead = m_audioBuffer.offsetDelta(safeWritePosition, writePosition);
    SampleOffset newLead = m_audioBuffer.offsetDelta(safeWritePosition, targetWritePosition);

    if (currentLead > 44100 * 0.5) {
        m_audioBuffer.m_writePointer = m_audioBuffer.getBufferIndex(safeWritePosition, (int)(44100 * 0.05));
        currentLead = m_audioBuffer.offsetDelta(safeWritePosition, m_audioBuffer.m_writePointer);
        maxWrite = m_audioBuffer.offsetDelta(m_audioBuffer.m_writePointer, targetWritePosition);
    }

    if (currentLead > newLead) {
        maxWrite = 0;
    }

    int16_t *samples = new int16_t[maxWrite];
    const int readSamples = m_simulator->readAudioOutput(maxWrite, samples);

    updateTurbineWhineParameters((double)frame_dt);
    updateSuperchargerWhineParameters((double)frame_dt);

    for (SampleOffset i = 0; i < (SampleOffset)readSamples && i < maxWrite; ++i) {
        const int mixedSample = clamp(
            samples[i] + sampleTurbineWhine() + sampleSuperchargerWhine(),
            (int)INT16_MIN, (int)INT16_MAX);
        const int16_t sample = (int16_t)mixedSample;

        if (m_oscillatorSampleOffset % 4 == 0) {
            m_oscCluster->getAudioWaveformOscilloscope()->addDataPoint(
                m_oscillatorSampleOffset,
                sample / (float)(INT16_MAX));
        }

        m_audioBuffer.writeSample(sample, m_audioBuffer.m_writePointer, (int)i);

        m_oscillatorSampleOffset = (m_oscillatorSampleOffset + 1) % (44100 / 10);
    }

    delete[] samples;

    if (readSamples > 0) {
        SampleOffset size0, size1;
        void *data0, *data1;
        m_audioSource->LockBufferSegment(
            m_audioBuffer.m_writePointer, readSamples, &data0, &size0, &data1, &size1);

        m_audioBuffer.copyBuffer(
            reinterpret_cast<int16_t *>(data0), m_audioBuffer.m_writePointer, size0);
        m_audioBuffer.copyBuffer(
            reinterpret_cast<int16_t *>(data1),
            m_audioBuffer.getBufferIndex(m_audioBuffer.m_writePointer, size0),
            size1);

        m_audioSource->UnlockBufferSegments(data0, size0, data1, size1);
        m_audioBuffer.commitBlock(readSamples);
    }

    m_performanceCluster->addInputBufferUsageSample(
        (double)m_simulator->getSynthesizerInputLatency() / m_simulator->getSynthesizerInputLatencyTarget());
    m_performanceCluster->addAudioLatencySample(
        m_audioBuffer.offsetDelta(m_audioSource->GetCurrentWritePosition(), m_audioBuffer.m_writePointer) / (44100 * 0.1));
}

void EngineSimApplication::updateTurbineWhineParameters(double dt) {
    const int turboCount = (m_iceEngine != nullptr) ? m_iceEngine->getTurbochargerCount() : 0;

    m_turbineWhinePhase.resize(turboCount, 0.0);
    m_turbineWhineFrequency.resize(turboCount, 0.0);
    m_turbineWhineAmplitude.resize(turboCount, 0.0);
    m_flutterPhase.resize(turboCount, 0.0);
    m_flutterAmplitude.resize(turboCount, 0.0);
    m_flutterGate.resize(turboCount, 0.0);
    m_flutterIntensity.resize(turboCount, 0.0);
    m_flutterRate.resize(turboCount, 22.0);
    m_flutterBurstGain.resize(turboCount, 1.0);
    m_flutterEnvelope.resize(turboCount, 0.0);
    m_flutterFilterState.resize(turboCount, 0.0);
    m_bovSampleData.resize(turboCount);
    m_bovSampleRate.resize(turboCount, 44100.0);
    m_bovVolume.resize(turboCount, 1.0);
    m_bovPlaybackPosition.resize(turboCount, 0.0);
    m_bovPlaying.resize(turboCount, false);
    m_bovVolumeScale.resize(turboCount, 1.0);
    m_bovPeakBoost.resize(turboCount, 0.0);

    // ponytail: blade-pass whine (fundamental + one harmonic). Flutter is
    // gated by Turbocharger::isSurging() -- a real backflow event from the
    // Moore-Greitzer compressor characteristic (see turbocharger.cpp), not
    // a throttle/boost heuristic -- so it only sounds when the physics
    // actually surged. Chatter rate/duty/loudness are still re-randomized
    // every chuff and scaled by excess boost (see sampleTurbineWhine), and
    // the gate/noise are smoothed so it doesn't sound like a metronome or a
    // flat hiss.
    //
    // isSurging() is instantaneous (this substep's flow sign, nothing
    // more) -- a real BOV doesn't snap open the moment flow direction
    // flips, and won't be doing anything at all with no real boost behind
    // it (e.g. residual charge-pipe pressure bleeding back through a
    // stopped compressor at 0 RPM/engine-off was audible as a phantom
    // chuff). Require actual excess boost before counting it as a surge
    // worth making noise over, and fade the gate in/out instead of
    // snapping it, so the flutter tracks how hard the compressor is
    // surging rather than acting like a switch.
    constexpr double bladeCount = 9.0;
    constexpr double maxAudibleFrequency = 16000.0;
    const double referenceSpeed = units::rpm(150000.0);
    constexpr double maxAmplitude = 3500.0;

    constexpr double maxFlutterAmplitude = 8000.0;
    // Only used to scale *intensity* now (see below) -- just above
    // atmospheric, not the old hard gate threshold.
    constexpr double flutterPressureRatioThreshold = 1.02;
    constexpr double flutterGateSmoothing = 0.4;
    constexpr double flutterBaseGate = 0.7;

    // A real BOV/dump valve fires once, on the driver actually lifting off
    // the throttle, releasing whatever boost is trapped in the charge pipe
    // at that instant -- not on the compressor surging (that's a distinct
    // backflow phenomenon, already covered by the flutter chuff above) and
    // not on merely reaching the wastegate's max_boost setpoint while still
    // on-throttle (the wastegate bleeds exhaust around the turbine; nothing
    // audible happens on the intake side from that alone). So this is
    // gated purely on a throttle-closing edge, independent of the flutter
    // gate/isSurging() logic above.
    constexpr double throttleClosedThreshold = 0.9; // getThrottle(): 0 = open, 1 = closed
    constexpr double throttleWasOpenThreshold = 0.5;
    const double minBoostForBov = units::pressure(1.0, units::psi);
    // Real-time decay rate for the peak-boost hold below -- ~0.4s to fall
    // from a generous 30psi down through the trigger threshold, comfortably
    // longer than one video frame's worth of physics substeps.
    const double peakBoostDecayPerSecond = units::pressure(30.0, units::psi) / 0.4;
    const double throttle = (m_iceEngine != nullptr) ? m_iceEngine->getThrottle() : 1.0;

    // Latch "was open" the instant we see it, and only consume the latch
    // (firing the edge) once the throttle actually lands above the closed
    // threshold -- however many frames that takes. See m_bovThrottleWasOpen.
    if (throttle < throttleWasOpenThreshold) {
        m_bovThrottleWasOpen = true;
    }

    const bool liftedOff = m_bovThrottleWasOpen && throttle >= throttleClosedThreshold;
    if (liftedOff) {
        m_bovThrottleWasOpen = false;
    }

    for (int i = 0; i < turboCount; ++i) {
        Turbocharger *turbo = m_iceEngine->getTurbocharger(i);

        // A closed throttle's backflow/surge response plays out over many
        // physics substeps within this one video frame, so the raw
        // instantaneous getBoostPressure() below can already have
        // collapsed back toward zero by the time this (once-per-frame)
        // code runs -- hold the recent peak instead of trusting the
        // instantaneous reading for the lift-off check.
        const double decayedPeakBoost = m_bovPeakBoost[i] - peakBoostDecayPerSecond * dt;
        const double currentBoost = turbo->getBoostPressure();
        m_bovPeakBoost[i] = (currentBoost > decayedPeakBoost) ? currentBoost : decayedPeakBoost;

        const bool hasBovSample = i < (int)m_bovSampleData.size() && !m_bovSampleData[i].empty();
        if (hasBovSample && liftedOff && m_bovPeakBoost[i] > minBoostForBov) {
            // Scale this shot's loudness by how much boost was actually
            // trapped, relative to this turbo's own wastegate setpoint --
            // barely-over-threshold trigger gets a quiet chirp
            // (minVolumeScale floor, not silence), a full-boost lift cracks
            // at the sample's full authored volume.
            constexpr double minVolumeScale = 0.2;
            const double maxBoostForScale = turbo->getParameters().maxBoost;
            const double boostRange = maxBoostForScale - minBoostForBov;
            const double boostFrac = (boostRange > 0.0)
                ? clamp((m_bovPeakBoost[i] - minBoostForBov) / boostRange, 0.0, 1.0)
                : 1.0;
            m_bovVolumeScale[i] = minVolumeScale + (1.0 - minVolumeScale) * boostFrac;

            m_bovPlaybackPosition[i] = 0.0;
            m_bovPlaying[i] = true;
            m_bovPeakBoost[i] = 0.0;
        }

        const double shaftFrequency = turbo->getShaftSpeed() / (2.0 * constants::pi);

        m_turbineWhineFrequency[i] = clamp(shaftFrequency * bladeCount, 0.0, maxAudibleFrequency);

        const double speedNorm = clamp(turbo->getShaftSpeed() / referenceSpeed, 0.0, 1.0);
        m_turbineWhineAmplitude[i] = maxAmplitude * speedNorm * speedNorm;

        const double excessBoost =
            clamp(turbo->getPressureRatio() - flutterPressureRatioThreshold, 0.0, 1.0);

        // Gate purely on the real surge signal -- requiring excess boost on
        // top of isSurging() choked the sound off almost immediately, since
        // surging is exactly what drains the trapped boost that condition
        // was waiting on. A real compressor keeps chattering audibly well
        // after boost has mostly bled away; excess boost now only makes it
        // louder/busier (see sampleTurbineWhine()), never silences it.
        const double gateTarget = turbo->isSurging()
            ? clamp(flutterBaseGate + excessBoost, 0.0, 1.0)
            : 0.0;
        m_flutterGate[i] += (gateTarget - m_flutterGate[i]) * flutterGateSmoothing;
        m_flutterAmplitude[i] = maxFlutterAmplitude * m_flutterGate[i];
        m_flutterIntensity[i] = excessBoost;
    }

}

int EngineSimApplication::sampleTurbineWhine() {
    constexpr double dtPerSample = 1.0 / 44100.0;
    constexpr double twoPi = 2.0 * constants::pi;

    // Noise filter smoothing (one-pole, per-sample) -- gives each chuff some
    // throaty body instead of sounding like flat white hiss.
    constexpr double noiseFilterCoeff = 0.15;

    // Per-sample multiplicative decay for each chuff's envelope -- ~12ms to
    // decay to 1% of its peak. Each chuff is retriggered to 1.0 (instant
    // attack, see below) and left to decay on its own from there, so it
    // reads as a distinct percussive pop with real silence between chuffs
    // instead of the previous thresholded on/off gate, which snapped
    // between two noise levels roughly every other sample (envelope
    // coefficient 0.4 at 44.1kHz) -- audibly a harsh, clicky buzz rather
    // than a stutter.
    constexpr double flutterDecayPerSample = 0.9913;

    double total = 0.0;
    for (size_t i = 0; i < m_turbineWhinePhase.size(); ++i) {
        m_turbineWhinePhase[i] += twoPi * m_turbineWhineFrequency[i] * dtPerSample;
        if (m_turbineWhinePhase[i] > twoPi) {
            m_turbineWhinePhase[i] -= twoPi;
        }

        total += m_turbineWhineAmplitude[i] * (
            0.8 * std::sin(m_turbineWhinePhase[i])
            + 0.2 * std::sin(2.0 * m_turbineWhinePhase[i]));

        const bool gateOpen = m_flutterAmplitude[i] > 1.0;
        const bool hasBovSample = i < m_bovSampleData.size() && !m_bovSampleData[i].empty();

        if (hasBovSample) {
            // One-shot recorded bov_sound playback, triggered on an actual
            // throttle lift in updateTurbineWhineParameters() -- plays
            // through once and stops (no looping, no re-triggering off
            // gateOpen/isSurging()), in place of the synthesized chuff
            // below.
            if (m_bovPlaying[i]) {
                const std::vector<int16_t> &sample = m_bovSampleData[i];
                const size_t index = (size_t)m_bovPlaybackPosition[i];

                total += sample[index] * m_bovVolume[i] * m_bovVolumeScale[i];

                const double increment = m_bovSampleRate[i] * dtPerSample;
                m_bovPlaybackPosition[i] += increment;
                if (m_bovPlaybackPosition[i] >= (double)sample.size()) {
                    m_bovPlaying[i] = false;
                }
            }
        }
        else if (gateOpen) {
            // Harder boost overrun means a busier, louder chatter -- not
            // just the same chuff pattern scaled by a volume knob.
            const double intensity = m_flutterIntensity[i];
            const double rateMin = 12.0 + 10.0 * intensity;
            const double rateMax = 22.0 + 26.0 * intensity;

            m_flutterPhase[i] += twoPi * m_flutterRate[i] * dtPerSample;
            if (m_flutterPhase[i] > twoPi) {
                m_flutterPhase[i] -= twoPi;

                // Re-roll rate and loudness every chuff so neither the
                // timing nor the volume repeats like a loop, and retrigger
                // the envelope at full level -- instant attack, then decays
                // on its own below.
                m_flutterRate[i] = rateMin + ((double)rand() / RAND_MAX) * (rateMax - rateMin);
                m_flutterBurstGain[i] = 0.5 + ((double)rand() / RAND_MAX) * 0.5;
                m_flutterEnvelope[i] = 1.0;
            }
            else {
                m_flutterEnvelope[i] *= flutterDecayPerSample;
            }

            const double noise = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            m_flutterFilterState[i] += (noise - m_flutterFilterState[i]) * noiseFilterCoeff;

            total += m_flutterAmplitude[i] * m_flutterBurstGain[i]
                * m_flutterEnvelope[i] * m_flutterFilterState[i];
        }

    }

    return (int)total;
}

void EngineSimApplication::updateSuperchargerWhineParameters(double dt) {
    const int scCount = (m_iceEngine != nullptr) ? m_iceEngine->getSuperchargerCount() : 0;

    m_superchargerWhinePhase.resize(scCount, 0.0);
    m_superchargerWhineFrequency.resize(scCount, 0.0);
    m_superchargerWhineAmplitude.resize(scCount, 0.0);

    constexpr double maxAudibleFrequency = 16000.0;
    constexpr double maxAmplitude = 3200.0;

    // PD (Roots/twin-screw) blowers are geared far slower than a
    // centrifugal impeller or turbo turbine -- this asset pack's own
    // supercharged engines (e.g. ford/dct/gtd_predator.mr,
    // dodge/at/demon_170.mr) land their PD blower shaft speed around
    // 20-25krpm at redline (drive_ratio * redline), so that's the
    // reference this normalizes against. A centrifugal unit instead uses
    // its own max_impeller_speed rating directly, since that can
    // legitimately run much higher (50-60krpm+).
    const double pdReferenceSpeed = units::rpm(22000.0);

    for (int i = 0; i < scCount; ++i) {
        Supercharger *supercharger = m_iceEngine->getSupercharger(i);
        const Supercharger::Parameters &params = supercharger->getParameters();

        // Lobe/blade-pass count sets the whine's pitch relative to shaft
        // speed -- a 4-lobe Roots blower (Eaton TVS, the real unit most of
        // this asset pack's superchargers are modeled on) reads as a
        // deeper, throatier tone than a many-bladed centrifugal impeller
        // at the same shaft RPM.
        double lobeCount;
        double referenceSpeed;
        switch (params.type) {
            case Supercharger::Type::TwinScrew:
                lobeCount = 3.0;
                referenceSpeed = pdReferenceSpeed;
                break;
            case Supercharger::Type::Centrifugal:
                lobeCount = 7.0;
                referenceSpeed = (params.maxImpellerSpeed > 1.0) ? params.maxImpellerSpeed : 1.0;
                break;
            case Supercharger::Type::Roots:
            default:
                lobeCount = 4.0;
                referenceSpeed = pdReferenceSpeed;
                break;
        }

        // A clutched, currently-disengaged blower's shaft speed is
        // already 0 here (Supercharger::process() stops driving it when
        // declutched) -- this silences the whine on a twincharger's
        // blower-to-turbo handoff for free, no extra gating needed.
        const double shaftSpeed = supercharger->getShaftSpeed();
        const double shaftFrequency = shaftSpeed / (2.0 * constants::pi);

        m_superchargerWhineFrequency[i] = clamp(shaftFrequency * lobeCount, 0.0, maxAudibleFrequency);

        // Smoothed (not snapped straight to target) specifically for a
        // clutched supercharger: engaging/disengaging changes shaftSpeed
        // by a large step within a single video frame, and jumping
        // amplitude to match instantly would click. An unclutched
        // (always-driven) blower's speed only ever changes gradually with
        // crank RPM anyway, so this is inaudible there.
        const double speedNorm = clamp(shaftSpeed / referenceSpeed, 0.0, 1.0);
        const double targetAmplitude = maxAmplitude * speedNorm * speedNorm;
        constexpr double amplitudeSmoothingRate = 15.0; // ~1/15s time constant
        const double smoothing = clamp(amplitudeSmoothingRate * dt, 0.0, 1.0);
        m_superchargerWhineAmplitude[i] += (targetAmplitude - m_superchargerWhineAmplitude[i]) * smoothing;
    }
}

int EngineSimApplication::sampleSuperchargerWhine() {
    constexpr double dtPerSample = 1.0 / 44100.0;
    constexpr double twoPi = 2.0 * constants::pi;

    double total = 0.0;
    for (size_t i = 0; i < m_superchargerWhinePhase.size(); ++i) {
        m_superchargerWhinePhase[i] += twoPi * m_superchargerWhineFrequency[i] * dtPerSample;
        if (m_superchargerWhinePhase[i] > twoPi) {
            m_superchargerWhinePhase[i] -= twoPi;
        }

        // A belt-driven blower's whine reads as rounder/throatier than a
        // turbo's -- a touch of third-harmonic content instead of the
        // turbine whine's second harmonic.
        total += m_superchargerWhineAmplitude[i] * (
            0.85 * std::sin(m_superchargerWhinePhase[i])
            + 0.15 * std::sin(3.0 * m_superchargerWhinePhase[i]));
    }

    return (int)total;
}

void EngineSimApplication::render() {
    for (SimulationObject *object : m_objects) {
        object->generateGeometry();
    }

    m_viewParameters.Sublayer = 0;
    for (SimulationObject *object : m_objects) {
        object->render(&getViewParameters());
    }

    m_viewParameters.Sublayer = 1;
    for (SimulationObject *object : m_objects) {
        object->render(&getViewParameters());
    }

    m_viewParameters.Sublayer = 2;
    for (SimulationObject *object : m_objects) {
        object->render(&getViewParameters());
    }

    m_uiManager.render();
}

float EngineSimApplication::pixelsToUnits(float pixels) const {
    const float f = m_displayHeight / m_engineView->m_bounds.height();
    return pixels * f;
}

float EngineSimApplication::unitsToPixels(float units) const {
    const float f = m_engineView->m_bounds.height() / m_displayHeight;
    return units * f;
}

void EngineSimApplication::run() {
    while (true) {
        m_engine.StartFrame();

        if (!m_engine.IsOpen()) break;

        // The settings overlay hardcodes Escape as "cancel capture" / "close
        // panel" rather than routing it through the (remappable) Quit
        // action -- otherwise a user who rebinds Quit away from Escape would
        // lose the conventional dialog-dismiss key, and one still bound to
        // Escape would quit the app while trying to cancel a key capture.
        bool escapeConsumedByOverlay = false;
        const bool settingsOpen = (m_settingsCluster != nullptr && m_settingsCluster->isVisible());
        if (settingsOpen && m_engine.ProcessKeyDown(ysKey::Code::Escape)) {
            if (m_settingsCluster->isCapturing()) {
                m_settingsCluster->cancelCapture();
            }
            else {
                m_settingsCluster->close();
            }
            escapeConsumedByOverlay = true;
        }

        if (!escapeConsumedByOverlay && m_keyBindings.pressed(InputAction::Quit, &m_engine)) {
            break;
        }

        // While the settings panel is open, world/view controls are
        // suspended entirely -- otherwise pressing a gameplay key to
        // capture a new binding would also drive the engine (throttle,
        // gear shifts, etc.) in the background.
        if (!settingsOpen) {
            if (m_keyBindings.pressed(InputAction::ReloadScript, &m_engine)) {
                reloadSimulation();
            }

            if (m_keyBindings.pressed(InputAction::ScreenCycle, &m_engine)) {
                m_dynoScreenVisible = false;
                int next = (int)m_screen + 1;
                if (next > (int)ScreenId::Cinematic) next = (int)ScreenId::Main;
                m_screen = (ScreenId)next;
            }

            if (m_keyBindings.pressed(InputAction::TurboEditorToggle, &m_engine)) {
                m_turboEditorVisible = !m_turboEditorVisible;
                if (m_turboEditorCluster != nullptr) {
                    m_turboEditorCluster->setVisible(m_turboEditorVisible);
                }
            }

            if (m_keyBindings.pressed(InputAction::Fullscreen, &m_engine)) {
                if (m_engine.GetGameWindow()->GetWindowStyle() != ysWindow::WindowStyle::Fullscreen) {
                    m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Fullscreen);
                    m_infoCluster->setLogMessage("Entered fullscreen mode");
                }
                else {
                    m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Windowed);
                    m_infoCluster->setLogMessage("Exited fullscreen mode");
                }
            }
        }

        m_gameWindowHeight = m_engine.GetGameWindow()->GetGameHeight();
        m_screenHeight = m_engine.GetGameWindow()->GetScreenHeight();
        m_screenWidth = m_engine.GetGameWindow()->GetScreenWidth();

        updateScreenSizeStability();

        if (!settingsOpen) {
            processEngineInput();

            if (m_keyBindings.pressed(InputAction::RecordToggle, &m_engine) &&
                m_engine.GetGameWindow()->IsActive()) {
                if (!isRecording() && readyToRecord()) {
                    startRecording();
                }
                else if (isRecording()) {
                    stopRecording();
                }
            }
        }

        if (isRecording() && !readyToRecord()) {
            stopRecording();
        }

        const bool singleStepPressed = !settingsOpen && m_keyBindings.pressed(InputAction::SingleStep, &m_engine);
        if (!m_paused || singleStepPressed) {
            process(m_engine.GetFrameLength());
        }

        m_uiManager.update(m_engine.GetFrameLength());

        // Handled here, after m_uiManager.update() has fully returned, since
        // both of these destroy and rebuild the UI tree -- doing that from
        // inside a button's own click callback would free the button while
        // it's still executing.
        if (m_reloadRequested) {
            m_reloadRequested = false;
            reloadSimulation();
        }
        if (m_loadDialogRequested) {
            m_loadDialogRequested = false;
            loadSimulationFromDialog();
        }

        renderScene();

        m_engine.EndFrame();

        if (isRecording()) {
            recordFrame();
        }
    }

    if (isRecording()) {
        stopRecording();
    }

    if (m_simulator != nullptr) {
        m_simulator->endAudioRenderingThread();
    }
}

void EngineSimApplication::destroy() {
    m_keyBindings.saveToDisk(m_keyBindingsPath);

    m_shaderSet.Destroy();

    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryVertexBuffer);
    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryIndexBuffer);

    m_assetManager.Destroy();
    m_engine.Destroy();

    if (m_simulator != nullptr) {
        m_simulator->destroy();
    }
    m_audioBuffer.destroy();
}

void EngineSimApplication::loadEngine(
    Engine *engine,
    Vehicle *vehicle,
    Transmission *transmission)
{
    destroyObjects();

    if (m_simulator != nullptr) {
        m_simulator->releaseSimulation();
        delete m_simulator;
    }

    if (m_vehicle != nullptr) {
        delete m_vehicle;
        m_vehicle = nullptr;
    }

    if (m_transmission != nullptr) {
        delete m_transmission;
        m_transmission = nullptr;
    }

    if (m_iceEngine != nullptr) {
        m_iceEngine->destroy();
        delete m_iceEngine;
    }

    if (engine == nullptr || vehicle == nullptr || transmission == nullptr) {
        m_iceEngine = nullptr;
        m_vehicle = nullptr;
        m_transmission = nullptr;
        m_simulator = nullptr;
        m_viewParameters.Layer1 = 0;

        return;
    }

    m_iceEngine = engine;
    m_vehicle = vehicle;
    m_transmission = transmission;

    m_simulator = engine->createSimulator(vehicle, transmission);

    createObjects(engine);

    m_viewParameters.Layer1 = engine->getMaxDepth();
    engine->calculateDisplacement();

    // Auto-tuned in process() instead of trusting the .mr file's
    // simulation_frequency -- start at the baseline and let it climb to
    // whatever this machine can sustain at targetFramerate. If we've
    // already found that answer for this engine on this machine, skip the
    // hunt and reuse it directly (process() will still fall back to
    // re-hunting if framerate turns out not to hold at that frequency).
    double cachedFrequency = 0.0;
    const bool haveCachedFrequency =
        m_simFrequencyCache.get(m_currentEnginePath, &cachedFrequency);

    m_simulator->setSimulationFrequency(haveCachedFrequency ? cachedFrequency : 7000.0);
    m_autoTuneSimFrequency = true;
    m_simFrequencyPegged = haveCachedFrequency;
    m_simFrequencyRampingUp = true;
    m_simFrequencyHigh = -1.0;
    m_simFrequencyLow = -1.0;
    m_simFrequencySearchTime = 0.0;

    Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
    audioParams.inputSampleNoise = static_cast<float>(engine->getInitialJitter());
    audioParams.airNoise = static_cast<float>(engine->getInitialNoise());
    audioParams.dF_F_mix = static_cast<float>(engine->getInitialHighFrequencyGain());
    m_simulator->synthesizer().setAudioParameters(audioParams);

    const int exhaustSystemCount = engine->getExhaustSystemCount();
    for (int i = 0; i < exhaustSystemCount; ++i) {
        ImpulseResponse *response = engine->getExhaustSystem(i)->getImpulseResponse();

        ysWindowsAudioWaveFile waveFile;
        waveFile.OpenFile(response->getFilename().c_str());
        waveFile.InitializeInternalBuffer(waveFile.GetSampleCount());
        waveFile.FillBuffer(0);
        waveFile.CloseFile();

        m_simulator->synthesizer().initializeImpulseResponse(
            reinterpret_cast<const int16_t *>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            response->getVolume(),
            i
        );

        waveFile.DestroyInternalBuffer();
    }

    // Intake-noise channels, appended after the exhaust channels above --
    // see PistonEngineSimulator::writeToSynthesizer() for the matching
    // channel-index layout.
    for (int i = 0; i < engine->getIntakeCount(); ++i) {
        ImpulseResponse *response = engine->getIntake(i)->getImpulseResponse();
        if (response == nullptr) continue;

        ysWindowsAudioWaveFile waveFile;
        waveFile.OpenFile(response->getFilename().c_str());
        waveFile.InitializeInternalBuffer(waveFile.GetSampleCount());
        waveFile.FillBuffer(0);
        waveFile.CloseFile();

        m_simulator->synthesizer().initializeImpulseResponse(
            reinterpret_cast<const int16_t *>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            response->getVolume(),
            exhaustSystemCount + i
        );

        waveFile.DestroyInternalBuffer();
    }

    loadBovSounds();

    m_simulator->startAudioRenderingThread();
}

void EngineSimApplication::loadBovSounds() {
    const int turboCount = (m_iceEngine != nullptr) ? m_iceEngine->getTurbochargerCount() : 0;

    m_bovSampleData.clear();
    m_bovSampleData.resize(turboCount);
    m_bovSampleRate.assign(turboCount, 44100.0);
    m_bovVolume.assign(turboCount, 1.0);
    m_bovPlaybackPosition.assign(turboCount, 0.0);
    m_bovPlaying.assign(turboCount, false);
    m_bovVolumeScale.assign(turboCount, 1.0);
    m_bovThrottleWasOpen = false;

    for (int i = 0; i < turboCount; ++i) {
        Turbocharger *turbo = m_iceEngine->getTurbocharger(i);
        BovSound *bov = (turbo != nullptr) ? turbo->getBovSound() : nullptr;
        if (bov == nullptr) continue;

        int channels = 0;
        int sampleRate = 0;
        short *decoded = nullptr;
        const int frameCount = stb_vorbis_decode_filename(
            bov->getFilename().c_str(), &channels, &sampleRate, &decoded);

        if (frameCount <= 0 || decoded == nullptr || channels <= 0) {
            if (decoded != nullptr) {
                free(decoded);
            }

            continue;
        }

        // Downmix to mono -- the flutter mixer below is a single running
        // total, same as the synthesized noise it replaces.
        m_bovSampleData[i].resize(frameCount);
        for (int s = 0; s < frameCount; ++s) {
            int sum = 0;
            for (int c = 0; c < channels; ++c) {
                sum += decoded[s * channels + c];
            }

            m_bovSampleData[i][s] = (int16_t)(sum / channels);
        }

        free(decoded);

        m_bovSampleRate[i] = (double)sampleRate;
        m_bovVolume[i] = bov->getVolume();
    }
}

void EngineSimApplication::drawGenerated(
    const GeometryGenerator::GeometryIndices &indices,
    int layer)
{
    drawGenerated(indices, layer, m_shaders.GetRegularFlags());
}

void EngineSimApplication::drawGeneratedUi(
    const GeometryGenerator::GeometryIndices &indices,
    int layer)
{
    drawGenerated(indices, layer, m_shaders.GetUiFlags());
}

void EngineSimApplication::drawGenerated(
    const GeometryGenerator::GeometryIndices &indices,
    int layer,
    dbasic::StageEnableFlags flags)
{
    m_engine.DrawGeneric(
        flags,
        m_geometryIndexBuffer,
        m_geometryVertexBuffer,
        sizeof(dbasic::Vertex),
        indices.BaseIndex,
        indices.BaseVertex,
        indices.FaceCount,
        false,
        layer);
}

void EngineSimApplication::configure(const ApplicationSettings &settings) {
    m_applicationSettings = settings;

    if (settings.startFullscreen) {
        m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Fullscreen);
    }

    m_background = ysColor::srgbiToLinear(m_applicationSettings.colorBackground);
    m_foreground = ysColor::srgbiToLinear(m_applicationSettings.colorForeground);
    m_shadow = ysColor::srgbiToLinear(m_applicationSettings.colorShadow);
    m_highlight1 = ysColor::srgbiToLinear(m_applicationSettings.colorHighlight1);
    m_highlight2 = ysColor::srgbiToLinear(m_applicationSettings.colorHighlight2);
    m_pink = ysColor::srgbiToLinear(m_applicationSettings.colorPink);
    m_red = ysColor::srgbiToLinear(m_applicationSettings.colorRed);
    m_orange = ysColor::srgbiToLinear(m_applicationSettings.colorOrange);
    m_yellow = ysColor::srgbiToLinear(m_applicationSettings.colorYellow);
    m_blue = ysColor::srgbiToLinear(m_applicationSettings.colorBlue);
    m_green = ysColor::srgbiToLinear(m_applicationSettings.colorGreen);
}

void EngineSimApplication::createObjects(Engine *engine) {
    for (int i = 0; i < engine->getCylinderCount(); ++i) {
        ConnectingRodObject *rodObject = new ConnectingRodObject;
        rodObject->initialize(this);
        rodObject->m_connectingRod = engine->getConnectingRod(i);
        m_objects.push_back(rodObject);

        PistonObject *pistonObject = new PistonObject;
        pistonObject->initialize(this);
        pistonObject->m_piston = engine->getPiston(i);
        m_objects.push_back(pistonObject);

        CombustionChamberObject *ccObject = new CombustionChamberObject;
        ccObject->initialize(this);
        ccObject->m_chamber = m_iceEngine->getChamber(i);
        m_objects.push_back(ccObject);
    }

    for (int i = 0; i < engine->getCrankshaftCount(); ++i) {
        CrankshaftObject *crankshaftObject = new CrankshaftObject;
        crankshaftObject->initialize(this);
        crankshaftObject->m_crankshaft = engine->getCrankshaft(i);
        m_objects.push_back(crankshaftObject);
    }

    for (int i = 0; i < engine->getCylinderBankCount(); ++i) {
        CylinderBankObject *cbObject = new CylinderBankObject;
        cbObject->initialize(this);
        cbObject->m_bank = engine->getCylinderBank(i);
        cbObject->m_head = engine->getHead(i);
        m_objects.push_back(cbObject);

        CylinderHeadObject *chObject = new CylinderHeadObject;
        chObject->initialize(this);
        chObject->m_head = engine->getHead(i);
        chObject->m_engine = engine;
        m_objects.push_back(chObject);
    }
}

void EngineSimApplication::destroyObjects() {
    for (SimulationObject *object : m_objects) {
        object->destroy();
        delete object;
    }

    m_objects.clear();
}

const SimulationObject::ViewParameters &
    EngineSimApplication::getViewParameters() const
{
    return m_viewParameters;
}

void EngineSimApplication::reloadSimulation() {
    if (m_targetScript.empty()) {
        if (m_infoCluster != nullptr) {
            m_infoCluster->setLogMessage("No engine loaded - use LOAD ENGINE");
        }
        return;
    }

    m_audioSource->SetMode(ysAudioSource::Mode::Stop);
    loadScript(m_targetScript);
    if (m_simulator != nullptr && m_simulator->getEngine() != nullptr) {
        m_audioSource->SetMode(ysAudioSource::Mode::Loop);
    }
}

void EngineSimApplication::loadSimulationFromDialog() {
    char fileBuffer[MAX_PATH] = {};

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Engine Simulator Scripts (*.mr)\0*.mr\0All Files\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = "../assets/engines";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        m_targetScript = fileBuffer;
        reloadSimulation();
    }
}

void EngineSimApplication::loadThemeFromDialog() {
    char fileBuffer[MAX_PATH] = {};

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Engine Simulator Scripts (*.mr)\0*.mr\0All Files\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = "../assets/themes";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        loadTheme(fileBuffer);
    }
}

void EngineSimApplication::toggleSettingsScreen() {
    if (m_settingsCluster == nullptr) return;

    if (m_settingsCluster->isVisible()) {
        m_settingsCluster->close();
    }
    else {
        m_settingsCluster->open();
    }
}

void EngineSimApplication::toggleDynoScreen() {
    m_dynoScreenVisible = !m_dynoScreenVisible;
}

void EngineSimApplication::loadTheme(const std::string &path) {
#ifdef ATG_ENGINE_SIM_PIRANHA_ENABLED
    es_script::Compiler compiler;
    compiler.initialize();
    if (compiler.compile(path)) {
        configure(compiler.execute().applicationSettings);
        if (m_infoCluster != nullptr) {
            m_infoCluster->setLogMessage("Loaded theme '" + path + "'");
        }
    }
    else if (m_infoCluster != nullptr) {
        m_infoCluster->setLogMessage("Failed to load theme '" + path + "'");
    }
    compiler.destroy();
#endif /* ATG_ENGINE_SIM_PIRANHA_ENABLED */
}

void EngineSimApplication::loadScript(const std::string &path) {
    Engine *engine = nullptr;
    Vehicle *vehicle = nullptr;
    Transmission *transmission = nullptr;
    std::string runtimeError;

    std::string wrapperPath;
    const bool usingWrapper = autoWireEngineScript(path, wrapperPath);
    const std::string &compilePath = usingWrapper ? wrapperPath : path;

#ifdef ATG_ENGINE_SIM_PIRANHA_ENABLED
    es_script::Compiler compiler;
    compiler.initialize();
    const bool compiled = compiler.compile(compilePath);
    if (usingWrapper) {
        std::remove(wrapperPath.c_str());
    }
    if (compiled) {
        const es_script::Compiler::Output output = compiler.execute();
        configure(output.applicationSettings);

        engine = output.engine;
        vehicle = output.vehicle;
        transmission = output.transmission;
        runtimeError = compiler.getRuntimeError();
    }
    else {
        engine = nullptr;
        vehicle = nullptr;
        transmission = nullptr;
    }

    compiler.destroy();
#endif /* ATG_ENGINE_SIM_PIRANHA_ENABLED */

    if (engine == nullptr && m_iceEngine != nullptr) {
        // Nothing usable came out of this script (compile error, or a script
        // that doesn't define an engine) -- keep whatever was already loaded
        // instead of tearing down a working simulation. On first startup
        // there's nothing loaded yet, so fall through to the defaults below.
        m_infoCluster->setLogMessage(
            "Failed to load '" + path + "' - no engine defined"
            + (runtimeError.empty() ? "" : " (" + runtimeError + ")"));
        return;
    }

    if (vehicle == nullptr) {
        Vehicle::Parameters vehParams;
        vehParams.mass = units::mass(1597, units::kg);
        vehParams.diffRatio = 3.42;
        vehParams.tireRadius = units::distance(10, units::inch);
        vehParams.dragCoefficient = 0.25;
        vehParams.crossSectionArea = units::distance(6.0, units::foot) * units::distance(6.0, units::foot);
        vehParams.rollingResistance = 2000.0;
        vehParams.brakeConstant = 8000.0;
        vehicle = new Vehicle;
        vehicle->initialize(vehParams);
    }

    if (transmission == nullptr) {
        const double gearRatios[] = { 2.97, 2.07, 1.43, 1.00, 0.84, 0.56 };
        Transmission::Parameters tParams;
        tParams.GearCount = 6;
        tParams.GearRatios = gearRatios;
        tParams.MaxClutchTorque = units::torque(1000.0, units::ft_lb);
        transmission = new Transmission;
        transmission->initialize(tParams);
    }

    m_currentEnginePath = path;
    loadEngine(engine, vehicle, transmission);
    refreshUserInterface();
}

void EngineSimApplication::processEngineInput() {
    if (m_iceEngine == nullptr) {
        return;
    }

    const float dt = m_engine.GetFrameLength();
    const bool fineControlMode = m_keyBindings.isDown(InputAction::FineControl, &m_engine);

    const int mouseWheel = m_engine.GetMouseWheel();
    const int mouseWheelDelta = mouseWheel - m_lastMouseWheel;
    m_lastMouseWheel = mouseWheel;

    bool fineControlInUse = false;
    if (m_keyBindings.isDown(InputAction::Volume, &m_engine)) {
        const double rate = fineControlMode
            ? 0.001
            : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.volume = clamp(audioParams.volume + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[Z] - Set volume to " + std::to_string(audioParams.volume));
    }
    else if (m_keyBindings.isDown(InputAction::ConvolutionLevel, &m_engine)) {
        const double rate = fineControlMode
            ? 0.001
            : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.convolution = clamp(audioParams.convolution + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[X] - Set convolution level to " + std::to_string(audioParams.convolution));
    }
    else if (m_keyBindings.isDown(InputAction::HighFreqGain, &m_engine)) {
        const double rate = fineControlMode
            ? 0.00001
            : 0.001;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.dF_F_mix = clamp(audioParams.dF_F_mix + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[C] - Set high freq. gain to " + std::to_string(audioParams.dF_F_mix));
    }
    else if (m_keyBindings.isDown(InputAction::LowFreqNoise, &m_engine)) {
        const double rate = fineControlMode
            ? 0.001
            : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.airNoise = clamp(audioParams.airNoise + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[V] - Set low freq. noise to " + std::to_string(audioParams.airNoise));
    }
    else if (m_keyBindings.isDown(InputAction::HighFreqNoise, &m_engine)) {
        const double rate = fineControlMode
            ? 0.001
            : 0.01;

        Synthesizer::AudioParameters audioParams = m_simulator->synthesizer().getAudioParameters();
        audioParams.inputSampleNoise = clamp(audioParams.inputSampleNoise + mouseWheelDelta * rate * dt);

        m_simulator->synthesizer().setAudioParameters(audioParams);
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[B] - Set high freq. noise to " + std::to_string(audioParams.inputSampleNoise));
    }
    else if (m_keyBindings.isDown(InputAction::SimFrequency, &m_engine)) {
        const double rate = fineControlMode
            ? 10.0
            : 100.0;

        const double newSimulationFrequency = clamp(
            m_simulator->getSimulationFrequency() + mouseWheelDelta * rate * dt,
            400.0, 400000.0);

        m_simulator->setSimulationFrequency(newSimulationFrequency);
        m_autoTuneSimFrequency = false;
        fineControlInUse = true;

        m_infoCluster->setLogMessage("[N] - Set simulation freq to " + std::to_string(m_simulator->getSimulationFrequency()));
    }
    else if (m_keyBindings.isDown(InputAction::DynoHoldAdjust, &m_engine) && m_simulator->m_dyno.m_hold) {
        if (mouseWheelDelta > 0) {
            m_dynoSpeed += m_iceEngine->getDynoHoldStep();
        }
        else if (mouseWheelDelta < 0) {
            m_dynoSpeed -= m_iceEngine->getDynoHoldStep();
        }

        m_dynoSpeed = clamp(m_dynoSpeed, m_iceEngine->getDynoMinSpeed(), m_iceEngine->getDynoMaxSpeed());

        m_infoCluster->setLogMessage("[G] - Set dyno speed to " + std::to_string(units::toRpm(m_dynoSpeed)));
        fineControlInUse = true;
    }

    const double prevTargetThrottle = m_targetSpeedSetting;
    m_targetSpeedSetting = fineControlMode ? m_targetSpeedSetting : 0.0;
    if (m_keyBindings.isDown(InputAction::ThrottleIdle, &m_engine)) {
        m_targetSpeedSetting = 0.01;
    }
    else if (m_keyBindings.isDown(InputAction::ThrottleLow, &m_engine)) {
        m_targetSpeedSetting = 0.1;
    }
    else if (m_keyBindings.isDown(InputAction::ThrottleMid, &m_engine)) {
        m_targetSpeedSetting = 0.2;
    }
    else if (m_keyBindings.isDown(InputAction::ThrottleFull, &m_engine)) {
        m_targetSpeedSetting = 1.0;
    }
    else if (fineControlMode && !fineControlInUse) {
        m_targetSpeedSetting = clamp(m_targetSpeedSetting + mouseWheelDelta * 0.0001);
    }

    if (prevTargetThrottle != m_targetSpeedSetting) {
        m_infoCluster->setLogMessage("Speed control set to " + std::to_string(m_targetSpeedSetting));
    }

    m_speedSetting = m_targetSpeedSetting * 0.5 + 0.5 * m_speedSetting;

    m_iceEngine->setSpeedControl(m_speedSetting);
    if (m_keyBindings.pressed(InputAction::ViewLayerUp, &m_engine)) {
        const int currentLayer = getViewParameters().Layer0;
        if (currentLayer + 1 < m_iceEngine->getMaxDepth()) {
            setViewLayer(currentLayer + 1);
        }

        m_infoCluster->setLogMessage("[M] - Set render layer to " + std::to_string(getViewParameters().Layer0));
    }

    if (m_keyBindings.pressed(InputAction::ViewLayerDown, &m_engine)) {
        if (getViewParameters().Layer0 - 1 >= 0)
            setViewLayer(getViewParameters().Layer0 - 1);

        m_infoCluster->setLogMessage("[,] - Set render layer to " + std::to_string(getViewParameters().Layer0));
    }

    if (m_keyBindings.pressed(InputAction::DynoEnable, &m_engine)) {
        m_simulator->m_dyno.m_enabled = !m_simulator->m_dyno.m_enabled;

        const std::string msg = m_simulator->m_dyno.m_enabled
            ? "DYNOMOMETER ENABLED"
            : "DYNOMOMETER DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_keyBindings.pressed(InputAction::DynoHold, &m_engine)) {
        m_simulator->m_dyno.m_hold = !m_simulator->m_dyno.m_hold;

        const std::string msg = m_simulator->m_dyno.m_hold
            ? m_simulator->m_dyno.m_enabled ? "HOLD ENABLED" : "HOLD ON STANDBY [ENABLE DYNO. FOR HOLD]"
            : "HOLD DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_simulator->m_dyno.m_enabled) {
        if (!m_simulator->m_dyno.m_hold) {
            if (m_simulator->getFilteredDynoTorque() > units::torque(1.0, units::ft_lb)) {
                m_dynoSpeed += units::rpm(500) * dt;
            }
            else {
                m_dynoSpeed *= (1 / (1 + dt));
            }

            if (m_dynoSpeed > m_iceEngine->getRedline()) {
                m_simulator->m_dyno.m_enabled = false;
                m_dynoSpeed = units::rpm(0);
            }
        }
    }
    else {
        if (!m_simulator->m_dyno.m_hold) {
            m_dynoSpeed = units::rpm(0);
        }
    }

    m_dynoSpeed = clamp(m_dynoSpeed, m_iceEngine->getDynoMinSpeed(), m_iceEngine->getDynoMaxSpeed());
    m_simulator->m_dyno.m_rotationSpeed = m_dynoSpeed;

    const bool prevStarterEnabled = m_simulator->m_starterMotor.m_enabled;
    if (m_keyBindings.isDown(InputAction::Starter, &m_engine)) {
        m_simulator->m_starterMotor.m_enabled = true;
    }
    else {
        m_simulator->m_starterMotor.m_enabled = false;
    }

    if (prevStarterEnabled != m_simulator->m_starterMotor.m_enabled) {
        const std::string msg = m_simulator->m_starterMotor.m_enabled
            ? "STARTER ENABLED"
            : "STARTER DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    if (m_keyBindings.pressed(InputAction::Ignition, &m_engine)) {
        m_simulator->getEngine()->getIgnitionModule()->m_enabled =
            !m_simulator->getEngine()->getIgnitionModule()->m_enabled;

        const std::string msg = m_simulator->getEngine()->getIgnitionModule()->m_enabled
            ? "IGNITION ENABLED"
            : "IGNITION DISABLED";
        m_infoCluster->setLogMessage(msg);
    }

    // Catch gear changes the computer made on its own since last frame
    // (TCU shifting by RPM, or a CVT sweeping its ratio) before this
    // frame's own manual shift key is handled, so the two don't get
    // double-logged. Mode changes (Drive/Sport/Manual) are always a direct
    // result of a key press below, never something the computer does on
    // its own, so this only needs to watch the gear number.
    {
        Transmission *transmission = m_simulator->getTransmission();
        const int gearNow = transmission->getGear();
        if (gearNow != m_lastLoggedGear && !transmission->isManual()) {
            m_infoCluster->setLogMessage(gearNow > m_lastLoggedGear
                ? "AUTO-UPSHIFTED TO " + transmission->getGearLabel()
                : "AUTO-DOWNSHIFTED TO " + transmission->getGearLabel());
        }
        m_lastLoggedGear = gearNow;
    }

    if (m_keyBindings.pressed(InputAction::GearUp, &m_engine)) {
        Transmission *transmission = m_simulator->getTransmission();
        if (transmission->shiftUp()) {
            m_lastLoggedGear = transmission->getGear();
            m_infoCluster->setLogMessage("SHIFTED TO " + transmission->getGearLabel());
        }
        else if (transmission->isManual()) {
            m_infoCluster->setLogMessage("CANNOT SHIFT - PRESS CLUTCH");
        }
    }
    else if (m_keyBindings.pressed(InputAction::GearDown, &m_engine)) {
        Transmission *transmission = m_simulator->getTransmission();
        if (transmission->shiftDown()) {
            m_lastLoggedGear = transmission->getGear();
            m_infoCluster->setLogMessage(transmission->getGear() != -1
                ? "SHIFTED TO " + transmission->getGearLabel()
                : "SHIFTED TO NEUTRAL");
        }
        else if (transmission->isManual()) {
            m_infoCluster->setLogMessage("CANNOT SHIFT - PRESS CLUTCH");
        }
        else if (transmission->wasBlockedByDownshiftProtection()) {
            m_infoCluster->setLogMessage("DOWNSHIFT BLOCKED - OVER-REV PROTECTION");
        }
    }

    if (m_keyBindings.isDown(InputAction::ClutchDecrease, &m_engine)) {
        m_targetClutchPressure -= 0.2 * dt;
    }
    else if (m_keyBindings.isDown(InputAction::ClutchIncrease, &m_engine)) {
        m_targetClutchPressure += 0.2 * dt;
    }
    else if (m_keyBindings.isDown(InputAction::ClutchDepress, &m_engine)) {
        m_targetClutchPressure = 0.0;
        m_infoCluster->setLogMessage("CLUTCH DEPRESSED");
    }
    else if (!m_keyBindings.isDown(InputAction::ClutchHold, &m_engine)) {
        m_targetClutchPressure = 1.0;
    }

    m_targetClutchPressure = clamp(m_targetClutchPressure);

    double clutchRC = 0.001;
    if (m_keyBindings.isDown(InputAction::FineControl, &m_engine)) {
        clutchRC = 1.0;
    }

    const double clutch_s = dt / (dt + clutchRC);
    m_clutchPressure = m_clutchPressure * (1 - clutch_s) + m_targetClutchPressure * clutch_s;
    m_simulator->getTransmission()->setClutchPressure(m_clutchPressure);

    const bool prevBrakeApplied = m_simulator->getVehicle()->getBrakeInput() > 0.0;
    const bool brakeApplied = m_keyBindings.isDown(InputAction::Brake, &m_engine);
    m_simulator->getVehicle()->setBrakeInput(brakeApplied ? 1.0 : 0.0);
    if (brakeApplied != prevBrakeApplied) {
        m_infoCluster->setLogMessage(brakeApplied ? "BRAKE APPLIED" : "BRAKE RELEASED");
    }
}

void EngineSimApplication::renderScene() {
    getShaders()->ResetBaseColor();
    getShaders()->SetObjectTransform(ysMath::LoadIdentity());

    m_textRenderer.SetColor(ysColor::linearToSrgb(m_foreground));
    m_shaders.SetClearColor(ysColor::linearToSrgb(m_shadow));

    const int screenWidth = m_engine.GetGameWindow()->GetGameWidth();
    const int screenHeight = m_engine.GetGameWindow()->GetGameHeight();
    const float aspectRatio = screenWidth / (float)screenHeight;

    const Point cameraPos = m_engineView->getCameraPosition();
    m_shaders.m_cameraPosition = ysMath::LoadVector(cameraPos.x, cameraPos.y);

    m_shaders.CalculateUiCamera(screenWidth, screenHeight);

    Bounds windowBounds((float)screenWidth, (float)screenHeight, { 0, (float)screenHeight });

    // Main/Telemetry/Dyno all share a full-width top strip for InfoCluster
    // (branding/toolbar buttons/log messages); Cinematic omits it entirely
    // for a clean full-viewport shot.
    Grid outer;
    outer.h_cells = 1;
    outer.v_cells = 6;
    const Bounds topBar = outer.get(windowBounds, 0, 0, 1, 1);
    const Bounds contentArea = outer.get(windowBounds, 0, 1, 1, 5);

    if (m_dynoScreenVisible) {
        m_engineView->setBounds(windowBounds);
        m_engineView->setLocalPosition({ 0, 0 });

        m_infoCluster->m_bounds = topBar;
        m_dynoCluster->m_bounds = contentArea;

        m_engineView->setVisible(false);
        m_rightGaugeCluster->setVisible(false);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(true);
        m_dynoCluster->setVisible(true);

        m_dynoCluster->activate();
    }
    else if (m_screen == ScreenId::Main) {
        Grid mainGrid;
        mainGrid.h_cells = 3;
        mainGrid.v_cells = 1;

        m_engineView->setDrawFrame(true);
        m_engineView->setBounds(mainGrid.get(contentArea, 0, 0, 2, 1));
        m_engineView->setLocalPosition({ 0, 0 });
        m_engineView->activate();

        m_rightGaugeCluster->m_bounds = mainGrid.get(contentArea, 2, 0, 1, 1);
        m_infoCluster->m_bounds = topBar;

        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(true);
        m_infoCluster->setVisible(true);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_dynoCluster->setVisible(false);
    }
    else if (m_screen == ScreenId::Telemetry) {
        Grid grid;
        grid.v_cells = 2;
        grid.h_cells = 3;
        Grid grid3x3;
        grid3x3.v_cells = 3;
        grid3x3.h_cells = 3;

        m_engineView->setDrawFrame(true);
        m_engineView->setBounds(grid.get(contentArea, 1, 0, 1, 1));
        m_engineView->setLocalPosition({ 0, 0 });

        m_rightGaugeCluster->m_bounds = grid.get(contentArea, 2, 0, 1, 2);
        m_oscCluster->m_bounds = grid.get(contentArea, 1, 1);
        m_performanceCluster->m_bounds = grid3x3.get(contentArea, 0, 0);
        m_loadSimulationCluster->m_bounds = grid3x3.get(contentArea, 0, 1);
        m_mixerCluster->m_bounds = grid3x3.get(contentArea, 0, 2);
        m_infoCluster->m_bounds = topBar;

        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(true);
        m_oscCluster->setVisible(true);
        m_performanceCluster->setVisible(true);
        m_loadSimulationCluster->setVisible(true);
        m_mixerCluster->setVisible(true);
        m_infoCluster->setVisible(true);
        m_dynoCluster->setVisible(false);

        m_oscCluster->activate();
    }
    else { // ScreenId::Cinematic
        m_engineView->setDrawFrame(false);
        m_engineView->setBounds(windowBounds);
        m_engineView->setLocalPosition({ 0, 0 });
        m_engineView->activate();

        m_engineView->setVisible(true);
        m_rightGaugeCluster->setVisible(false);
        m_oscCluster->setVisible(false);
        m_performanceCluster->setVisible(false);
        m_loadSimulationCluster->setVisible(false);
        m_mixerCluster->setVisible(false);
        m_infoCluster->setVisible(false);
        m_dynoCluster->setVisible(false);
    }

    // Covers the full window and sits on top of whichever screen is active
    // above, regardless of m_screen/dyno state.
    m_settingsCluster->m_bounds = windowBounds;

    if (m_turboEditorCluster != nullptr) {
        constexpr float panelWidth = 300.0f;
        constexpr float panelHeight = 300.0f;
        m_turboEditorCluster->m_bounds = Bounds(
            panelWidth,
            panelHeight,
            { (float)screenWidth - panelWidth - 20.0f, (float)screenHeight - 20.0f },
            Bounds::tl);
    }

    const float cameraAspectRatio =
        m_engineView->m_bounds.width() / m_engineView->m_bounds.height();
    m_engine.GetDevice()->ResizeRenderTarget(
        m_mainRenderTarget,
        m_engineView->m_bounds.width(),
        m_engineView->m_bounds.height(),
        m_engineView->m_bounds.width(),
        m_engineView->m_bounds.height()
    );
    m_engine.GetDevice()->RepositionRenderTarget(
        m_mainRenderTarget,
        m_engineView->m_bounds.getPosition(Bounds::tl).x,
        screenHeight - m_engineView->m_bounds.getPosition(Bounds::tl).y
    );
    m_shaders.CalculateCamera(
        cameraAspectRatio * m_displayHeight / m_engineView->m_zoom,
        m_displayHeight / m_engineView->m_zoom,
        m_engineView->m_bounds,
        m_screenWidth,
        m_screenHeight,
        m_displayAngle);

    m_geometryGenerator.reset();

    render();

    m_engine.GetDevice()->EditBufferDataRange(
        m_geometryVertexBuffer,
        (char *)m_geometryGenerator.getVertexData(),
        sizeof(dbasic::Vertex) * m_geometryGenerator.getCurrentVertexCount(),
        0);

    m_engine.GetDevice()->EditBufferDataRange(
        m_geometryIndexBuffer,
        (char *)m_geometryGenerator.getIndexData(),
        sizeof(unsigned short) * m_geometryGenerator.getCurrentIndexCount(),
        0);
}

void EngineSimApplication::refreshUserInterface() {
    m_uiManager.destroy();
    m_uiManager.initialize(this);

    m_engineView = m_uiManager.getRoot()->addElement<EngineView>();
    m_rightGaugeCluster = m_uiManager.getRoot()->addElement<RightGaugeCluster>();
    m_oscCluster = m_uiManager.getRoot()->addElement<OscilloscopeCluster>();
    m_performanceCluster = m_uiManager.getRoot()->addElement<PerformanceCluster>();
    m_loadSimulationCluster = m_uiManager.getRoot()->addElement<LoadSimulationCluster>();
    m_mixerCluster = m_uiManager.getRoot()->addElement<MixerCluster>();
    m_infoCluster = m_uiManager.getRoot()->addElement<InfoCluster>();
    m_turboEditorCluster = m_uiManager.getRoot()->addElement<TurbochargerEditorCluster>();
    m_dynoCluster = m_uiManager.getRoot()->addElement<DynoCluster>();
    m_settingsCluster = m_uiManager.getRoot()->addElement<SettingsCluster>();

    m_infoCluster->setEngine(m_iceEngine);
    m_turboEditorCluster->setEngine(m_iceEngine);
    m_turboEditorCluster->setVisible(m_turboEditorVisible);
    m_rightGaugeCluster->m_simulator = m_simulator;
    m_rightGaugeCluster->setEngine(m_iceEngine);
    m_oscCluster->setSimulator(m_simulator);
    if (m_iceEngine != nullptr) {
        m_oscCluster->setDynoMaxRange(units::toRpm(m_iceEngine->getRedline()));
    }
    m_performanceCluster->setSimulator(m_simulator);
    m_loadSimulationCluster->setSimulator(m_simulator);
    m_mixerCluster->setSimulator(m_simulator);
    m_dynoCluster->setSimulator(m_simulator);

    // m_settingsCluster is rebuilt here (like every other cluster) whenever
    // the UI tree is refreshed, so it always starts closed -- m_dynoCluster
    // has no equivalent open/closed state of its own; the dyno *screen*'s
    // visibility is tracked independently by m_dynoScreenVisible (see
    // renderScene()) since it must survive engine reloads, not reset with
    // the UI tree.
}

void EngineSimApplication::startRecording() {
    m_recording = true;

#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    atg_dtv::Encoder::VideoSettings settings{};

    // Output filename
    settings.fname = "../workspace/video_capture/engine_sim_video_capture.mp4";
    settings.inputWidth = m_engine.GetScreenWidth();
    settings.inputHeight = m_engine.GetScreenHeight();
    settings.width = settings.inputWidth;
    settings.height = settings.inputHeight;
    settings.hardwareEncoding = true;
    settings.inputAlpha = true;
    settings.bitRate = 40000000;

    m_encoder.run(settings, 2);
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

void EngineSimApplication::updateScreenSizeStability() {
    m_screenResolution[m_screenResolutionIndex][0] = m_engine.GetScreenWidth();
    m_screenResolution[m_screenResolutionIndex][1] = m_engine.GetScreenHeight();

    m_screenResolutionIndex = (m_screenResolutionIndex + 1) % ScreenResolutionHistoryLength;
}

bool EngineSimApplication::readyToRecord() {
    const int w = m_screenResolution[0][0];
    const int h = m_screenResolution[0][1];

    if (w <= 0 && h <= 0) return false;
    if ((w % 2) != 0 || (h % 2) != 0) return false;

    for (int i = 1; i < ScreenResolutionHistoryLength; ++i) {
        if (m_screenResolution[i][0] != w) return false;
        if (m_screenResolution[i][1] != h) return false;
    }

    return true;
}

void EngineSimApplication::stopRecording() {
    m_recording = false;

#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    m_encoder.commit();
    m_encoder.stop();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

void EngineSimApplication::recordFrame() {
#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    atg_dtv::Frame *frame = m_encoder.newFrame(false);
    if (frame != nullptr && m_encoder.getError() == atg_dtv::Encoder::Error::None) {
        m_engine.GetDevice()->ReadRenderTarget(m_engine.GetScreenRenderTarget(), frame->m_rgb);
    }

    m_encoder.submitFrame();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}
