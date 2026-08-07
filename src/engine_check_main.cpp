// Headless engine test tool -- loads a .mr script the same way the real app
// does (compile -> Engine/Vehicle/Transmission -> Simulator), but with no
// SDL window/rendering, and runs a fixed battery of checks against it:
//   1) does it start (starter cranks it, it catches and idles on its own)
//   2) does it reach its configured redline at wide-open throttle in neutral
//   3) a WOT dyno pull (torque/power curve, same sweep the app's dyno does)
//   4) steady-state throughput (physics + audio load), reported as an
//      equivalent sustainable FPS
// Usage: engine-sim-check.exe [path/to/engine.mr]
// With no argument, a file-open dialog is shown instead. Results go to
// stdout; the process only pauses for a keypress when no path was given on
// the command line (so double-clicking the exe doesn't flash-close).

#pragma comment(lib, "comdlg32.lib")

#define NOMINMAX

#include "../scripting/include/compiler.h"

#include "../include/piston_engine_simulator.h"
#include "../include/units.h"
#include "../include/utilities.h"

#include <delta-studio/include/yds_windows_audio_wave_file.h>

#include <windows.h>
#include <commdlg.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string toForwardSlashes(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

bool pickFileViaDialog(std::string *outPath) {
    char fileBuffer[MAX_PATH] = {};

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Engine Simulator Scripts (*.mr)\0*.mr\0All Files\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = "../assets/engines"; // cwd is pinned to this exe's own directory, see main()
    // OFN_NOCHANGEDIR: without it, GetOpenFileNameA silently changes the
    // process's cwd to match wherever the user navigated to, which would
    // undo the cwd pin in main() (needed so the compiler's "../es/" import
    // search paths keep resolving) the instant a file gets picked.
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn)) return false;
    *outPath = fileBuffer;
    return true;
}

std::string absolutePath(const std::string &path) {
    char buffer[MAX_PATH] = {};
    const DWORD n = GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr);
    return (n > 0 && n < MAX_PATH) ? std::string(buffer) : path;
}

// Mirrors EngineSimApplication::autoWireEngineScript()/the mr_asset_smoke_test
// wrapper trick: the script defines `public node main { ... }` but never
// instantiates it itself, so a tiny wrapper importing it and calling main()
// is needed to actually run it.
bool compileScript(
    const std::string &path,
    es_script::Compiler::Output *out,
    std::string *error)
{
    const std::string absPath = toForwardSlashes(absolutePath(path));
    const std::string wrapperPath = absPath + ".__engine_check_wrapper.mr";

    {
        std::ofstream wrapper(wrapperPath);
        wrapper << "import \"" << absPath << "\"\n" << "main()\n";
    }

    es_script::Compiler compiler;
    compiler.initialize();
    const bool compiled = compiler.compile(wrapperPath);
    std::remove(wrapperPath.c_str());

    if (!compiled) {
        compiler.destroy();
        std::ifstream log("error_log.log");
        std::stringstream ss;
        ss << log.rdbuf();
        *error = ss.str();
        return false;
    }

    *out = compiler.execute();
    *error = compiler.getRuntimeError();
    compiler.destroy();
    return out->engine != nullptr;
}

// engine_sim_application.cpp's own fallback drivetrain, reused verbatim so a
// script that only defines an engine (no set_vehicle/set_transmission) still
// gets something driveable to test against.
Vehicle *defaultVehicle() {
    Vehicle::Parameters p;
    p.mass = units::mass(1597, units::kg);
    p.diffRatio = 3.42;
    p.tireRadius = units::distance(10, units::inch);
    p.dragCoefficient = 0.25;
    p.crossSectionArea = units::distance(6.0, units::foot) * units::distance(6.0, units::foot);
    p.rollingResistance = 2000.0;
    p.brakeConstant = 8000.0;
    Vehicle *v = new Vehicle;
    v->initialize(p);
    return v;
}

Transmission *defaultTransmission() {
    static const double gearRatios[] = { 2.97, 2.07, 1.43, 1.00, 0.84, 0.56 };
    Transmission::Parameters p;
    p.GearCount = 6;
    p.GearRatios = gearRatios;
    p.MaxClutchTorque = units::torque(1000.0, units::ft_lb);
    Transmission *t = new Transmission;
    t->initialize(p);
    return t;
}

// Loads each exhaust system's impulse response .wav and wires up the audio
// parameters, same as EngineSimApplication::loadEngine() -- this is the
// "everything the program loads for this engine" piece that isn't part of
// Simulator/PistonEngineSimulator itself.
void loadAudio(Simulator *sim, Engine *engine) {
    Synthesizer::AudioParameters audioParams = sim->synthesizer().getAudioParameters();
    audioParams.inputSampleNoise = static_cast<float>(engine->getInitialJitter());
    audioParams.airNoise = static_cast<float>(engine->getInitialNoise());
    audioParams.dF_F_mix = static_cast<float>(engine->getInitialHighFrequencyGain());
    sim->synthesizer().setAudioParameters(audioParams);

    const int exhaustSystemCount = engine->getExhaustSystemCount();
    for (int i = 0; i < exhaustSystemCount; ++i) {
        ImpulseResponse *response = engine->getExhaustSystem(i)->getImpulseResponse();
        if (response == nullptr) continue;

        ysWindowsAudioWaveFile waveFile;
        waveFile.OpenFile(response->getFilename().c_str());
        waveFile.InitializeInternalBuffer(waveFile.GetSampleCount());
        waveFile.FillBuffer(0);
        waveFile.CloseFile();

        sim->synthesizer().initializeImpulseResponse(
            reinterpret_cast<const int16_t *>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            static_cast<float>(response->getVolume()),
            i);

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

        sim->synthesizer().initializeImpulseResponse(
            reinterpret_cast<const int16_t *>(waveFile.GetBuffer()),
            waveFile.GetSampleCount(),
            static_cast<float>(response->getVolume()),
            exhaustSystemCount + i);

        waveFile.DestroyInternalBuffer();
    }
}

// Simulator::startFrame() throttles its physics-step count against the
// synthesizer's own input/output latency (it assumes a real-time consumer is
// pulling rendered audio out every frame, same as the real app's audio
// device callback) -- without that drain, latency backs up without bound and
// the feedback in startFrame() collapses the step count to zero forever, so
// this has to run every step of every test, not just the FPS one.
int16_t g_audioDrain[4096];

void stepSimulator(Simulator *sim, double dt) {
    sim->startFrame(dt);
    while (sim->simulateStep()) { /* drain all substeps for this frame */ }
    sim->endFrame();
    sim->readAudioOutput(4096, g_audioDrain);
}

// Test 1: crank the starter, let go, see if it's still running under its own
// power (rather than decaying back toward a dead stop) once released.
struct StartResult {
    bool started = false;
    bool stable = true; // false if RPM went non-finite (NaN/Inf) at any point
    double settledRpm = 0.0;
};

StartResult runStartTest(Simulator *sim, Engine *engine) {
    const double dt = 1.0 / 240.0;
    StartResult result;

    // Engine::update() re-drives the intake throttle position from the
    // engine's Throttle object (DirectThrottleLinkage/Governor) every single
    // physics substep, so a one-shot Engine::setThrottle() call gets
    // overwritten on the very next substep -- setSpeedControl() (the "gas
    // pedal", 0 = off, 1 = WOT) is the one that actually sticks.
    engine->setSpeedControl(0.0); // foot off the gas, idling

    sim->m_starterMotor.m_enabled = true;
    for (double t = 0.0; t < 4.0 && result.stable; t += dt) {
        stepSimulator(sim, dt);
        result.stable = std::isfinite(engine->getRpm());
    }

    sim->m_starterMotor.m_enabled = false;
    for (double t = 0.0; t < 3.0 && result.stable; t += dt) {
        stepSimulator(sim, dt);
        result.stable = std::isfinite(engine->getRpm());
    }

    if (result.stable) {
        result.settledRpm = engine->getRpm();
        const double runningFloor = std::max(300.0, 0.08 * units::toRpm(engine->getRedline()));
        result.started = result.settledRpm > runningFloor;
    }

    return result;
}

// Test 2: wide-open throttle, neutral (the transmission starts in neutral
// and nothing here ever shifts it), see if it reaches its configured
// redline (Engine::getRedline() -- the driver-facing dashboard value, not
// necessarily the same as the ignition's independent rev_limit cutoff).
struct RedlineResult {
    bool reached = false;
    bool stable = true;
    double maxRpm = 0.0;
    double timeToRedline = 0.0;
};

RedlineResult runRedlineTest(Simulator *sim, Engine *engine) {
    const double dt = 1.0 / 240.0;
    RedlineResult result;

    engine->setSpeedControl(1.0); // WOT (see runStartTest's comment on why not setThrottle())

    for (double t = 0.0; t < 12.0 && result.stable && !result.reached; t += dt) {
        stepSimulator(sim, dt);
        result.stable = std::isfinite(engine->getRpm());
        if (!result.stable) break;

        result.maxRpm = std::max(result.maxRpm, engine->getRpm());
        if (engine->getRpm() >= units::toRpm(engine->getRedline())) {
            result.reached = true;
            result.timeToRedline = t + dt;
        }
    }

    return result;
}

// Test 3: a WOT dyno pull -- exactly the sweep
// EngineSimApplication::onUpdate() drives off the [D]/[H] dyno keys: hold
// the crank at a target speed, ramp that target up by 500rpm/s as long as
// the engine is making meaningful torque, stop at redline.
struct DynoPoint { double rpm, torqueFtLb, powerHp; };

struct DynoResult {
    bool stable = true;
    std::vector<DynoPoint> curve;
    DynoPoint peakTorque{};
    DynoPoint peakPower{};
};

DynoResult runDynoTest(Simulator *sim, Engine *engine) {
    const double dt = 1.0 / 240.0;
    DynoResult result;

    // Let whatever the previous test left the engine doing settle back
    // toward idle first -- starting the dyno's speed-hold constraint at a
    // target of 0rpm while the crank is still spinning at redline (e.g.
    // right after runRedlineTest()) is a huge, instant speed mismatch that
    // reads as a violent stall rather than a real pull.
    engine->setSpeedControl(0.0);
    for (double t = 0.0; t < 2.0 && result.stable; t += dt) {
        stepSimulator(sim, dt);
        result.stable = std::isfinite(engine->getRpm());
    }
    engine->setSpeedControl(1.0); // WOT (see runStartTest's comment on why not setThrottle())

    sim->m_dyno.m_enabled = true;
    sim->m_dyno.m_hold = false;

    double dynoSpeed = 0.0;
    double nextSampleAt = 0.0;

    for (double t = 0.0; t < 40.0 && result.stable; t += dt) {
        if (sim->m_dyno.m_enabled) {
            if (sim->getFilteredDynoTorque() > units::torque(1.0, units::ft_lb)) {
                dynoSpeed += units::rpm(500) * dt;
            }
            else {
                dynoSpeed *= (1 / (1 + dt));
            }

            if (dynoSpeed > engine->getRedline()) {
                sim->m_dyno.m_enabled = false;
                dynoSpeed = units::rpm(0);
            }
        }

        dynoSpeed = clamp(dynoSpeed, engine->getDynoMinSpeed(), engine->getDynoMaxSpeed());
        sim->m_dyno.m_rotationSpeed = dynoSpeed;

        stepSimulator(sim, dt);
        result.stable = std::isfinite(engine->getRpm()) && std::isfinite(sim->getFilteredDynoTorque());
        if (!result.stable) break;

        if (t >= nextSampleAt) {
            DynoPoint p;
            p.rpm = engine->getRpm();
            p.torqueFtLb = sim->getFilteredDynoTorque() / units::ft_lb;
            p.powerHp = sim->getDynoPower() / units::hp;
            result.curve.push_back(p);
            if (p.torqueFtLb > result.peakTorque.torqueFtLb) result.peakTorque = p;
            if (p.powerHp > result.peakPower.powerHp) result.peakPower = p;
            nextSampleAt = t + 0.1;
        }

        if (!sim->m_dyno.m_enabled) break; // hit redline, sweep is done
    }

    return result;
}

// Test 4: with everything already loaded (impulse responses, audio params,
// the background audio-rendering thread already running since main()
// started it), measure how much simulated time a fixed wall-clock budget
// buys -- reported back as the FPS the physics+audio pipeline could sustain
// if the real app's frame rate were bottlenecked on it.
struct FpsResult {
    bool stable = true;
    double sustainableFps = 0.0;
    double avgFrameMicros = 0.0; // per stepSimulator() call, i.e. per ~1/60s frame's worth of substeps
};

FpsResult runFpsTest(Simulator *sim, Engine *engine) {
    const double frameDt = 1.0 / 60.0;
    const int frames = 240; // 4 simulated seconds
    FpsResult result;

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < frames && result.stable; ++i) {
        stepSimulator(sim, frameDt);
        result.stable = std::isfinite(engine->getRpm());
    }
    const auto end = std::chrono::steady_clock::now();

    const double wallClockSeconds =
        std::chrono::duration<double>(end - start).count();
    const double simulatedSeconds = frames * frameDt;

    if (result.stable && wallClockSeconds > 0.0) {
        result.sustainableFps = 60.0 * (simulatedSeconds / wallClockSeconds);
        result.avgFrameMicros = sim->getAverageProcessingTime(); // already in microseconds
    }

    return result;
}

void printHeader(const std::string &title) {
    std::cout << "\n=== " << title << " ===\n";
}

} // namespace

int main(int argc, char **argv) {
    const bool cliMode = argc > 1;
    std::string path;

    // Resolve a CLI-provided path against the caller's cwd *before* changing
    // directories below.
    if (cliMode) path = absolutePath(argv[1]);

    // es_script::Compiler::initialize() registers its .mr import search
    // paths (for the "import engine_sim.mr" every script starts with) as
    // "../es/" etc., relative to the process's current working directory --
    // it has no idea where this executable lives. Pin cwd to this exe's own
    // directory (a sibling of es/, same as the other build outputs) so that
    // resolves correctly no matter where this was actually launched from.
    char exePath[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, exePath, MAX_PATH) > 0) {
        std::string exeDir = exePath;
        const size_t lastSlash = exeDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            SetCurrentDirectoryA(exeDir.substr(0, lastSlash).c_str());
        }
    }

    if (!cliMode && !pickFileViaDialog(&path)) {
        std::cout << "No file selected.\n";
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::cout << "Engine Sim Check -- " << path << "\n";

    es_script::Compiler::Output out;
    std::string error;
    if (!compileScript(path, &out, &error)) {
        std::cout << "\nFAILED TO LOAD ENGINE\n";
        if (!error.empty()) std::cout << error << "\n";
        if (!cliMode) {
            std::cout << "\nPress Enter to exit...";
            std::cin.get();
        }
        return 1;
    }

    Engine *engine = out.engine;
    Vehicle *vehicle = (out.vehicle != nullptr) ? out.vehicle : defaultVehicle();
    Transmission *transmission =
        (out.transmission != nullptr) ? out.transmission : defaultTransmission();

    std::cout << "Loaded: " << engine->getName() << "\n";

    Simulator *sim = engine->createSimulator(vehicle, transmission);
    sim->setSimulationFrequency(engine->getSimulationFrequency());
    loadAudio(sim, engine);
    sim->startAudioRenderingThread();

    // "Ignition on" -- separate from the starter motor (cranking) and off by
    // default (see IgnitionModule's constructor and the app's own "A" key),
    // same as a real key needing both ON and START to actually catch.
    engine->getIgnitionModule()->m_enabled = true;

    printHeader("1) Starts?");
    const StartResult startResult = runStartTest(sim, engine);
    if (!startResult.stable) {
        std::cout << "CRASHED -- simulation went unstable (NaN/Inf RPM) while cranking.\n";
    }
    else {
        std::cout << (startResult.started ? "PASS" : "FAIL") << " -- settled at "
            << startResult.settledRpm << " rpm after starter release"
            << (startResult.started ? "" : " (stalled)") << "\n";
    }

    printHeader("2) Hits redline in neutral?");
    const RedlineResult redlineResult = runRedlineTest(sim, engine);
    if (!redlineResult.stable) {
        std::cout << "CRASHED -- simulation went unstable (NaN/Inf RPM) at WOT.\n";
    }
    else {
        std::cout << (redlineResult.reached ? "PASS" : "FAIL") << " -- redline "
            << units::toRpm(engine->getRedline()) << " rpm, reached "
            << redlineResult.maxRpm << " rpm";
        if (redlineResult.reached) {
            std::cout << " in " << redlineResult.timeToRedline << "s";
        }
        std::cout << "\n";
    }

    printHeader("3) Dyno run");
    const DynoResult dynoResult = runDynoTest(sim, engine);
    if (!dynoResult.stable) {
        std::cout << "CRASHED -- simulation went unstable (NaN/Inf) during the pull.\n";
    }
    else if (dynoResult.curve.empty()) {
        std::cout << "FAIL -- never produced a usable torque reading.\n";
    }
    else {
        std::cout << "  RPM    Torque(ft-lb)   Power(hp)\n";
        for (size_t i = 0; i < dynoResult.curve.size(); i += 5) { // ~every 0.5s
            const DynoPoint &p = dynoResult.curve[i];
            std::printf("  %-6.0f %-15.1f %-.1f\n", p.rpm, p.torqueFtLb, p.powerHp);
        }
        std::cout << "  Peak torque: " << dynoResult.peakTorque.torqueFtLb
            << " ft-lb @ " << dynoResult.peakTorque.rpm << " rpm\n";
        std::cout << "  Peak power:  " << dynoResult.peakPower.powerHp
            << " hp @ " << dynoResult.peakPower.rpm << " rpm\n";
    }

    printHeader("4) Full load + FPS");
    const FpsResult fpsResult = runFpsTest(sim, engine);
    if (!fpsResult.stable) {
        std::cout << "CRASHED -- simulation went unstable (NaN/Inf) under full load.\n";
    }
    else {
        std::cout << "Sustainable FPS (physics + audio, everything loaded): "
            << fpsResult.sustainableFps << "\n";
        std::cout << "Average frame processing time: " << fpsResult.avgFrameMicros << " us\n";
    }

    std::cout << "\nDone.\n";
    std::cout.flush();

    // startAudioRenderingThread() leaves a real std::thread running -- stop
    // it explicitly so the process actually exits instead of hanging around.
    sim->releaseSimulation();

    if (!cliMode) {
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
    }

    return 0;
}
