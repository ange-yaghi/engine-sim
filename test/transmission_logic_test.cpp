#include <gtest/gtest.h>

#include "../include/transmission.h"
#include "../include/vehicle.h"
#include "../include/engine.h"
#include "../include/crankshaft.h"
#include "../include/units.h"

#include <algorithm>
#include <cmath>

namespace {
    // Transmission::addToSystem() only needs an Engine to read
    // getOutputCrankshaft()->m_body off of (to hook up the clutch/
    // converter constraint's body1) and, for the TCU/converter logic,
    // getThrottle(). These tests drive Transmission::update() directly and
    // never step the constraint solver, so a bare stub with a real
    // Crankshaft is enough -- no full Engine::initialize() required.
    class StubEngine : public Engine {
    public:
        Crankshaft *getOutputCrankshaft() const override { return &m_stubCrankshaft; }
        void setRedline(double redline) { m_redline = redline; }
        mutable Crankshaft m_stubCrankshaft;
    };

    // m_rotatingMass->I encodes the current effective gear ratio (see
    // Transmission::applyGearRatio()) -- read it back out the same way, so
    // tests can observe the CVT's continuous ratio without needing a
    // test-only getter on Transmission.
    double impliedGearRatio(const atg_scs::RigidBody &rotatingMass, const Vehicle &vehicle) {
        return vehicle.getTireRadius()
            / (vehicle.getDiffRatio() * std::sqrt(rotatingMass.I / vehicle.getMass()));
    }

    struct Rig {
        atg_scs::RigidBody rotatingMass;
        atg_scs::RigidBodySystem system;
        Vehicle vehicle;
        StubEngine engine;
        Transmission transmission;

        double gearRatios[4] = { 3.5, 2.0, 1.3, 1.0 };

        Rig(Transmission::Type type, Transmission::Parameters tp) {
            Vehicle::Parameters vp{};
            vp.mass = 1500.0;
            vp.dragCoefficient = 0.3;
            vp.crossSectionArea = 2.0;
            vp.diffRatio = 3.5;
            vp.tireRadius = 0.3;
            vp.rollingResistance = 300.0;
            vehicle.initialize(vp);
            vehicle.addToSystem(nullptr, &rotatingMass);

            tp.GearCount = 4;
            tp.GearRatios = gearRatios;
            tp.TransmissionType = type;
            transmission.initialize(tp);
            transmission.addToSystem(&system, &rotatingMass, &vehicle, &engine);
        }

        // Seeds the rig's kinetic energy so the vehicle reads back at
        // `speed` m/s once a gear is engaged (independent of which ratio
        // ends up applied -- Transmission::applyGearRatio() conserves this
        // energy across ratio changes, same as a real shift not changing
        // road speed instantaneously).
        void setVehicleSpeed(double speed) {
            const double Er = 0.5 * vehicle.getMass() * speed * speed;
            rotatingMass.I = 1.0;
            rotatingMass.v_theta = std::sqrt(2.0 * Er / rotatingMass.I);
        }

        // Puts the engine and the transmission input shaft at the same speed
        // -- a car actually cruising along in gear. Tests that drive the
        // shift schedule by writing rotatingMass.v_theta on its own leave the
        // stub crankshaft behind at zero, which the launch clutch and
        // converter both read as a permanently slipping launch rather than
        // steady cruise, since what they care about is the difference.
        void setDrivetrainSpeed(double v_theta) {
            rotatingMass.v_theta = v_theta;
            engine.m_stubCrankshaft.m_body.v_theta = v_theta;
        }

        // Runs update() for `steps` ticks of `dt`, keeping the stub crank
        // locked to the input shaft on every tick -- the steady-cruise
        // assumption setDrivetrainSpeed() documents, but sustained across a
        // whole run instead of a single snapshot. updateAutoShift() reads
        // the engine's own crank speed (not the input shaft -- see
        // Transmission::updateAutoShift()), and nothing in this Rig steps a
        // real constraint solver to keep the two in sync on its own, so a
        // multi-tick loop that fires a shift (which changes rotatingMass via
        // applyGearRatio()'s energy-conserving ratio swap) would otherwise
        // leave the crank stale and pinned above the shift point, cascading
        // into further unintended shifts. AutomaticTorqueConverter has no
        // equivalent of DualClutch's snap-to-new-gear -- this is the
        // logic-test stand-in for it.
        void runLocked(double dt, int steps) {
            for (int i = 0; i < steps; ++i) {
                engine.m_stubCrankshaft.m_body.v_theta = rotatingMass.v_theta;
                transmission.update(dt);
            }
        }
    };

    Transmission::Parameters baseParams() {
        Transmission::Parameters tp{};
        tp.MaxClutchTorque = units::torque(300.0, units::ft_lb);
        // Closed-throttle == WOT by default -- most of these tests don't
        // care about the throttle blend, so this keeps them a single
        // deterministic shift point (StubEngine's default getThrottle() is
        // 0, i.e. WOT, so this also avoids every non-throttle-aware test
        // silently exercising the WOT half of the schedule only). Tests
        // that do care about the blend set their own distinct values (see
        // ShiftScheduleBlendsByThrottlePosition below).
        tp.ShiftUpRpmClosedThrottle = tp.ShiftUpRpmWOT = units::rpm(4000);
        tp.ShiftDownRpmClosedThrottle = tp.ShiftDownRpmWOT = units::rpm(1500);
        tp.SportShiftUpRpmClosedThrottle = tp.SportShiftUpRpmWOT = units::rpm(6000);
        tp.SportShiftDownRpmClosedThrottle = tp.SportShiftDownRpmWOT = units::rpm(3000);
        // Shift feel is per-transmission now, not implied by Type -- tests
        // that care about specific timing/cut numbers set their own.
        tp.ShiftTimeDrive = 0.3;
        tp.ShiftTimeSport = 0.2;
        tp.ShiftTorqueCut = 0.5;
        tp.CvtTargetRpm = units::rpm(2500);
        tp.CvtRatioRate = 5.0;
        return tp;
    }
}

TEST(TransmissionLogicTest, StartsInNeutralAndLadderWalksDriveSportManual) {
    Transmission::Parameters tp = baseParams();
    tp.HasSportMode = true;
    tp.AllowManualShift = true;
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);

    // Starts in neutral -- no auto-engage.
    ASSERT_EQ(rig.transmission.getGear(), -1);
    EXPECT_EQ(rig.transmission.getGearLabel(), "N");
    EXPECT_FALSE(rig.transmission.changeGear(0)) << "can't hand-pick a gear before Drive is engaged";

    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getGearLabel(), "D1");

    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getDriveMode(), Transmission::DriveMode::Sport);
    EXPECT_EQ(rig.transmission.getGearLabel(), "S1");

    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getDriveMode(), Transmission::DriveMode::Manual);
    EXPECT_EQ(rig.transmission.getGearLabel(), "M1");

    // In Manual, Up/Down are ordinary gear shifts.
    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getGearLabel(), "M2");
    ASSERT_TRUE(rig.transmission.shiftDown());
    EXPECT_EQ(rig.transmission.getGearLabel(), "M1");

    // Floor of Manual drops back down the ladder, one step per press.
    ASSERT_TRUE(rig.transmission.shiftDown());
    EXPECT_EQ(rig.transmission.getGearLabel(), "S1");
    ASSERT_TRUE(rig.transmission.shiftDown());
    EXPECT_EQ(rig.transmission.getGearLabel(), "D1");
    ASSERT_TRUE(rig.transmission.shiftDown());
    EXPECT_EQ(rig.transmission.getGearLabel(), "N");
}

TEST(TransmissionLogicTest, SportModeUnavailableWhenNotEquipped) {
    Transmission::Parameters tp = baseParams();
    tp.HasSportMode = false;
    tp.AllowManualShift = true;
    Rig rig(Transmission::Type::DualClutch, tp);
    rig.setVehicleSpeed(15.0);

    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getGearLabel(), "D1");

    // No Sport equipped -- Up from Drive should go straight to Manual.
    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getDriveMode(), Transmission::DriveMode::Manual);
    EXPECT_NE(rig.transmission.getDriveMode(), Transmission::DriveMode::Sport);
}

TEST(TransmissionLogicTest, DumbOldAutomaticHasOnlyDrive) {
    Transmission::Parameters tp = baseParams();
    tp.HasSportMode = false;
    tp.AllowManualShift = false;
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);

    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getGearLabel(), "D1");

    // Nothing past Drive -- no Sport, no Manual.
    EXPECT_FALSE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getDriveMode(), Transmission::DriveMode::Drive);
}

TEST(TransmissionLogicTest, ShiftPrepareTimeDefaultsToInstantSameTickShift) {
    // baseParams() never sets ShiftPrepareTimeDrive/Sport -- confirms the
    // zero default is a true same-tick passthrough, same as every file
    // that predates requestShift().
    Transmission::Parameters tp = baseParams();
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGear(), 0);

    // Let the initial Neutral -> Drive engage's own cooldown floor
    // (kAutoShiftCooldownFloor) clear before triggering the real test --
    // same reason AutomaticShiftsByRpmWithCooldown runs its loop for a
    // full second rather than a single tick.
    for (int i = 0; i < 10; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);

    rig.setDrivetrainSpeed(units::rpm(4500));
    rig.transmission.update(0.05);
    EXPECT_EQ(rig.transmission.getGear(), 1) << "should swap the same tick the decision is made";
}

TEST(TransmissionLogicTest, ShiftPrepareTimeDelaysTheRatioSwapBeforeAnyDip) {
    // Models the real mechanical lag between a valve body/TCU deciding to
    // shift and the clutch pack/band actually finishing its apply stroke
    // -- distinct from (and prior to) ShiftTimeDrive's own post-swap dip.
    Transmission::Parameters tp = baseParams();
    tp.ShiftPrepareTimeDrive = 0.3;
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGear(), 0);

    // Let the initial Neutral -> Drive engage's own cooldown floor clear
    // first (see ShiftPrepareTimeDefaultsToInstantSameTickShift).
    for (int i = 0; i < 10; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);

    rig.setDrivetrainSpeed(units::rpm(4500)); // above baseParams()'s 4000rpm shift-up point
    rig.transmission.update(0.05); // the decision is made this tick
    EXPECT_EQ(rig.transmission.getGear(), 0)
        << "the ratio shouldn't swap the instant the decision is made";

    for (int i = 0; i < 4; ++i) rig.transmission.update(0.05); // 0.2s elapsed of the 0.3s prepare time
    EXPECT_EQ(rig.transmission.getGear(), 0) << "still pending -- prepare time hasn't elapsed yet";

    rig.transmission.update(0.15); // now comfortably past the 0.3s prepare time
    EXPECT_EQ(rig.transmission.getGear(), 1) << "should finally swap once the prepare time elapses";
}

TEST(TransmissionLogicTest, ShiftPrepareTimeUsesTheSportValueInSportMode) {
    Transmission::Parameters tp = baseParams();
    tp.HasSportMode = true;
    tp.ShiftPrepareTimeDrive = 1.0; // deliberately huge -- shouldn't apply in Sport
    tp.ShiftPrepareTimeSport = 0.1;
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp(); // Neutral -> Drive
    ASSERT_TRUE(rig.transmission.shiftUp()); // Drive -> Sport
    ASSERT_EQ(rig.transmission.getDriveMode(), Transmission::DriveMode::Sport);
    ASSERT_EQ(rig.transmission.getGear(), 0);

    // Let the initial Neutral -> Drive engage's own cooldown floor clear
    // first (see ShiftPrepareTimeDefaultsToInstantSameTickShift).
    for (int i = 0; i < 10; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);

    rig.setDrivetrainSpeed(units::rpm(6500)); // above baseParams()'s 6000rpm sport shift-up point
    rig.transmission.update(0.05);
    EXPECT_EQ(rig.transmission.getGear(), 0) << "still pending right after the decision";

    rig.transmission.update(0.15); // past the 0.1s Sport prepare time, nowhere near the 1.0s Drive one
    EXPECT_EQ(rig.transmission.getGear(), 1)
        << "Sport's own (much shorter) prepare time should be what applies here";
}

TEST(TransmissionLogicTest, AutomaticShiftsByRpmWithCooldown) {
    Transmission::Parameters tp = baseParams();
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGear(), 0);

    const double dt = 0.05;

    // Force engine speed above the upshift threshold and let the cooldown
    // elapse -- expect exactly one upshift, not a cascade, since the shift
    // itself drops RPM back out of the upshift band (a taller gear means
    // lower engine speed for the same road speed).
    rig.rotatingMass.v_theta = units::rpm(4500);
    rig.runLocked(dt, 20);
    EXPECT_EQ(rig.transmission.getGear(), 1);

    rig.rotatingMass.v_theta = units::rpm(1000);
    rig.runLocked(dt, 20);
    EXPECT_EQ(rig.transmission.getGear(), 0);
}

TEST(TransmissionLogicTest, ShiftScheduleBlendsByThrottlePosition) {
    // A real transmission -- hydraulic/vacuum or electronic, doesn't
    // matter -- reads throttle position, not just RPM, to decide shift
    // points: closed throttle shifts early for economy, wide open
    // throttle (WOT) holds the gear for power. This applies uniformly to
    // every automated transmission type now (see updateAutoShift()),
    // not just ones with some special "electronic" flag.
    Transmission::Parameters tp = baseParams();
    tp.ShiftUpRpmClosedThrottle = units::rpm(4000);
    tp.ShiftUpRpmWOT = units::rpm(6000);
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();

    const double dt = 0.05;

    // Wide open throttle (0 == plate wide open) should blend the shift-up
    // point up toward ShiftUpRpmWOT (6000) -- 4500 RPM shouldn't be enough
    // to upshift here, even though it clears the closed-throttle 4000 RPM
    // point.
    rig.engine.setThrottle(0.0);
    rig.rotatingMass.v_theta = units::rpm(4500);
    rig.runLocked(dt, 20);
    EXPECT_EQ(rig.transmission.getGear(), 0) << "WOT should kick down/hold the gear, not short-shift";

    // Ease off the throttle -- same RPM should now be enough to upshift
    // against the un-blended, closed-throttle economy shift point.
    rig.engine.setThrottle(1.0);
    rig.runLocked(dt, 20);
    EXPECT_EQ(rig.transmission.getGear(), 1);
}

TEST(TransmissionLogicTest, PartThrottleBarelyMovesTheKickdownPointButWotStillReachesIt) {
    // The throttle blend is squared, not linear (see updateAutoShift()) --
    // a real kickdown line barely moves through most of the pedal's
    // travel and only ramps up near the floor. A linear blend meant any
    // moderate tip-in already pushed the downshift point most of the way
    // to its WOT value, kicking down far more readily than a real car.
    Transmission::Parameters tp = baseParams();
    tp.ShiftDownRpmClosedThrottle = units::rpm(1500);
    tp.ShiftDownRpmWOT = units::rpm(4500);
    tp.ShiftUpRpmClosedThrottle = tp.ShiftUpRpmWOT = units::rpm(9000); // keep the upshift point out of the way
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGear(), 0);

    rig.rotatingMass.v_theta = units::rpm(9500);
    rig.runLocked(0.05, 20); // force the upshift, let its cooldown clear
    ASSERT_EQ(rig.transmission.getGear(), 1);

    // Half throttle: linear blending would put the downshift point at
    // 3000 RPM (comfortably above 2600, so it'd kick down); squared
    // blending only pushes it to 2250 (below 2600), so it should hold.
    rig.engine.setThrottle(0.5);
    rig.rotatingMass.v_theta = units::rpm(2600);
    rig.runLocked(0.05, 10);
    EXPECT_EQ(rig.transmission.getGear(), 1)
        << "moderate throttle shouldn't already kick down almost to the WOT point";

    // Flooring it should still reach the full WOT kickdown point.
    rig.engine.setThrottle(0.0);
    rig.runLocked(0.05, 10);
    EXPECT_EQ(rig.transmission.getGear(), 0) << "WOT should still kick down";
}

TEST(TransmissionLogicTest, ShiftScheduleDoesntHuntAfterAWideRatioUpshift) {
    // A widely-spaced gear pair (Rig's own 1st/2nd, 3.5/2.0 == 1.75x --
    // similar to the 454 truck's TH400, 2.48/1.48 == 1.68x) means a WOT
    // upshift lands well below the upshift RPM once it's in the new gear.
    // shift_down_rpm_wot has to sit safely below that landing point, or
    // the transmission upshifts, immediately asks to downshift back, and
    // hunts between the two gears forever -- exactly what the th400 did
    // before its shift_down_rpm_* got tuned down to leave headroom.
    Transmission::Parameters tp = baseParams();
    tp.ShiftUpRpmClosedThrottle = units::rpm(1700);
    tp.ShiftUpRpmWOT = units::rpm(4200);
    tp.ShiftDownRpmClosedThrottle = units::rpm(650);
    tp.ShiftDownRpmWOT = units::rpm(1800);
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGear(), 0);

    rig.engine.setThrottle(0.0); // WOT, held for the whole test
    rig.rotatingMass.v_theta = units::rpm(4500);
    rig.runLocked(0.05, 20);
    ASSERT_EQ(rig.transmission.getGear(), 1) << "should have upshifted out of 1st";

    // Let the natural post-shift RPM (~2570, from applyGearRatio()'s own
    // energy-conserving ratio swap -- not forced here) keep simulating
    // well past the shift cooldown. A hunting schedule drops straight
    // back to 1st as soon as the cooldown clears.
    rig.runLocked(0.05, 40);
    EXPECT_EQ(rig.transmission.getGear(), 1) << "shouldn't immediately hunt back down to 1st";
}

TEST(TransmissionLogicTest, ShiftScheduleDoesntHuntAfterADownshiftLandsAboveTheUpshiftLine) {
    // The mirror image of ShiftScheduleDoesntHuntAfterAWideRatioUpshift:
    // here the schedule itself (both thresholds fixed, no throttle blend
    // to muddy it) has just barely enough headroom for a steady cruise,
    // but the natural post-upshift RPM already sits below shiftDownRpm --
    // so the instant the shift cooldown clears, the schedule wants to
    // downshift right back. Without the impliedInputSpeed(m_gear - 1) <
    // shiftUpRpm guard in updateAutoShift(), that downshift lands back
    // above shiftUpRpm (energy-conserving ratio swaps are reversible) and
    // immediately re-upshifts -- an infinite 1st/2nd hunt.
    Transmission::Parameters tp = baseParams();
    tp.ShiftUpRpmClosedThrottle = tp.ShiftUpRpmWOT = units::rpm(2500);
    tp.ShiftDownRpmClosedThrottle = tp.ShiftDownRpmWOT = units::rpm(1500);
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGear(), 0);

    rig.rotatingMass.v_theta = units::rpm(2600); // just over shiftUpRpm
    rig.runLocked(0.05, 20);
    ASSERT_EQ(rig.transmission.getGear(), 1)
        << "should have upshifted out of 1st (lands at ~1486rpm, already under shiftDownRpm)";

    // A hunting schedule drops straight back to 1st (landing at ~2600rpm,
    // right back over shiftUpRpm) as soon as the cooldown clears.
    rig.runLocked(0.05, 40);
    EXPECT_EQ(rig.transmission.getGear(), 1)
        << "shouldn't downshift into a gear it would immediately want to leave again";

    // Once road speed has actually dropped enough that 1st's implied RPM
    // clears the upshift line, the downshift the schedule's been asking
    // for the whole time should go ahead normally.
    rig.rotatingMass.v_theta = units::rpm(1400);
    rig.runLocked(0.05, 20);
    EXPECT_EQ(rig.transmission.getGear(), 0)
        << "should downshift once it wouldn't just bounce back up again";
}

TEST(TransmissionLogicTest, TorqueConverterRidesTheFluidUntilLockupThenLocksSolid) {
    Transmission::Parameters tp = baseParams();
    tp.HasLockupClutch = true;
    tp.AllowManualShift = true;
    tp.ConverterStallTorque = units::torque(150.0, units::ft_lb);
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();

    // Up into a cruising gear -- a real transmission never applies lockup in
    // first, so first would never lock no matter how little slip there is.
    ASSERT_TRUE(rig.transmission.shiftUp()); // Drive -> Manual
    ASSERT_TRUE(rig.transmission.changeGear(1));
    rig.setDrivetrainSpeed(tp.ConverterStallSpeed);
    for (int i = 0; i < 20; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 1);

    // Big gap between engine crank speed and the input shaft -- well outside
    // lockup range, so the fluid carries everything and nothing rigid is in
    // the path at all.
    rig.engine.m_stubCrankshaft.m_body.v_theta = tp.ConverterStallSpeed + 50.0;
    rig.transmission.update(0.01);
    EXPECT_FALSE(rig.transmission.isConverterLocked());
    EXPECT_TRUE(rig.transmission.getTorqueConverter().isEnabled());
    EXPECT_NEAR(rig.transmission.getClutchTorqueCapacity(), 0.0, 1e-6)
        << "an unlocked converter has no rigid coupling -- the fluid is the whole path";

    // Speeds converge (steady cruise) -- should lock up solid, and hand the
    // load over from the fluid to the clutch.
    rig.engine.m_stubCrankshaft.m_body.v_theta = rig.rotatingMass.v_theta;
    rig.transmission.update(0.01);
    EXPECT_TRUE(rig.transmission.isConverterLocked());
    EXPECT_FALSE(rig.transmission.getTorqueConverter().isEnabled());
    EXPECT_NEAR(rig.transmission.getClutchTorqueCapacity(), tp.MaxClutchTorque, 1e-6);
}

TEST(TransmissionLogicTest, LockupClutchHasHysteresisAndNeverAppliesNearAStandstill) {
    Transmission::Parameters tp = baseParams();
    tp.HasLockupClutch = true;
    tp.AllowManualShift = true;
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_TRUE(rig.transmission.shiftUp()); // Drive -> Manual
    ASSERT_TRUE(rig.transmission.changeGear(1));

    // Locked at cruise.
    rig.setDrivetrainSpeed(units::rpm(2500));
    rig.transmission.update(0.01);
    ASSERT_TRUE(rig.transmission.isConverterLocked());

    // A slip nudge past the engage threshold but still under the release
    // one must not drop it -- one threshold for both directions means it
    // chatters in and out every tick when slip sits right on the line.
    rig.engine.m_stubCrankshaft.m_body.v_theta = units::rpm(2500) + 15.0;
    rig.transmission.update(0.01);
    EXPECT_TRUE(rig.transmission.isConverterLocked()) << "released inside its own hysteresis band";

    // Past the release threshold it does let go.
    rig.engine.m_stubCrankshaft.m_body.v_theta = units::rpm(2500) + 40.0;
    rig.transmission.update(0.01);
    EXPECT_FALSE(rig.transmission.isConverterLocked());

    // Now the case that used to kill the engine outright: stopped car, and
    // an engine sagging down toward the input shaft's zero. Slip is tiny,
    // which naively reads as "steady cruise, lock it up" -- and bridging the
    // converter there bolts the engine straight to a parked car.
    rig.setDrivetrainSpeed(0.0);
    rig.engine.m_stubCrankshaft.m_body.v_theta = units::rpm(50);
    rig.transmission.update(0.01);
    EXPECT_FALSE(rig.transmission.isConverterLocked())
        << "lockup must never apply near a standstill, however little slip there is";
    EXPECT_NEAR(rig.transmission.getClutchTorqueCapacity(), 0.0, 1e-6);
}

TEST(TransmissionLogicTest, OldConverterWithNoLockupAlwaysSlips) {
    Transmission::Parameters tp = baseParams();
    tp.HasLockupClutch = false;
    tp.AllowManualShift = true;
    tp.ConverterStallTorque = units::torque(150.0, units::ft_lb);
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_TRUE(rig.transmission.shiftUp()); // Drive -> Manual
    ASSERT_TRUE(rig.transmission.changeGear(1));

    rig.setDrivetrainSpeed(tp.ConverterStallSpeed);
    for (int i = 0; i < 20; ++i) rig.transmission.update(0.05);

    // Even with matched speeds (what would be a lockup-worthy steady cruise
    // on a modern car), an old converter with no lockup clutch should still
    // be riding the fluid.
    rig.transmission.update(0.01);
    EXPECT_FALSE(rig.transmission.isConverterLocked());
    EXPECT_TRUE(rig.transmission.getTorqueConverter().isEnabled());
    EXPECT_NEAR(rig.transmission.getClutchTorqueCapacity(), 0.0, 1e-6);
}

TEST(TransmissionLogicTest, ConverterAbsorptionFollowsImpellerSpeedSquaredSoIdleDoesntStall) {
    TorqueConverter::Parameters cp{};
    cp.StallTorque = units::torque(150.0, units::ft_lb);
    cp.StallSpeed = units::rpm(4000);
    cp.StallTorqueRatio = 2.0;
    cp.CouplingPoint = 0.87;

    TorqueConverter converter;
    converter.initialize(cp);
    converter.setEngagement(1.0);

    // Idling in Drive at a stop: engine turning over slowly, input shaft at
    // rest. A real converter absorbs almost nothing here -- absorption goes
    // with the square of impeller speed, so at a fifth of stall speed it's
    // a twenty-fifth of the rated torque.
    converter.solve(units::rpm(800), 0.0);
    EXPECT_LT(converter.getImpellerTorque(), 0.1 * cp.StallTorque)
        << "idle absorption should be near-zero, not the full rated stall torque";

    // Revved to stall speed against a stalled turbine is the definition of
    // the rating: exactly ConverterStallTorque.
    converter.solve(cp.StallSpeed, 0.0);
    EXPECT_NEAR(converter.getImpellerTorque(), cp.StallTorque, 1e-6);

    // And the square law itself: half the speed, a quarter the torque.
    converter.solve(0.5 * cp.StallSpeed, 0.0);
    EXPECT_NEAR(converter.getImpellerTorque(), 0.25 * cp.StallTorque, 1e-6);
}

TEST(TransmissionLogicTest, ConverterAbsorptionFallsAwayAsTheTurbineCatchesUp) {
    // The part a plain capacity cap can't express: what the fluid carries
    // depends on how fast the two sides are moving *relative to each other*,
    // not just on pump speed. At the coupling point there's nothing left to
    // transmit, which is what makes handing over to a lockup clutch there a
    // continuous transition rather than a step.
    TorqueConverter::Parameters cp{};
    cp.StallTorque = units::torque(150.0, units::ft_lb);
    cp.StallSpeed = units::rpm(4000);
    cp.StallTorqueRatio = 2.0;
    cp.CouplingPoint = 0.87;

    TorqueConverter converter;
    converter.initialize(cp);
    converter.setEngagement(1.0);

    converter.solve(cp.StallSpeed, 0.0);
    const double stalled = converter.getImpellerTorque();

    converter.solve(cp.StallSpeed, 0.5 * cp.StallSpeed);
    const double halfway = converter.getImpellerTorque();

    converter.solve(cp.StallSpeed, 0.99 * cp.StallSpeed);
    const double nearlySynced = converter.getImpellerTorque();

    EXPECT_LT(halfway, stalled);
    EXPECT_LT(nearlySynced, 0.05 * stalled)
        << "still absorbing full capacity with the turbine nearly synced";
}

TEST(TransmissionLogicTest, ConverterStatorMultipliesTorqueBelowTheCouplingPoint) {
    TorqueConverter::Parameters cp{};
    cp.StallTorque = units::torque(150.0, units::ft_lb);
    cp.StallSpeed = units::rpm(4000);
    cp.StallTorqueRatio = 2.2;
    cp.CouplingPoint = 0.87;

    TorqueConverter converter;
    converter.initialize(cp);
    converter.setEngagement(1.0);

    // Stalled turbine: the stator is redirecting everything, so the turbine
    // sees the full stall torque ratio more than the impeller gives up.
    converter.solve(cp.StallSpeed, 0.0);
    EXPECT_NEAR(converter.getTorqueRatio(), cp.StallTorqueRatio, 1e-6);
    EXPECT_NEAR(
        converter.getTurbineTorque(),
        cp.StallTorqueRatio * converter.getImpellerTorque(),
        1e-6);

    // Multiplication fades out as the turbine comes up...
    converter.solve(cp.StallSpeed, 0.5 * cp.StallSpeed);
    EXPECT_GT(converter.getTorqueRatio(), 1.0);
    EXPECT_LT(converter.getTorqueRatio(), cp.StallTorqueRatio);

    // ...and is gone by the coupling point, where the stator freewheels and
    // it's a plain fluid coupling.
    converter.solve(cp.StallSpeed, cp.CouplingPoint * cp.StallSpeed);
    EXPECT_NEAR(converter.getTorqueRatio(), 1.0, 1e-6);

    // Torque multiplication is not free energy: power out never exceeds
    // power in at any operating point.
    for (double speedRatio = 0.0; speedRatio <= 1.0; speedRatio += 0.05) {
        converter.solve(cp.StallSpeed, speedRatio * cp.StallSpeed);
        const double powerIn = converter.getImpellerTorque() * cp.StallSpeed;
        const double powerOut = converter.getTurbineTorque() * speedRatio * cp.StallSpeed;
        EXPECT_LE(powerOut, powerIn + 1e-6)
            << "converter manufactured energy at a speed ratio of " << speedRatio;
    }
}

TEST(TransmissionLogicTest, ConverterCouplesWeaklyOnOverrun) {
    // The turbine driving the impeller (coasting in gear) runs backwards
    // through blading that isn't shaped for it, with the stator freewheeling
    // -- far weaker than the drive direction, and no multiplication. It's
    // why an automatic with the converter unlocked has so little engine
    // braking, and the symmetric capacity the old model applied was wrong
    // about it in both directions at once.
    TorqueConverter::Parameters cp{};
    cp.StallTorque = units::torque(150.0, units::ft_lb);
    cp.StallSpeed = units::rpm(4000);
    cp.StallTorqueRatio = 2.0;
    cp.CouplingPoint = 0.87;

    TorqueConverter converter;
    converter.initialize(cp);
    converter.setEngagement(1.0);

    converter.solve(cp.StallSpeed, 0.4 * cp.StallSpeed);
    const double driving = converter.getImpellerTorque();
    ASSERT_GT(driving, 0.0);

    // Mirror image: same speeds, swapped over.
    converter.solve(0.4 * cp.StallSpeed, cp.StallSpeed);
    const double overrunning = converter.getImpellerTorque();

    EXPECT_LT(overrunning, 0.0) << "on overrun the coupling should be pushing the engine, not loading it";
    EXPECT_LT(std::abs(overrunning), std::abs(driving))
        << "overrun coupling should be much weaker than the drive direction";
    EXPECT_NEAR(converter.getTorqueRatio(), 1.0, 1e-6) << "no stator multiplication on overrun";
}

TEST(TransmissionLogicTest, ConverterStallSpeedIsIndependentOfALowShiftPoint) {
    // A transmission tuned to short-shift low (a lazy, torque-heavy truck
    // engine, like the 454's th400) shouldn't also make its torque
    // converter reach significant capacity right at idle just because the
    // shift point is low -- the two used to be accidentally coupled
    // through the shift-up threshold (updateTorqueConverter() reused it as
    // the capacity-ramp reference), which meant idling in gear could drag
    // the engine down toward a stall.
    Transmission::Parameters tp = baseParams();
    tp.ShiftUpRpmClosedThrottle = tp.ShiftUpRpmWOT = units::rpm(1700); // matches the 454 truck's th400
    tp.ConverterStallTorque = units::torque(150.0, units::ft_lb);
    tp.ConverterStallSpeed = units::rpm(2600);
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(0.0);
    rig.transmission.shiftUp();

    rig.setDrivetrainSpeed(0.0);
    for (int i = 0; i < 20; ++i) rig.transmission.update(0.05);

    // Idling in gear at a stop with the driver off the gas: whatever the
    // shift schedule says, the converter should be released to near-nothing
    // here rather than loading the engine.
    rig.engine.m_stubCrankshaft.m_body.v_theta = units::rpm(800); // idle
    rig.engine.setThrottle(1.0); // closed
    rig.transmission.update(0.01);

    EXPECT_LT(rig.transmission.getTorqueConverter().getEngagement(), 0.05)
        << "converter should stay released at idle in gear even with a shift point "
           "tuned well below idle*2";
    EXPECT_NEAR(rig.transmission.getClutchTorqueCapacity(), 0.0, 1e-6);
}

TEST(TransmissionLogicTest, AutoShiftRespondsToNegativeRotationConvention) {
    Transmission::Parameters tp = baseParams();
    Rig rig(Transmission::Type::DualClutch, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGear(), 0);

    const double dt = 0.05;

    // This sim's normal running direction is negative v_theta (see
    // StarterMotor's own default) -- the auto-shift schedule has to
    // compare magnitude, not the signed value, or it'd never read a
    // negative RPM as "above" a positive threshold and would just never
    // shift a real, running engine.
    //
    // Nothing in this no-physics Rig moves the stub crank on its own (DCT
    // included -- see DctDoesNotSnapEngineSpeedOnShift), so it stays
    // pinned above the shift point for as long as the loop runs; stop as
    // soon as the first shift lands instead of running a fixed tick count
    // that'd eventually cascade into a second, unrelated one.
    rig.setDrivetrainSpeed(-units::rpm(4500));
    for (int i = 0; i < 20 && rig.transmission.getGear() < 1; ++i) rig.transmission.update(dt);
    EXPECT_EQ(rig.transmission.getGear(), 1);
}

TEST(TransmissionLogicTest, AutoShiftCooldownCoversTheShiftTransitionWindow) {
    Transmission::Parameters tp = baseParams();
    tp.ShiftTimeDrive = 0.35; // single-clutch AMT: slower than a DCT's handoff
    Rig rig(Transmission::Type::AutomatedManual, tp);
    rig.setVehicleSpeed(15.0);
    rig.rotatingMass.v_theta = units::rpm(2500);
    rig.transmission.shiftUp();
    for (int i = 0; i < 50; ++i) rig.transmission.update(0.05); // let the initial engage clear
    ASSERT_EQ(rig.transmission.getGear(), 0);

    // Hold engine RPM pinned above the upshift threshold throughout -- this
    // is exactly the "still slipping post-shift" scenario that let the TCU
    // fire another upshift before the previous one's torque dip (and the
    // engine/wheel resync it interrupts) had actually finished, which is
    // what made RPM run away from road speed instead of settling back
    // down to what the new gear ratio calls for. AMT never syncs the crank
    // to the input shaft on its own (see shiftToGear()), so setting it once
    // here is enough to hold it pinned across every remaining tick below.
    rig.engine.m_stubCrankshaft.m_body.v_theta = units::rpm(7500);
    rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 1) << "first upshift should fire immediately";

    // AMT's shift transition is 0.35s; within that window (plus its
    // resync margin) no second upshift should fire even with RPM still
    // (synthetically) pinned above threshold.
    for (int i = 0; i < 8; ++i) rig.transmission.update(0.05); // 0.4s elapsed
    EXPECT_EQ(rig.transmission.getGear(), 1)
        << "shouldn't re-shift before the previous shift's transition has cleared";

    for (int i = 0; i < 4; ++i) rig.transmission.update(0.05); // past the 0.5s cooldown
    EXPECT_EQ(rig.transmission.getGear(), 2)
        << "should be free to shift again once the transition margin has elapsed";
}

TEST(TransmissionLogicTest, DownshiftProtectionBlocksOverRevButAllowsSafeDownshift) {
    Transmission::Parameters tp = baseParams();
    tp.HasSportMode = false;
    tp.AllowManualShift = true;
    Rig rig(Transmission::Type::DualClutch, tp);
    rig.engine.setRedline(units::rpm(7000));

    // Highway speed in top gear -- paddling straight down to 1st
    // (gearRatios[0] / gearRatios[3] == 3.5x) would send the engine to
    // roughly 15,600 RPM, way past the 7000 RPM redline.
    rig.setVehicleSpeed(40.0);
    ASSERT_TRUE(rig.transmission.shiftUp()); // Neutral -> Drive, gear 0
    ASSERT_TRUE(rig.transmission.shiftUp()); // Drive -> Manual (no Sport equipped)
    ASSERT_TRUE(rig.transmission.changeGear(3));

    EXPECT_FALSE(rig.transmission.changeGear(0))
        << "a paddle downshift that would over-rev the engine should be refused, "
           "same as a real DCT/AMT/manumatic mode's protection";
    EXPECT_EQ(rig.transmission.getGear(), 3) << "rejected downshift shouldn't change gear";
    EXPECT_TRUE(rig.transmission.wasBlockedByDownshiftProtection())
        << "caller (UI status message) needs to be able to tell this apart from other rejection reasons";

    // One step down (gearRatios[2] / gearRatios[3] == 1.3x) lands around
    // 5800 RPM at this speed -- comfortably under redline, so it should
    // still be allowed through.
    EXPECT_TRUE(rig.transmission.changeGear(2));
    EXPECT_EQ(rig.transmission.getGear(), 2);
    EXPECT_FALSE(rig.transmission.wasBlockedByDownshiftProtection())
        << "flag shouldn't stick around after a shift that actually succeeded";
}

TEST(TransmissionLogicTest, DctDoesNotSnapEngineSpeedOnShift) {
    // A DCT's incoming clutch pack being pre-spun to the new gear's speed
    // only means rotatingMass (applyGearRatio(), above in shiftToGear())
    // starts the transition already there -- it's not license to teleport
    // the crank to match it in a single tick. That used to happen here and
    // reads as a hard, physically-impossible pitch pop on every shift, real
    // DCT or not: closing the gap between the crank and the newly-engaged
    // input shaft is the capacity-limited clutch constraint's job, same as
    // any other automated type, just over a much shorter/shallower window
    // (see DctShiftBarelyDipsTorque for the capacity curve that actually
    // governs it).
    Transmission::Parameters tp = baseParams();

    Rig dct(Transmission::Type::DualClutch, tp);
    dct.setVehicleSpeed(15.0);
    dct.transmission.shiftUp();
    dct.setDrivetrainSpeed(units::rpm(2500));
    for (int i = 0; i < 50; ++i) dct.transmission.update(0.05);
    ASSERT_EQ(dct.transmission.getGear(), 0);

    dct.engine.m_stubCrankshaft.m_body.v_theta = units::rpm(4500);
    dct.rotatingMass.v_theta = units::rpm(1000);
    dct.transmission.update(0.05);
    ASSERT_EQ(dct.transmission.getGear(), 1);
    EXPECT_NEAR(dct.engine.m_stubCrankshaft.m_body.v_theta, units::rpm(4500), 1e-6)
        << "the crank shouldn't be forced to the input shaft's speed the instant the shift "
           "fires -- closing that gap is the clutch constraint's job, not shiftToGear()'s";
}

TEST(TransmissionLogicTest, AutomatedShiftCutsIgnitionForTheWholeTransitionButNotOutsideOne) {
    Transmission::Parameters tp = baseParams();
    tp.ShiftTimeDrive = 0.2;
    tp.ShiftTorqueCut = 0.5;
    Rig rig(Transmission::Type::DualClutch, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();

    rig.rotatingMass.v_theta = units::rpm(2500);
    for (int i = 0; i < 50; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);
    EXPECT_FALSE(rig.engine.getIgnitionModule()->getShiftCut()) << "no cut outside a shift";

    rig.setDrivetrainSpeed(units::rpm(4500));
    rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 1);

    // updateClutchTorqueLimits() runs before updateAutoShift() within a
    // single update() call, so the shift-cut flag it sets doesn't reflect
    // a shift triggered by that same call until the next tick -- same
    // ordering the clutch capacity dip is already subject to (see
    // DctShiftBarelyDipsTorque).
    rig.transmission.update(0.1); // partway through the transition
    EXPECT_TRUE(rig.engine.getIgnitionModule()->getShiftCut())
        << "should be cut for the whole transition -- a retard alone isn't a strong enough "
           "effect to reliably unload the drivetrain for the handoff";

    // Nothing in this no-physics Rig brings the stub crank back down on its
    // own (DCT included -- see DctDoesNotSnapEngineSpeedOnShift), so left
    // pinned at 4500rpm it would eventually clear the shift cooldown and
    // cascade into a second, unrelated shift before this loop finishes.
    // Settle it back to a normal in-gear cruise speed first, same as the
    // (now purely physics-driven, not snapped) clutch constraint would
    // actually do by this point in a real run.
    rig.setDrivetrainSpeed(units::rpm(2500));
    for (int i = 0; i < 10; ++i) rig.transmission.update(0.05); // let the transition clear
    EXPECT_FALSE(rig.engine.getIgnitionModule()->getShiftCut())
        << "cut should lift once the transition is over";
}

TEST(TransmissionLogicTest, AutomaticTorqueConverterNeverCutsIgnitionDuringAShiftUnlikeClutchPackTypes) {
    Transmission::Parameters tp = baseParams();
    tp.ShiftTimeDrive = 0.2;
    tp.ShiftTorqueCut = 0.5;
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();

    rig.setDrivetrainSpeed(units::rpm(2500));
    for (int i = 0; i < 50; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);

    rig.setDrivetrainSpeed(units::rpm(4500));
    rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 1);

    rig.transmission.update(0.1); // partway through the transition
    EXPECT_FALSE(rig.engine.getIgnitionModule()->getShiftCut())
        << "a torque-converter automatic has no clutch pack for the engine's own ignition to "
           "interfere with -- cutting spark here would kill the engine's torque right as the "
           "converter goes slack, backwards from the real flare-then-catch shift feel";

    // The dip should still be real, just carried entirely by the fluid
    // coupling's own engagement instead of an ignition cut.
    EXPECT_LT(rig.transmission.getTorqueConverter().getEngagement(), 1.0)
        << "the shift dip should still reduce the converter's own engagement";
}

TEST(TransmissionLogicTest, DctShiftBarelyDipsTorque) {
    Transmission::Parameters tp = baseParams();
    tp.ShiftTimeDrive = 0.08; // real DCTs: ~100-200ms, overlapping clutches
    tp.ShiftTorqueCut = 0.15;
    Rig rig(Transmission::Type::DualClutch, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();

    // Settle at a steady, in-band RPM so no auto-shift sneaks in before the
    // transition this test actually cares about. Engine and input shaft move
    // together -- the launch clutch is fully clamped once it isn't slipping,
    // which is the baseline a shift dips away from.
    rig.setDrivetrainSpeed(units::rpm(2500));
    for (int i = 0; i < 50; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);
    ASSERT_NEAR(rig.transmission.getClutchTorqueCapacity(), tp.MaxClutchTorque, 1e-6);

    rig.setDrivetrainSpeed(units::rpm(4500));
    rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 1);

    rig.transmission.update(0.03); // partway through the transition
    EXPECT_GT(rig.transmission.getClutchTorqueCapacity(), 0.7 * tp.MaxClutchTorque)
        << "a DCT's pre-selected-gear handoff should barely dip, not bog down like a slushbox";

    // With capacity left high through the handoff (just asserted above),
    // the real clutch constraint has plenty of torque to pull the crank
    // in to match rotatingMass well before the transition ends -- this
    // no-physics Rig doesn't step that solver, so stand in for it here.
    // Settle back to the same steady, in-band RPM used above rather than
    // reusing the 4500rpm that fired the shift: still pinned there and
    // with nothing to bring it down on its own, it'd clear the next
    // cooldown and cascade into another shift, restranding the crank at
    // yet another stale speed instead of ever converging. Without settling
    // it at all, the raw slip-based mismatch penalty (see
    // updateLaunchClutch()) -- which stops being suppressed the instant
    // m_shiftTransitionTimer clears -- would catch the crank still stale
    // and keep capacity clamped down even after the transition is long over.
    rig.setDrivetrainSpeed(units::rpm(2500));
    for (int i = 0; i < 20; ++i) rig.transmission.update(0.05);
    EXPECT_NEAR(rig.transmission.getClutchTorqueCapacity(), tp.MaxClutchTorque, 1e-6)
        << "capacity should fully recover once the transition elapses";
}

TEST(TransmissionLogicTest, LaunchClutchLocksUpOnceRollingEvenIfSlipStaysWide) {
    // Bug: updateLaunchClutch() had no equivalent of launchEngagement()'s
    // "rolling" hand-off (used by updateTorqueConverter() -- "once the
    // input shaft is turning at a decent fraction of engine speed the car
    // is genuinely moving, hand it full engagement"). Capacity here was
    // purely a function of slip (which only ever SHRINKS capacity as it
    // grows) and engine RPM relative to redline (flare/overrun), neither of
    // which reacts to road speed climbing. An engine parked at its
    // flare-capped equilibrium against a low, fixed creep capacity just sat
    // there forever -- RPM pinned, speed climbing, clutch never actually
    // locking up, same symptom as a torque converter that never leaves the
    // fluid.
    Transmission::Parameters tp = baseParams();
    tp.LaunchClutchCreepCapacity = 0.35;
    Rig rig(Transmission::Type::DualClutch, tp);
    rig.engine.setRedline(units::rpm(7000));
    rig.setVehicleSpeed(0.1);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGear(), 0);

    // Engine parked right at the flare target (45% redline) at WOT -- the
    // creep/flare formula alone caps out here (== LaunchClutchCreepCapacity)
    // and never climbs further on its own.
    rig.engine.m_stubCrankshaft.m_body.v_theta = units::rpm(3150); // 45% of 7000
    rig.engine.setThrottle(0.0); // WOT

    // Input shaft well past "genuinely rolling" (kLaunchLockSpeedRatio is
    // 25% of engine speed) but still a wide, real mismatch away from engine
    // speed -- exactly the "car got away from a slipping clutch" case this
    // was stuck in.
    rig.rotatingMass.v_theta = units::rpm(1200);

    rig.transmission.update(0.01);
    EXPECT_NEAR(rig.transmission.getClutchTorqueCapacity(), tp.MaxClutchTorque, 1e-6)
        << "once the input shaft is genuinely rolling relative to the engine, the launch "
           "device should hand off to full capacity instead of staying parked at its low "
           "flare-capped creep value forever";
}

TEST(TransmissionLogicTest, AutomaticTorqueConverterShiftDipReleasesQuicklyButRecatchesSlowly) {
    // Unlike a clutch pack's symmetric dip (see DctShiftBarelyDipsTorque),
    // a fluid coupling's outgoing clutch pack/band vents pressure and goes
    // slack fast, but the incoming one has to build apply pressure
    // gradually -- so engagement should already be at/near its deepest
    // dip a quarter of the way through the transition, and clearly
    // recovering well before the transition ends.
    Transmission::Parameters tp = baseParams();
    tp.ShiftTimeDrive = 1.0; // round number, easy to reason about fractions of
    tp.ShiftTorqueCut = 0.6;
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();

    rig.setDrivetrainSpeed(units::rpm(2500));
    for (int i = 0; i < 50; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);
    ASSERT_NEAR(rig.transmission.getTorqueConverter().getEngagement(), 1.0, 1e-6);

    rig.setDrivetrainSpeed(units::rpm(4500));
    rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 1);

    rig.transmission.update(0.25); // 0.25s into the 1.0s transition
    const double engagementAtQuarter = rig.transmission.getTorqueConverter().getEngagement();
    EXPECT_LT(engagementAtQuarter, 0.45)
        << "release should already be at/near full dip a quarter of the way through, got "
        << engagementAtQuarter;

    rig.transmission.update(0.25); // 0.5s into the transition
    const double engagementAtHalf = rig.transmission.getTorqueConverter().getEngagement();
    EXPECT_GT(engagementAtHalf, engagementAtQuarter)
        << "should already be re-catching (recovering) by halfway through, unlike a symmetric "
           "dip which would still be at its lowest here -- got " << engagementAtHalf
        << " vs. " << engagementAtQuarter << " at the quarter mark";
}

TEST(TransmissionLogicTest, AutomaticTorqueConverterSweepsTheRatioGraduallyInsteadOfSnapping) {
    // An instant ratio snap at the shift instant changes m_rotatingMass's
    // implied speed in a single tick while the engine (only connected
    // through the fluid coupling, not a rigid constraint) hasn't moved at
    // all yet -- a sudden, artificial slip spike that TorqueConverter's
    // speed-squared absorption law turns into a violent multi-thousand
    // lb-ft transient. The ratio should instead ease in gradually over the
    // transition, same window as the dip above.
    Transmission::Parameters tp = baseParams();
    tp.ShiftTimeDrive = 1.0;
    Rig rig(Transmission::Type::AutomaticTorqueConverter, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();

    rig.setDrivetrainSpeed(units::rpm(2500));
    for (int i = 0; i < 50; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);
    ASSERT_NEAR(impliedGearRatio(rig.rotatingMass, rig.vehicle), rig.gearRatios[0], 1e-6);

    rig.setDrivetrainSpeed(units::rpm(4500));
    rig.transmission.update(0.05); // the shift fires this tick
    ASSERT_EQ(rig.transmission.getGear(), 1);

    // Right after the shift fires, the ratio shouldn't have snapped to the
    // new gear yet -- shiftToGear() only records where to sweep from/to;
    // the actual sweep runs starting next tick (see update()).
    EXPECT_NEAR(impliedGearRatio(rig.rotatingMass, rig.vehicle), rig.gearRatios[0], 1e-6)
        << "should still be at the OLD ratio immediately after the shift fires, not snapped "
           "straight to the new one";

    for (int i = 0; i < 20; ++i) rig.transmission.update(0.05); // let the 1.0s transition finish
    EXPECT_NEAR(impliedGearRatio(rig.rotatingMass, rig.vehicle), rig.gearRatios[1], 0.05)
        << "should have (almost exactly) swept to the new ratio by the end of the transition "
           "-- some small residual is expected from the coarse 0.05s test dt";
}

TEST(TransmissionLogicTest, AmtShiftNearlyCutsTorqueMidTransition) {
    Transmission::Parameters tp = baseParams();
    tp.ShiftTimeDrive = 0.35; // single clutch: slower and near-total cut, unlike a DCT
    tp.ShiftTorqueCut = 0.9;
    Rig rig(Transmission::Type::AutomatedManual, tp);
    rig.setVehicleSpeed(15.0);
    rig.transmission.shiftUp();

    rig.rotatingMass.v_theta = units::rpm(2500);
    for (int i = 0; i < 50; ++i) rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 0);

    rig.setDrivetrainSpeed(units::rpm(4500));
    rig.transmission.update(0.05);
    ASSERT_EQ(rig.transmission.getGear(), 1);

    rig.transmission.update(0.15); // near the midpoint of AMT's longer transition
    EXPECT_LT(rig.transmission.getClutchTorqueCapacity(), 0.3 * tp.MaxClutchTorque)
        << "single-clutch AMT should nearly cut torque mid-shift, unlike a DCT";
}

TEST(TransmissionLogicTest, CvtHoldsTargetRpmContinuouslyThenModesLockSteps) {
    Transmission::Parameters tp = baseParams();
    tp.HasSportMode = true;
    tp.AllowManualShift = true;
    Rig rig(Transmission::Type::CVT, tp);
    rig.setVehicleSpeed(20.0);
    rig.transmission.shiftUp();
    ASSERT_EQ(rig.transmission.getGearLabel(), "D");

    const double dt = 0.1;
    const double wheelOmega = rig.vehicle.getSpeed() / rig.vehicle.getTireRadius();
    const double expectedRatio = std::clamp(
        units::rpm(2500) / (wheelOmega * rig.vehicle.getDiffRatio()), 1.0, 3.5);

    for (int i = 0; i < 20; ++i) rig.transmission.update(dt);
    EXPECT_NEAR(impliedGearRatio(rig.rotatingMass, rig.vehicle), expectedRatio, 1e-3);

    // Sport: snaps onto the nearest fixed ratio, then auto-shifts among the
    // discrete steps by RPM (same schedule logic as any other automated
    // type) rather than varying continuously -- the snap itself may not
    // land in-band, so let it settle before checking it holds.
    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getDriveMode(), Transmission::DriveMode::Sport);

    for (int i = 0; i < 40; ++i) rig.transmission.update(dt);
    const int settledGear = rig.transmission.getGear();
    for (int i = 0; i < 20; ++i) rig.transmission.update(dt);
    EXPECT_EQ(rig.transmission.getGear(), settledGear)
        << "should have settled into a steady gear and stopped shifting";

    // Manual: driver takes over, no auto-shift logic runs at all.
    ASSERT_TRUE(rig.transmission.shiftUp());
    EXPECT_EQ(rig.transmission.getDriveMode(), Transmission::DriveMode::Manual);
    EXPECT_EQ(rig.transmission.getGear(), settledGear) << "entering Manual shouldn't itself change gear";

    ASSERT_TRUE(rig.transmission.changeGear(0));
    EXPECT_EQ(rig.transmission.getGearLabel(), "M1");

    for (int i = 0; i < 20; ++i) rig.transmission.update(dt);
    EXPECT_EQ(rig.transmission.getGear(), 0) << "Manual should hold its step, not auto-shift";
}
