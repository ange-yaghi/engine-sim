#include <gtest/gtest.h>

#ifdef ATG_ENGINE_SIM_PIRANHA_ENABLED

#include "../scripting/include/compiler.h"

#include "../include/simulator.h"
#include "../include/units.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

// The one thing every real drivetrain has to do that a dyno never asks of it:
// sit in gear, at a standstill, with the driver's foot off the gas, and not
// kill the engine. A torque converter does it by being a fluid coupling with
// almost no capacity at idle speed; a DCT/AMT/CVT does it by holding the
// launch clutch open until the engine is spinning fast enough to bite.
//
// These are full-stack tests (real .mr asset -> Engine/Vehicle/Transmission ->
// constraint solver), not logic-only like transmission_logic_test.cpp -- the
// stall they guard against is an emergent property of the coupling torque
// fighting the engine's idle torque, so it only shows up once the solver is
// actually in the loop.

namespace {
    // Same wrapper trick as mr_asset_smoke_test.cpp: the asset files define
    // `public node main { ... }` but never instantiate it.
    bool compileMainNode(const std::string &assetRelativePath, es_script::Compiler::Output *out) {
        const std::string path = std::string(ATG_ENGINE_SIM_ASSET_DIR) + "/" + assetRelativePath;
        const std::string wrapperPath = path + ".__launch_test_wrapper.mr";

        {
            std::ofstream wrapper(wrapperPath);
            wrapper << "import \"" << path << "\"\n" << "main()\n";
        }

        es_script::Compiler compiler;
        compiler.initialize();
        const bool compiled = compiler.compile(wrapperPath);
        std::remove(wrapperPath.c_str());

        if (!compiled) {
            compiler.destroy();
            return false;
        }

        *out = compiler.execute();
        compiler.destroy();
        return true;
    }

    // Simulator::startFrame() throttles its physics-step count against the
    // synthesizer's latency, assuming a real-time consumer is draining
    // rendered audio every frame. Without that drain the latency backs up and
    // the step count collapses to zero forever, so every step has to drain.
    int16_t g_audioDrain[4096];

    void stepSimulator(Simulator *sim, double dt) {
        sim->startFrame(dt);
        while (sim->simulateStep()) { /* drain all substeps for this frame */ }
        sim->endFrame();
        sim->readAudioOutput(4096, g_audioDrain);
    }

    struct LaunchRig {
        es_script::Compiler::Output out;
        Simulator *sim = nullptr;
        Engine *engine = nullptr;
        Transmission *transmission = nullptr;

        static constexpr double dt = 1.0 / 240.0;

        bool load(const std::string &assetRelativePath) {
            if (!compileMainNode(assetRelativePath, &out)) return false;
            if (out.engine == nullptr || out.vehicle == nullptr || out.transmission == nullptr) {
                return false;
            }

            engine = out.engine;
            transmission = out.transmission;

            sim = engine->createSimulator(out.vehicle, transmission);
            sim->setSimulationFrequency(static_cast<int>(engine->getSimulationFrequency()));

            // Synthesizer::renderAudio() runs every exhaust channel through
            // its convolution filter unconditionally, so each one has to be
            // initialized or it reads uninitialized memory. The real app
            // loads the exhaust's impulse response .wav here; nothing about
            // these tests looks at the audio, so a bare two-sample impulse
            // (above initializeImpulseResponse()'s own >100 clip threshold,
            // or it initializes to length zero) stands in for it and keeps
            // the test off delta-studio's wav loader.
            const int16_t impulse[2] = { 1000, 1000 };
            for (int i = 0; i < engine->getExhaustSystemCount(); ++i) {
                sim->synthesizer().initializeImpulseResponse(impulse, 2, 1.0f, i);
            }

            sim->startAudioRenderingThread();
            engine->getIgnitionModule()->m_enabled = true;
            return true;
        }

        ~LaunchRig() {
            if (sim != nullptr) {
                sim->endAudioRenderingThread();
                sim->releaseSimulation();
                delete sim;
            }
        }

        // Crank until it catches, then let it settle to a steady idle in
        // neutral. Returns the idle speed it settled at.
        double startAndIdle() {
            engine->setSpeedControl(0.0); // foot off the gas

            sim->m_starterMotor.m_enabled = true;
            for (double t = 0.0; t < 4.0; t += dt) stepSimulator(sim, dt);

            sim->m_starterMotor.m_enabled = false;
            for (double t = 0.0; t < 3.0; t += dt) stepSimulator(sim, dt);

            return engine->getRpm();
        }

        // Holds for `seconds` with the foot off the gas and reports the
        // lowest RPM seen. A stall shows up as this going to (near) zero.
        double runAndReportMinRpm(double seconds) {
            double minRpm = engine->getRpm();
            for (double t = 0.0; t < seconds; t += dt) {
                stepSimulator(sim, dt);
                const double rpm = engine->getRpm();
                if (!std::isfinite(rpm)) return 0.0;
                minRpm = std::min(minRpm, rpm);
            }

            return minRpm;
        }
    };

    // Shared body for "engage Drive at a standstill, foot off the gas, and
    // the engine had better survive it".
    void expectSurvivesIdleInGear(const std::string &assetRelativePath) {
        LaunchRig rig;
        ASSERT_TRUE(rig.load(assetRelativePath)) << "failed to load " << assetRelativePath;

        const double idleRpm = rig.startAndIdle();
        ASSERT_GT(idleRpm, 300.0) << "engine didn't even idle in neutral -- test is meaningless";

        // Neutral -> Drive, first gear, car stopped.
        ASSERT_TRUE(rig.transmission->shiftUp());
        ASSERT_EQ(rig.transmission->getGear(), 0);
        ASSERT_EQ(rig.transmission->getDriveMode(), Transmission::DriveMode::Drive);

        const double minRpm = rig.runAndReportMinRpm(5.0);

        EXPECT_GT(minRpm, 0.4 * idleRpm)
            << "engine was dragged down to " << minRpm << " rpm (idled at " << idleRpm
            << " in neutral) after engaging Drive at a standstill -- the coupling is "
            << "pulling harder than the engine can idle against";
    }

    // The other half of the same coin. Surviving idle in gear is trivial if
    // the launch device simply never transmits anything, so every type also
    // has to actually move the car once the driver asks it to.
    void expectLaunchesFromStandstill(const std::string &assetRelativePath) {
        LaunchRig rig;
        ASSERT_TRUE(rig.load(assetRelativePath)) << "failed to load " << assetRelativePath;

        ASSERT_GT(rig.startAndIdle(), 300.0);
        ASSERT_TRUE(rig.transmission->shiftUp());
        ASSERT_EQ(rig.transmission->getGear(), 0);
        ASSERT_LT(rig.sim->getVehicle()->getSpeed(), 0.1) << "car should still be stopped here";

        rig.engine->setSpeedControl(1.0); // floor it
        for (double t = 0.0; t < 4.0; t += LaunchRig::dt) {
            stepSimulator(rig.sim, LaunchRig::dt);
        }

        EXPECT_GT(rig.sim->getVehicle()->getSpeed(), 5.0)
            << "wide open throttle from a standstill only got the car to "
            << rig.sim->getVehicle()->getSpeed() << " m/s in 4s -- the launch "
            << "device isn't transmitting anything";
        EXPECT_GT(rig.engine->getRpm(), 300.0)
            << "engine died during the launch";
    }
}

TEST(DrivetrainLaunchTest, TorqueConverterAutoDoesNotStallAtIdleInGear) {
    expectSurvivesIdleInGear("soli_dstate/bmw/at/B58B30M1.mr");
}

TEST(DrivetrainLaunchTest, DualClutchDoesNotStallAtIdleInGear) {
    expectSurvivesIdleInGear("soli_dstate/ferrari/dct/f163_296gtb.mr");
}

TEST(DrivetrainLaunchTest, AutomatedManualDoesNotStallAtIdleInGear) {
    expectSurvivesIdleInGear("soli_dstate/lexus/amt/1lr_gue.mr");
}

TEST(DrivetrainLaunchTest, CvtDoesNotStallAtIdleInGear) {
    expectSurvivesIdleInGear("soli_dstate/toyota/cvt/corolla_2zrfae_cvt.mr");
}

TEST(DrivetrainLaunchTest, TorqueConverterAutoLaunchesFromStandstill) {
    expectLaunchesFromStandstill("soli_dstate/bmw/at/B58B30M1.mr");
}

TEST(DrivetrainLaunchTest, DualClutchLaunchesFromStandstill) {
    expectLaunchesFromStandstill("soli_dstate/ferrari/dct/f163_296gtb.mr");
}

TEST(DrivetrainLaunchTest, AutomatedManualLaunchesFromStandstill) {
    expectLaunchesFromStandstill("soli_dstate/lexus/amt/1lr_gue.mr");
}

TEST(DrivetrainLaunchTest, CvtLaunchesFromStandstill) {
    expectLaunchesFromStandstill("soli_dstate/toyota/cvt/corolla_2zrfae_cvt.mr");
}

// The stator is the difference between a torque converter and a plain fluid
// coupling: pulling away from a stop, the turbine has to see meaningfully
// more torque than the impeller gives up, and that multiplication has to
// fade out as the two sides come together.
TEST(DrivetrainLaunchTest, TorqueConverterMultipliesTorqueOnLaunchAndStopsAtTheCouplingPoint) {
    LaunchRig rig;
    ASSERT_TRUE(rig.load("soli_dstate/bmw/at/B58B30M1.mr"));

    ASSERT_GT(rig.startAndIdle(), 300.0);
    ASSERT_TRUE(rig.transmission->shiftUp());
    rig.engine->setSpeedControl(1.0);

    const TorqueConverter &converter = rig.transmission->getTorqueConverter();

    double torqueRatioAtLowSpeedRatio = 0.0;
    double lowestSpeedRatioSeen = 1.0;
    double highestSpeedRatioSeen = 0.0;
    double torqueRatioAtHighSpeedRatio = 0.0;

    for (double t = 0.0; t < 4.0; t += LaunchRig::dt) {
        stepSimulator(rig.sim, LaunchRig::dt);
        if (!converter.isEnabled() || converter.getImpellerTorque() <= 0.0) continue;

        const double speedRatio = converter.getSpeedRatio();
        if (speedRatio < lowestSpeedRatioSeen) {
            lowestSpeedRatioSeen = speedRatio;
            torqueRatioAtLowSpeedRatio =
                converter.getTurbineTorque() / converter.getImpellerTorque();
        }
        if (speedRatio > highestSpeedRatioSeen) {
            highestSpeedRatioSeen = speedRatio;
            torqueRatioAtHighSpeedRatio =
                converter.getTurbineTorque() / converter.getImpellerTorque();
        }
    }

    ASSERT_LT(lowestSpeedRatioSeen, 0.2) << "launch never got near stall conditions";
    ASSERT_GT(highestSpeedRatioSeen, 0.8) << "launch never got near the coupling point";

    EXPECT_GT(torqueRatioAtLowSpeedRatio, 1.5)
        << "no torque multiplication off the line -- the stator isn't doing anything";
    EXPECT_LT(torqueRatioAtHighSpeedRatio, 1.1)
        << "still multiplying torque at a speed ratio of " << highestSpeedRatioSeen
        << " -- the stator should be freewheeling by the coupling point";
}

#endif /* ATG_ENGINE_SIM_PIRANHA_ENABLED */

