#include <gtest/gtest.h>

#include "../include/ignition_module.h"
#include "../include/crankshaft.h"
#include "../include/function.h"
#include "../include/units.h"

#include <cmath>

namespace {
    constexpr int kCylinderCount = 8;
    constexpr double kFourPi = 4.0 * 3.14159265358979323846;

    // A bare Crankshaft/Function pair is enough to drive IgnitionModule in
    // isolation -- it only reads getCycleAngle() (derived from
    // m_body.theta) and samples the timing curve, neither of which needs a
    // real constraint solver or combustion physics in the loop. Mirrors the
    // StubEngine pattern in transmission_logic_test.cpp.
    struct Rig {
        Crankshaft crankshaft;
        Function timingCurve;
        IgnitionModule ignition;

        Rig(double revLimit, double limiterDuration, double limiterCutFraction,
            double limiterMaxRetard = 0.0) {
            timingCurve.initialize(0, 1.0);
            timingCurve.addSample(0.0, 0.0); // flat 0-degree advance -- keeps plug angle == fire angle

            IgnitionModule::Parameters params;
            params.cylinderCount = kCylinderCount;
            params.crankshaft = &crankshaft;
            params.timingCurve = &timingCurve;
            params.revLimit = revLimit;
            params.limiterDuration = limiterDuration;
            params.limiterCutFraction = limiterCutFraction;
            params.limiterMaxRetard = limiterMaxRetard;
            ignition.initialize(params);

            // 8 cylinders spread evenly across one 4*pi combustion cycle --
            // same interval a V8's real firing spacing works out to,
            // though the specific firing order doesn't matter for these
            // tests (only the event count does).
            for (int i = 0; i < kCylinderCount; ++i) {
                ignition.setFiringOrder(i, i * (kFourPi / kCylinderCount));
            }

            ignition.m_enabled = true;
            ignition.reset();
        }

        // Spins the crank at a constant `omega` (rad/s, sim convention is
        // negative-running) for `duration` seconds and returns how many
        // ignition events fired across all cylinders combined.
        int runAndCountFirings(double omega, double duration) {
            const double dt = 0.0005; // fine enough to resolve each event distinctly

            // The limiter can only arm m_revLimitTimer *after* seeing this
            // tick's speed (see IgnitionModule::update()), so the very
            // first tick a run starts already over the limit reads
            // "not limiting yet" -- a one-event grace inherent to the
            // detect-then-arm design, not specific to the cut-fraction
            // feature. One untallied warm-up tick arms it before counting
            // starts, so the counted totals below reflect steady-state
            // behavior rather than that startup transient.
            crankshaft.m_body.v_theta = omega;
            crankshaft.m_body.theta += omega * dt;
            ignition.update(dt);
            ignition.resetIgnitionEvents();

            int fired = 0;
            for (double t = 0.0; t < duration; t += dt) {
                crankshaft.m_body.v_theta = omega;
                crankshaft.m_body.theta += omega * dt;

                ignition.update(dt);
                for (int i = 0; i < kCylinderCount; ++i) {
                    if (ignition.getIgnitionEvent(i)) ++fired;
                }
                ignition.resetIgnitionEvents();
            }
            return fired;
        }
    };
}

TEST(IgnitionModuleTest, CutFractionOneIsATotalCutSameAsBeforeTheParameterExisted) {
    Rig rig(units::rpm(1000.0), 0.1, 1.0);

    // Comfortably over the limit for the whole run.
    const int fired = rig.runAndCountFirings(-units::rpm(3000.0), 2.0);
    EXPECT_EQ(fired, 0) << "fraction 1.0 should cut every single ignition event, same as the "
                            "old whole-block skip it replaces";
}

TEST(IgnitionModuleTest, CutFractionZeroNeverCutsEvenWhileOverLimit) {
    Rig rig(units::rpm(1000.0), 0.1, 0.0);

    const int firedOverLimit = rig.runAndCountFirings(-units::rpm(3000.0), 2.0);

    Rig baseline(units::rpm(100000.0), 0.1, 1.0); // limit unreachable -- never limiting
    const int firedNeverLimiting = baseline.runAndCountFirings(-units::rpm(3000.0), 2.0);

    ASSERT_GT(firedNeverLimiting, 0) << "test is meaningless if nothing fired at all";
    EXPECT_EQ(firedOverLimit, firedNeverLimiting)
        << "fraction 0.0 should be indistinguishable from never hitting the limiter at all";
}

TEST(IgnitionModuleTest, PartialCutFractionCutsRoughlyThatShareOfEvents) {
    const int totalIfUncut = [] {
        Rig baseline(units::rpm(100000.0), 0.1, 1.0);
        return baseline.runAndCountFirings(-units::rpm(3000.0), 2.0);
    }();

    // Held just barely OVER the limit (the over-limit check is a strict
    // `>`, so omega == revLimit exactly would never even arm the limiter)
    // but negligibly so relative to the 15% overrun band, to isolate the
    // base cut fraction from the overrun ramp tested separately below --
    // running well past the limit would ramp effectiveCutFraction up
    // toward 1.0 regardless of the 0.3 requested here.
    Rig rolling(units::rpm(2999.0), 0.1, 0.3);
    const int fired = rolling.runAndCountFirings(-units::rpm(3000.0), 2.0);

    ASSERT_GT(totalIfUncut, 100) << "need a decent sample size for the fraction check below";

    // A rolling 0.3 cut fraction cuts 30% of events, i.e. should let
    // roughly 70% still fire -- loose tolerance since the Bresenham-style
    // accumulator only converges to the exact fraction over many events,
    // not exactly.
    const double survivalRate = (double)fired / (double)totalIfUncut;
    EXPECT_NEAR(survivalRate, 0.7, 0.05)
        << "expected ~70% of events to still fire through a 0.3 cut fraction right at the "
           "limit, got " << fired << "/" << totalIfUncut;
}

TEST(IgnitionModuleTest, SoftCutRampsToAFullCutFarEnoughPastTheLimitInsteadOfRunningAway) {
    // This is the actual bug a flat cut fraction has: even a very light
    // 0.05 base cut just multiplies torque down slightly -- if the engine
    // has torque to spare, nothing stops RPM from climbing well past
    // redline (reported as the LS3/4L60E file revving to ~10000rpm with a
    // flat 0.3 fraction that wasn't enough to hold it). Held here 50% over
    // the limit -- well past kSoftLimiterOverrunBand's 15% band -- the
    // ramp should already be pinned at a full cut regardless of how low
    // the requested base fraction was.
    const int totalIfUncut = [] {
        Rig baseline(units::rpm(100000.0), 0.1, 1.0);
        return baseline.runAndCountFirings(-units::rpm(3000.0), 2.0);
    }();

    Rig rig(units::rpm(2000.0), 0.1, 0.05); // omega will be 50% over this limit
    const int fired = rig.runAndCountFirings(-units::rpm(3000.0), 2.0);

    ASSERT_GT(totalIfUncut, 100) << "need a decent sample size for the fraction check below";
    EXPECT_LT((double)fired / (double)totalIfUncut, 0.05)
        << "a low base cut fraction should still ramp to a full cut once far enough over the "
           "limit, got " << fired << "/" << totalIfUncut << " events surviving";
}

TEST(IgnitionModuleTest, ZeroCutFractionStaysExemptFromTheOverrunRampEvenFarPastTheLimit) {
    // The "disable the limiter entirely" escape hatch (fraction 0.0) has
    // to survive the overrun ramp too, or there'd be no way to actually
    // turn the limiter off.
    const int totalIfUncut = [] {
        Rig baseline(units::rpm(100000.0), 0.1, 1.0);
        return baseline.runAndCountFirings(-units::rpm(3000.0), 2.0);
    }();

    Rig rig(units::rpm(2000.0), 0.1, 0.0); // omega will be 50% over this limit
    const int fired = rig.runAndCountFirings(-units::rpm(3000.0), 2.0);

    EXPECT_EQ(fired, totalIfUncut)
        << "fraction 0.0 should still mean 'never cuts,' even far past the limit";
}

TEST(IgnitionModuleTest, BelowRevLimitIgnoresCutFractionEntirely) {
    // fraction 1.0 (a total cut, if it were limiting) should have zero
    // effect when the engine never actually crosses the limit.
    Rig rig(units::rpm(100000.0), 0.1, 1.0);
    const int fired = rig.runAndCountFirings(-units::rpm(3000.0), 2.0);
    EXPECT_GT(fired, 0) << "should fire completely normally when never over the limit";
}

TEST(IgnitionModuleTest, TimingRetardIsZeroBelowRevLimit) {
    Rig rig(units::rpm(3000.0), 0.1, 0.0, 40.0 * units::deg);

    rig.crankshaft.m_body.v_theta = -units::rpm(2000.0); // comfortably under the limit
    EXPECT_NEAR(rig.ignition.getTimingAdvance(), 0.0, 1E-9)
        << "no retard should apply at all until rpm actually crosses revLimit";
}

TEST(IgnitionModuleTest, TimingRetardRampsSmoothlyAcrossTheOverrunBandThenHoldsAtMax) {
    // Same 5% overrun band the cut-fraction ramp uses (kSoftLimiterOverrunBand
    // in ignition_module.cpp) -- 3000rpm limit -> fully engaged by 3150rpm.
    Rig rig(units::rpm(3000.0), 0.1, 0.0, 40.0 * units::deg);

    rig.crankshaft.m_body.v_theta = -units::rpm(3075.0); // halfway through the band
    const double halfway = -rig.ignition.getTimingAdvance();
    EXPECT_NEAR(halfway, 20.0 * units::deg, 1.0 * units::deg)
        << "halfway through the overrun band should be roughly half the max retard";

    rig.crankshaft.m_body.v_theta = -units::rpm(3150.0); // exactly the top of the band
    const double atTop = -rig.ignition.getTimingAdvance();
    EXPECT_NEAR(atTop, 40.0 * units::deg, 1E-6)
        << "should be fully engaged (max retard) right at the top of the overrun band";

    rig.crankshaft.m_body.v_theta = -units::rpm(6000.0); // way past the band
    const double wayOver = -rig.ignition.getTimingAdvance();
    EXPECT_NEAR(wayOver, 40.0 * units::deg, 1E-6)
        << "retard shouldn't keep climbing past the max once fully engaged";
}

TEST(IgnitionModuleTest, TimingRetardNeverSkipsAnIgnitionEventUnlikeAnyCutFraction) {
    // The whole point of retard over cut: every cylinder still fires every
    // single cycle, no matter how hard the limiter is engaged -- it's a
    // smooth, continuous reduction, not a percussive missing-pop texture.
    const int totalIfUncut = [] {
        Rig baseline(units::rpm(100000.0), 0.1, 1.0);
        return baseline.runAndCountFirings(-units::rpm(3000.0), 2.0);
    }();

    Rig rig(units::rpm(1000.0), 0.1, 0.0, 45.0 * units::deg); // cut fraction off, retard only
    const int fired = rig.runAndCountFirings(-units::rpm(3000.0), 2.0); // way over the limit

    EXPECT_EQ(fired, totalIfUncut)
        << "a pure retard-based soft limiter (cut fraction 0) should never drop an ignition "
           "event, regardless of how far over the limit rpm is";
}

TEST(IgnitionModuleTest, ShiftCutStillSuppressesEverythingRegardlessOfCutFraction) {
    // Even a soft/rolling limiter tune shouldn't leak spark through during
    // a transmission's shift-cut window -- that's a different, unrelated
    // suppression path (see Transmission::updateClutchTorqueLimits()).
    Rig rig(units::rpm(100000.0), 0.1, 0.0); // limiter itself never engages
    rig.ignition.setShiftCut(true);

    const int fired = rig.runAndCountFirings(-units::rpm(3000.0), 0.5);
    EXPECT_EQ(fired, 0) << "shift cut should suppress ignition regardless of limiter cut fraction";
}
