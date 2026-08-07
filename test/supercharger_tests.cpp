#include <gtest/gtest.h>

#include "../include/supercharger.h"
#include "../include/crankshaft.h"
#include "../include/units.h"
#include "../include/scs.h"

#include <cmath>

namespace {
    Supercharger::Parameters rootsParams() {
        Supercharger::Parameters p;
        p.type = Supercharger::Type::Roots;
        p.driveRatio = 2.8;
        p.mechanicalEfficiency = 0.90;
        p.displacement = units::volume(2.3, units::L);
        p.volumetricEfficiency = 0.80;
        p.leakageCoefficient = 0.16;
        p.compressorEfficiency = 0.58;
        p.maxBoost = units::pressure(10.0, units::psi);
        p.compressorDischargeVolume = units::volume(2.5, units::L);
        return p;
    }

    Supercharger::Parameters centrifugalParams() {
        Supercharger::Parameters p;
        p.type = Supercharger::Type::Centrifugal;
        p.driveRatio = 8.0;
        p.mechanicalEfficiency = 0.95;
        p.maxImpellerSpeed = units::rpm(60000.0);
        p.compressorFlowRateK = GasSystem::k_carb(700.0);
        p.compressorEfficiency = 0.74;
        p.maxBoost = units::pressure(12.0, units::psi);
        p.compressorDischargeVolume = units::volume(1.5, units::L);
        return p;
    }
}

// The whole point of this feature request: a supercharger has to pull real
// torque off the crank, not just raise plenum pressure for free. Spin a
// Roots blower at a realistic idle-ish crank speed and confirm both halves
// actually happen -- boost builds AND a nonzero load torque shows up.
TEST(SuperchargerTests, PositiveDisplacementBuildsBoostAndAppliesCrankLoad) {
    Crankshaft crankshaft;
    Supercharger supercharger;
    supercharger.initialize(rootsParams());
    supercharger.setCrankshaft(&crankshaft);

    crankshaft.m_body.v_theta = -units::rpm(3000.0); // this sim runs negative

    const double dt = 1.0 / (4000.0 * 16.0);
    bool blewUp = false;
    for (int i = 0; i < 20000 && !blewUp; ++i) {
        supercharger.process(dt);
        if (!std::isfinite(supercharger.getPressureRatio())
            || !std::isfinite(supercharger.getLoadTorque()))
        {
            blewUp = true;
        }
    }

    ASSERT_FALSE(blewUp) << "supercharger state diverged";
    EXPECT_GT(supercharger.getPressureRatio(), 1.02)
        << "Roots blower failed to build any boost at 3000 rpm";
    EXPECT_GT(supercharger.getLoadTorque(), 0.0)
        << "Roots blower reported zero crank load despite compressing air";
}

// apply() is what actually gets this load onto the crank's rigid body each
// physics step -- confirm it writes a torque that opposes rotation
// (a drag), in both spin directions, and that it's a no-op with no
// crankshaft attached rather than crashing.
TEST(SuperchargerTests, ApplyWritesOpposingTorqueOntoCrankshaft) {
    Crankshaft crankshaft;
    crankshaft.m_body.index = 0;

    Supercharger supercharger;
    supercharger.initialize(rootsParams());
    supercharger.setCrankshaft(&crankshaft);
    crankshaft.m_body.v_theta = -units::rpm(4000.0);

    const double dt = 1.0 / (4000.0 * 16.0);
    for (int i = 0; i < 5000; ++i) {
        supercharger.process(dt);
    }
    ASSERT_GT(supercharger.getLoadTorque(), 0.0);

    atg_scs::SystemState state;
    state.resize(1, 0);

    // Spinning in this sim's normal (negative) direction: a drag should
    // push torque positive (opposing the negative velocity).
    state.t[0] = 0.0;
    state.v_theta[0] = -units::rpm(4000.0);
    supercharger.apply(&state);
    EXPECT_NEAR(state.t[0], supercharger.getLoadTorque(), 1e-9);

    // Spinning the other way: the drag should flip sign right along with
    // it, since it always opposes whichever way the crank is turning.
    state.t[0] = 0.0;
    state.v_theta[0] = units::rpm(4000.0);
    supercharger.apply(&state);
    EXPECT_NEAR(state.t[0], -supercharger.getLoadTorque(), 1e-9);

    state.destroy();

    // No crankshaft attached (e.g. before the DSL layer wires one up) must
    // not crash and must not write anything.
    Supercharger unattached;
    unattached.initialize(rootsParams());

    atg_scs::SystemState state2;
    state2.resize(1, 0);
    state2.t[0] = 0.0;
    state2.v_theta[0] = -units::rpm(4000.0);
    unattached.apply(&state2);
    EXPECT_EQ(state2.t[0], 0.0);
    state2.destroy();
}

// The defining qualitative difference between a positive-displacement
// blower and a centrifugal one: a PD unit's flow (and hence boost) is set
// by shaft speed and displacement almost linearly, so it builds meaningful
// boost even down at idle-ish RPM, while a centrifugal unit's boost
// follows a speed-squared compressor characteristic and stays close to
// atmospheric until the shaft is spinning fast. Confirm both halves of
// that contrast, not just that both eventually build "some" boost.
TEST(SuperchargerTests, CentrifugalBoostLagsPositiveDisplacementAtLowRpm) {
    Crankshaft crankshaft;
    Supercharger pd;
    pd.initialize(rootsParams());
    pd.setCrankshaft(&crankshaft);

    Supercharger centrifugal;
    centrifugal.initialize(centrifugalParams());
    centrifugal.setCrankshaft(&crankshaft);

    crankshaft.m_body.v_theta = -units::rpm(1200.0); // idle-ish

    const double dt = 1.0 / (4000.0 * 16.0);
    for (int i = 0; i < 20000; ++i) {
        pd.process(dt);
        centrifugal.process(dt);
    }

    const double pdBoostPsi = pd.getBoostPressure() / units::psi;
    const double centrifugalBoostPsi = centrifugal.getBoostPressure() / units::psi;

    EXPECT_GT(pdBoostPsi, 1.0)
        << "PD blower should already be making meaningful boost at idle-ish rpm";
    EXPECT_LT(centrifugalBoostPsi, pdBoostPsi)
        << "centrifugal blower should be building boost much more slowly than "
           "a PD unit at the same low rpm";

    // Now confirm the centrifugal unit actually can build real boost --
    // just not until the shaft is spinning fast, unlike the PD unit above.
    Supercharger centrifugalHighRpm;
    centrifugalHighRpm.initialize(centrifugalParams());
    centrifugalHighRpm.setCrankshaft(&crankshaft);
    crankshaft.m_body.v_theta = -units::rpm(6500.0);

    for (int i = 0; i < 20000; ++i) {
        centrifugalHighRpm.process(dt);
    }

    EXPECT_GT(centrifugalHighRpm.getBoostPressure() / units::psi, 5.0)
        << "centrifugal blower failed to build significant boost at high rpm";
}

namespace {
    Supercharger::Parameters twinchargerBlowerParams() {
        Supercharger::Parameters p;
        p.type = Supercharger::Type::Roots;
        p.driveRatio = 2.7;
        p.mechanicalEfficiency = 0.88;
        p.displacement = units::volume(0.65, units::L);
        p.volumetricEfficiency = 0.78;
        p.leakageCoefficient = 0.14;
        p.compressorEfficiency = 0.58;
        p.maxBoost = units::pressure(8.0, units::psi);
        p.compressorDischargeVolume = units::volume(0.5, units::L);
        p.clutched = true;
        p.clutchEngageRpm = units::rpm(3000.0);
        p.clutchDisengageRpm = units::rpm(3500.0);
        return p;
    }
}

// A twincharger's blower stage only covers low RPM -- below
// clutchEngageRpm it should behave exactly like a normal always-on
// Roots blower (real load, real boost), and above clutchDisengageRpm the
// clutch should open (zero load -- the whole point of a clutch, versus
// leaving it belt-driven and just idling the compression math) and the
// bypass should relax the discharge back toward atmosphere.
TEST(SuperchargerTests, ClutchDisengagesAboveThresholdAndDropsLoad) {
    Crankshaft crankshaft;
    Supercharger blower;
    blower.initialize(twinchargerBlowerParams());
    blower.setCrankshaft(&crankshaft);

    const double dt = 1.0 / (4000.0 * 16.0);

    crankshaft.m_body.v_theta = -units::rpm(2000.0);
    for (int i = 0; i < 20000; ++i) {
        blower.process(dt);
    }

    ASSERT_TRUE(blower.isClutchEngaged())
        << "clutch should still be engaged well below clutch_engage_rpm";
    EXPECT_GT(blower.getLoadTorque(), 0.0)
        << "engaged clutch should still be pulling real torque off the crank";
    EXPECT_GT(blower.getBoostPressure() / units::psi, 0.5)
        << "engaged blower should be making boost at low rpm";

    crankshaft.m_body.v_theta = -units::rpm(5000.0);
    for (int i = 0; i < 20000; ++i) {
        blower.process(dt);
    }

    EXPECT_FALSE(blower.isClutchEngaged())
        << "clutch should have disengaged well above clutch_disengage_rpm";
    EXPECT_EQ(blower.getLoadTorque(), 0.0)
        << "disengaged clutch must impose zero crank load";
    EXPECT_LT(blower.getBoostPressure() / units::psi, 1.0)
        << "disengaged (bypassed) blower should relax back toward atmosphere";
}

