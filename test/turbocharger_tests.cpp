#include <gtest/gtest.h>

#include "../include/turbocharger.h"
#include "../include/exhaust_system.h"
#include "../include/intake.h"
#include "../include/supercharger.h"
#include "../include/crankshaft.h"
#include "../include/units.h"

#include <cmath>
#include <vector>

// Exercises the Moore-Greitzer-style surge model end to end: spool the
// shaft up at wide-open throttle, then slam the throttle shut while it's
// still spun up (the classic surge trigger) and confirm the coupled
// flow/plenum-pressure state stays finite and actually reports backflow,
// rather than silently saturating or diverging.
TEST(TurbochargerTests, SurgeOnThrottleSnapIsStableAndDetected) {
    ExhaustSystem exhaust;
    ExhaustSystem::Parameters exhaustParams;
    exhaustParams.length = units::distance(0.3, units::m);
    exhaustParams.collectorCrossSectionArea = units::area(10.0, units::cm2);
    exhaustParams.outletFlowRate = GasSystem::k_carb(1000.0);
    exhaustParams.primaryTubeLength = units::distance(0.3, units::m);
    exhaustParams.primaryFlowRate = GasSystem::k_carb(1000.0);
    exhaustParams.velocityDecay = 0.5;
    exhaustParams.audioVolume = 1.0;
    exhaustParams.impulseResponse = nullptr;
    exhaust.initialize(exhaustParams);

    Intake intake;
    Intake::Parameters intakeParams;
    intakeParams.volume = units::volume(2.0, units::L);
    intakeParams.CrossSectionArea = units::area(10.0, units::cm2);
    intakeParams.InputFlowK = GasSystem::k_carb(800.0);
    intakeParams.IdleFlowK = 0.0;
    intakeParams.RunnerFlowRate = GasSystem::k_carb(800.0);
    intake.initialize(intakeParams);

    Turbocharger turbo;
    Turbocharger::Parameters turboParams;
    turboParams.turbineInertia = 0.00006;
    turboParams.turbineRateK = GasSystem::k_carb(220.0);
    turboParams.turbineEfficiency = 0.65;
    turboParams.compressorEfficiency = 0.72;
    turboParams.maxBoost = units::pressure(18.0, units::psi);
    turboParams.spoolSpeed = units::rpm(130000.0);
    turboParams.spinDownTime = 0.8;
    turbo.initialize(turboParams);

    turbo.addExhaustSystem(&exhaust);
    turbo.setIntake(&intake);
    intake.addTurbocharger(&turbo);

    const double dt = 1.0 / (4000.0 * 16.0);

    GasSystem::Mix exhaustMix;
    exhaustMix.p_inert = 1.0;

    bool blewUp = false;
    auto checkFinite = [&]() {
        if (!std::isfinite(turbo.getShaftSpeed())
            || !std::isfinite(turbo.getPressureRatio())
            || turbo.getShaftSpeed() < 0.0)
        {
            blewUp = true;
        }
    };

    // Phase 1: wide-open throttle (m_throttle is a restriction value --
    // 0 = wide open, 1 = fully closed) against a sustained hot exhaust
    // supply, to spool the shaft and build boost.
    intake.m_throttle = 0.0;
    for (int i = 0; i < 20000 && !blewUp; ++i) {
        exhaust.getSystem()->reset(
            units::pressure(3.0, units::atm), units::celcius(900.0), exhaustMix);

        intake.process(dt);
        turbo.process(dt);
        checkFinite();
    }

    ASSERT_FALSE(blewUp) << "turbo state diverged during spool-up";
    EXPECT_GT(turbo.getPressureRatio(), 1.05) << "turbo failed to build any boost";

    // Regression: the compressor flow that feeds back into next-substep's
    // pressure characteristic used to be assigned directly from the raw,
    // undamped per-substep flow, closing a one-step-delayed loop that
    // chattered -- isSurging() flipping true/false on literally every
    // single substep, even at sustained wide-open throttle (nothing to do
    // with the throttle-snap surge trigger this test exercises below).
    // Confirm that no longer happens once spooled up and holding steady.
    int surgeFlips = 0;
    bool lastSurging = turbo.isSurging();
    for (int i = 0; i < 2000 && !blewUp; ++i) {
        exhaust.getSystem()->reset(
            units::pressure(3.0, units::atm), units::celcius(900.0), exhaustMix);

        intake.process(dt);
        turbo.process(dt);
        checkFinite();

        const bool surging = turbo.isSurging();
        if (surging != lastSurging) {
            ++surgeFlips;
            lastSurging = surging;
        }
    }

    ASSERT_FALSE(blewUp) << "turbo state diverged while holding steady WOT";
    EXPECT_LT(surgeFlips, 200)
        << "compressor flow direction is chattering substep-to-substep at "
           "sustained WOT instead of holding steady";

    // Phase 2: snap the throttle shut while still spun up.
    intake.m_throttle = 1.0;
    bool sawSurge = false;
    for (int i = 0; i < 20000 && !blewUp; ++i) {
        exhaust.getSystem()->reset(
            units::pressure(3.0, units::atm), units::celcius(900.0), exhaustMix);

        intake.process(dt);
        turbo.process(dt);
        checkFinite();

        if (turbo.isSurging()) {
            sawSurge = true;
        }
    }

    ASSERT_FALSE(blewUp) << "turbo state diverged after throttle snap";
    EXPECT_TRUE(sawSurge)
        << "expected compressor backflow after slamming the throttle shut at boost";
}

// Regression covering both directions of a boost-regulation bug, with a
// real Intake attached and consuming from the compressor discharge plenum
// (not just a bare turbo+exhaust) -- the underboost half of this only shows
// up under real consumption, see below.
//
// Overboost: the wastegate freeze compared shutoffHead (the compressor
// characteristic's zero-flow asymptote) against max_boost, but the pressure
// ratio actually delivered to m_compressorDischarge is shutoffHead plus a
// "hump" term that can add up to 25% on top -- so a turbo spooled right up
// against its setpoint could deliver boost well past max_boost whenever
// flow sat near the hump's peak (reported as an 18 psi setpoint reading
// ~22 psi under load).
//
// Underboost: fixing the above by hard-clamping the compressor *source*
// pressure at exactly max_boost introduced the opposite bug -- the source
// feeds m_compressorDischarge by flowing against a pressure differential,
// and the intake is simultaneously draining that same plenum. A source
// capped at exactly the target can only sustain a plenum pressure *below*
// the cap in steady state (zero differential -> zero inflow), so with a
// real intake attached and consuming boost, delivered pressure settled
// well under max_boost (reported as an 18 psi setpoint settling around
// 14.5 psi) even though the source itself was pegged right at the limit.
// A bare turbo+exhaust test with no consumer can't see this at all.
TEST(TurbochargerTests, BoostNeverExceedsMaxBoost) {
    ExhaustSystem exhaust;
    ExhaustSystem::Parameters exhaustParams;
    exhaustParams.length = units::distance(0.3, units::m);
    exhaustParams.collectorCrossSectionArea = units::area(10.0, units::cm2);
    exhaustParams.outletFlowRate = GasSystem::k_carb(1000.0);
    exhaustParams.primaryTubeLength = units::distance(0.3, units::m);
    exhaustParams.primaryFlowRate = GasSystem::k_carb(1000.0);
    exhaustParams.velocityDecay = 0.5;
    exhaustParams.audioVolume = 1.0;
    exhaustParams.impulseResponse = nullptr;
    exhaust.initialize(exhaustParams);

    Intake intake;
    Intake::Parameters intakeParams;
    intakeParams.volume = units::volume(2.0, units::L);
    intakeParams.CrossSectionArea = units::area(10.0, units::cm2);
    intakeParams.InputFlowK = GasSystem::k_carb(800.0);
    intakeParams.IdleFlowK = 0.0;
    intakeParams.RunnerFlowRate = GasSystem::k_carb(800.0);
    intake.initialize(intakeParams);
    intake.m_throttle = 0.0; // wide open -- maximum consumption from the plenum

    Turbocharger turbo;
    Turbocharger::Parameters turboParams;
    turboParams.turbineInertia = 0.00006;
    turboParams.turbineRateK = GasSystem::k_carb(220.0);
    turboParams.turbineEfficiency = 0.65;
    turboParams.compressorEfficiency = 0.72;
    turboParams.maxBoost = units::pressure(18.0, units::psi);
    turboParams.spoolSpeed = units::rpm(130000.0);
    turboParams.spinDownTime = 0.8;
    turbo.initialize(turboParams);
    turbo.addExhaustSystem(&exhaust);
    turbo.setIntake(&intake);
    intake.addTurbocharger(&turbo);

    const double dt = 1.0 / (4000.0 * 16.0);
    GasSystem::Mix exhaustMix;
    exhaustMix.p_inert = 1.0;

    double peakBoost = 0.0;
    for (int i = 0; i < 40000; ++i) {
        exhaust.getSystem()->reset(
            units::pressure(3.0, units::atm), units::celcius(900.0), exhaustMix);
        intake.process(dt);
        turbo.process(dt);
        peakBoost = std::max(peakBoost, turbo.getBoostPressure());
    }

    EXPECT_LE(peakBoost, turboParams.maxBoost * 1.15)
        << "boost peaked at " << (peakBoost / units::psi)
        << " psi against an " << (turboParams.maxBoost / units::psi)
        << " psi max_boost setpoint";

    const double finalBoost = turbo.getBoostPressure();
    EXPECT_GE(finalBoost, turboParams.maxBoost * 0.85)
        << "boost settled at " << (finalBoost / units::psi)
        << " psi against an " << (turboParams.maxBoost / units::psi)
        << " psi max_boost setpoint -- turbo is underboosting";
}

namespace {
    Turbocharger::Parameters matchedTurboParams() {
        Turbocharger::Parameters p;
        p.turbineInertia = 0.00006;
        p.turbineRateK = GasSystem::k_carb(220.0);
        p.turbineEfficiency = 0.65;
        p.compressorEfficiency = 0.72;
        p.maxBoost = units::pressure(18.0, units::psi);
        p.spoolSpeed = units::rpm(130000.0);
        p.spinDownTime = 0.8;
        return p;
    }

}

// Regression: Intake used to hold a single Turbocharger* that a second
// addTurbocharger()/setTurbocharger() call on the same intake would just
// overwrite, so a twin-turbo engine's two turbos (RB26DETT, 2JZ-GTE, ...)
// sharing one intake would silently starve the first-attached turbo's
// compressor discharge -- Intake::process() would only ever read from and
// drain the last-attached one.
//
// Tests this directly against Intake::process() rather than through a full
// turbo spool-up (whose own self-regulation dynamics -- see
// BoostNeverExceedsMaxBoost's doc comment -- make "how much boost did it
// reach" a noisy, indirect proxy for "did the intake actually drain it").
// Seed both turbos' compressor discharge plenums directly at an elevated
// pressure (standing in for "already spooled up"), attach both to one
// intake with the throttle wide open, and run only Intake::process() --
// confirm *both* discharges lose moles, not just one.
TEST(TurbochargerTests, TwinTurbosOnOneIntakeBothGetDrained) {
    Intake intake;
    Intake::Parameters intakeParams;
    intakeParams.volume = units::volume(2.0, units::L);
    intakeParams.CrossSectionArea = units::area(10.0, units::cm2);
    intakeParams.InputFlowK = GasSystem::k_carb(800.0);
    intakeParams.IdleFlowK = 0.0;
    intakeParams.RunnerFlowRate = GasSystem::k_carb(800.0);
    intake.initialize(intakeParams);
    intake.m_throttle = 0.0; // wide open -- maximum consumption

    Turbocharger turboA, turboB;
    turboA.initialize(matchedTurboParams());
    turboB.initialize(matchedTurboParams());

    // Seed each discharge directly -- no exhaust/spool-up needed, since
    // Intake::process() only ever reads/drains the discharge plenum, never
    // calls Turbocharger::process() itself.
    GasSystem::Mix airMix;
    airMix.p_inert = 1.0;
    turboA.getCompressorDischargeSystem()->reset(
        units::pressure(2.0, units::atm), units::celcius(60.0), airMix);
    turboB.getCompressorDischargeSystem()->reset(
        units::pressure(2.0, units::atm), units::celcius(60.0), airMix);

    turboA.setIntake(&intake);
    turboB.setIntake(&intake);
    intake.addTurbocharger(&turboA); // first-attached -- the one the old bug starved
    intake.addTurbocharger(&turboB);

    const double nBeforeA = turboA.getCompressorDischargeSystem()->n();
    const double nBeforeB = turboB.getCompressorDischargeSystem()->n();
    ASSERT_GT(nBeforeA, 0.0);
    ASSERT_GT(nBeforeB, 0.0);

    const double dt = 1.0 / (4000.0 * 16.0);
    for (int i = 0; i < 1000; ++i) {
        intake.process(dt);
    }

    const double nAfterA = turboA.getCompressorDischargeSystem()->n();
    const double nAfterB = turboB.getCompressorDischargeSystem()->n();

    EXPECT_LT(nAfterA, nBeforeA * 0.999)
        << "first-attached turbo's discharge was never drawn from -- looks "
           "unconnected to the shared intake";
    EXPECT_LT(nAfterB, nBeforeB * 0.999)
        << "second-attached turbo's discharge was never drawn from -- looks "
           "unconnected to the shared intake";
}

namespace {
    // Spools a turbo with the given inducer/exducer diameters (defaults if
    // 0) under sustained hot exhaust for a fixed duration and returns the
    // resulting shaft speed. Everything else held identical across calls
    // so the only variable is wheel size.
    double spoolShaftSpeed(double inducerDiameter, double exducerDiameter) {
        ExhaustSystem exhaust;
        ExhaustSystem::Parameters exhaustParams;
        exhaustParams.length = units::distance(0.3, units::m);
        exhaustParams.collectorCrossSectionArea = units::area(10.0, units::cm2);
        exhaustParams.outletFlowRate = GasSystem::k_carb(1000.0);
        exhaustParams.primaryTubeLength = units::distance(0.3, units::m);
        exhaustParams.primaryFlowRate = GasSystem::k_carb(1000.0);
        exhaustParams.velocityDecay = 0.5;
        exhaustParams.audioVolume = 1.0;
        exhaustParams.impulseResponse = nullptr;
        exhaust.initialize(exhaustParams);

        Turbocharger turbo;
        Turbocharger::Parameters turboParams;
        turboParams.turbineInertia = 0.00006;
        turboParams.turbineRateK = GasSystem::k_carb(220.0);
        turboParams.turbineEfficiency = 0.65;
        turboParams.compressorEfficiency = 0.72;
        turboParams.maxBoost = units::pressure(18.0, units::psi);
        turboParams.spoolSpeed = units::rpm(130000.0);
        turboParams.spinDownTime = 0.8;
        if (inducerDiameter > 0.0) turboParams.turbineInducerDiameter = inducerDiameter;
        if (exducerDiameter > 0.0) turboParams.turbineExducerDiameter = exducerDiameter;
        turbo.initialize(turboParams);

        turbo.addExhaustSystem(&exhaust);

        const double dt = 1.0 / (4000.0 * 16.0);
        GasSystem::Mix exhaustMix;
        exhaustMix.p_inert = 1.0;

        for (int i = 0; i < 5000; ++i) {
            exhaust.getSystem()->reset(
                units::pressure(3.0, units::atm), units::celcius(900.0), exhaustMix);
            turbo.process(dt);
        }

        return turbo.getShaftSpeed();
    }
}

// The whole point of exposing inducer/exducer diameter is that they aren't
// just labels -- a bigger wheel should measurably change spool behavior:
// more inertia (slower to spin up) from a bigger inducer, more flow
// capacity (faster to spin up, same nominal turbine_flow_rate) from a
// bigger exducer.
TEST(TurbochargerTests, WheelSizeAffectsSpoolRate) {
    const double baseline = spoolShaftSpeed(0.0, 0.0);
    const double biggerInducer = spoolShaftSpeed(
        2.0 * Turbocharger::BaselineTurbineInducerDiameter, 0.0);
    const double biggerExducer = spoolShaftSpeed(
        0.0, 2.0 * Turbocharger::BaselineTurbineExducerDiameter);

    ASSERT_TRUE(std::isfinite(baseline));
    ASSERT_TRUE(std::isfinite(biggerInducer));
    ASSERT_TRUE(std::isfinite(biggerExducer));

    EXPECT_LT(biggerInducer, baseline)
        << "a bigger inducer (more rotational inertia) should spool slower";
    EXPECT_GT(biggerExducer, baseline)
        << "a bigger exducer (more flow capacity) should spool faster";
}

// recoverSpecParameters() exists so a live editor can seed its fields from
// an already-running (and therefore already wheel-scaled) turbo and then
// re-apply edits via initialize() without compounding the scale factor
// further on every edit -- see TurbochargerEditorCluster. Confirm both
// halves of that: recovering reproduces the original spec, and re-applying
// the recovered spec reproduces the same runtime state (idempotent, not
// compounding).
TEST(TurbochargerTests, RecoverSpecParametersUndoesWheelScaleExactly) {
    Turbocharger::Parameters spec;
    spec.turbineInertia = 0.00006;
    spec.turbineRateK = GasSystem::k_carb(220.0);
    spec.turbineEfficiency = 0.65;
    spec.compressorEfficiency = 0.72;
    spec.maxBoost = units::pressure(18.0, units::psi);
    spec.spoolSpeed = units::rpm(130000.0);
    spec.spinDownTime = 0.8;
    spec.turbineInducerDiameter = 1.3 * Turbocharger::BaselineTurbineInducerDiameter;
    spec.turbineExducerDiameter = 0.8 * Turbocharger::BaselineTurbineExducerDiameter;

    Turbocharger turbo;
    turbo.initialize(spec);

    const Turbocharger::Parameters recovered = turbo.recoverSpecParameters();
    EXPECT_NEAR(recovered.turbineRateK, spec.turbineRateK, spec.turbineRateK * 1e-9);
    EXPECT_NEAR(recovered.turbineInertia, spec.turbineInertia, spec.turbineInertia * 1e-9);

    // Re-applying the recovered spec must reproduce the exact same
    // wheel-scaled runtime values as the original initialize() call --
    // if recoverSpecParameters() were a no-op (or wrong), this second
    // initialize() would scale turbineRateK/turbineInertia a second time.
    const double liveRateKBefore = turbo.getParameters().turbineRateK;
    const double liveInertiaBefore = turbo.getParameters().turbineInertia;

    turbo.initialize(recovered);

    EXPECT_NEAR(turbo.getParameters().turbineRateK, liveRateKBefore, liveRateKBefore * 1e-9);
    EXPECT_NEAR(turbo.getParameters().turbineInertia, liveInertiaBefore, liveInertiaBefore * 1e-9);
}

// Twincharger series staging: a turbo fed by an upstream supercharger
// (setUpstreamSupercharger()) should end up with MORE boost than the same
// turbo breathing flat atmosphere, under identical exhaust conditions,
// because the two stages' pressure ratios multiply rather than either one
// working in isolation. This is the whole point of setUpstreamSupercharger
// -- confirm it actually changes the outcome, not just that it compiles.
TEST(TurbochargerTests, UpstreamSuperchargerIncreasesDeliveredBoost) {
    auto spoolWithOptionalSupercharger = [](bool withSupercharger) {
        ExhaustSystem exhaust;
        ExhaustSystem::Parameters exhaustParams;
        exhaustParams.length = units::distance(0.3, units::m);
        exhaustParams.collectorCrossSectionArea = units::area(10.0, units::cm2);
        exhaustParams.outletFlowRate = GasSystem::k_carb(1000.0);
        exhaustParams.primaryTubeLength = units::distance(0.3, units::m);
        exhaustParams.primaryFlowRate = GasSystem::k_carb(1000.0);
        exhaustParams.velocityDecay = 0.5;
        exhaustParams.audioVolume = 1.0;
        exhaustParams.impulseResponse = nullptr;
        exhaust.initialize(exhaustParams);

        Turbocharger turbo;
        Turbocharger::Parameters turboParams;
        turboParams.turbineInertia = 0.00006;
        turboParams.turbineRateK = GasSystem::k_carb(220.0);
        turboParams.turbineEfficiency = 0.65;
        turboParams.compressorEfficiency = 0.72;
        turboParams.maxBoost = units::pressure(18.0, units::psi);
        turboParams.spoolSpeed = units::rpm(130000.0);
        turboParams.spinDownTime = 0.8;
        turbo.initialize(turboParams);
        turbo.addExhaustSystem(&exhaust);

        Crankshaft crankshaft;
        Supercharger blower;
        if (withSupercharger) {
            Supercharger::Parameters blowerParams;
            blowerParams.type = Supercharger::Type::Roots;
            blowerParams.driveRatio = 2.7;
            blowerParams.mechanicalEfficiency = 0.88;
            blowerParams.displacement = units::volume(0.65, units::L);
            blowerParams.volumetricEfficiency = 0.78;
            blowerParams.leakageCoefficient = 0.14;
            blowerParams.compressorEfficiency = 0.58;
            blowerParams.maxBoost = units::pressure(8.0, units::psi);
            blowerParams.compressorDischargeVolume = units::volume(0.5, units::L);
            blower.initialize(blowerParams);
            blower.setCrankshaft(&crankshaft);
            crankshaft.m_body.v_theta = -units::rpm(4000.0);
            turbo.setUpstreamSupercharger(&blower);
        }

        const double dt = 1.0 / (4000.0 * 16.0);
        GasSystem::Mix exhaustMix;
        exhaustMix.p_inert = 1.0;

        for (int i = 0; i < 20000; ++i) {
            exhaust.getSystem()->reset(
                units::pressure(3.0, units::atm), units::celcius(900.0), exhaustMix);
            if (withSupercharger) {
                blower.process(dt);
            }
            turbo.process(dt);
        }

        return turbo.getBoostPressure();
    };

    const double boostAlone = spoolWithOptionalSupercharger(false);
    const double boostWithBlower = spoolWithOptionalSupercharger(true);

    ASSERT_TRUE(std::isfinite(boostAlone));
    ASSERT_TRUE(std::isfinite(boostWithBlower));
    EXPECT_GT(boostWithBlower, boostAlone)
        << "turbo fed by an upstream supercharger should deliver more boost "
           "than the same turbo breathing flat atmosphere -- staging isn't "
           "actually multiplying the two stages";
}
