#ifndef ATG_ENGINE_SIM_IGNITION_MODULE_H
#define ATG_ENGINE_SIM_IGNITION_MODULE_H

#include "part.h"

#include "crankshaft.h"
#include "function.h"
#include "units.h"

class IgnitionModule : public Part {
    public:
        struct Parameters {
            int cylinderCount;
            Crankshaft *crankshaft;
            Function *timingCurve;
            double revLimit = units::rpm(6000.0);
            double limiterDuration = 0.5 * units::sec;

            // Fraction of ignition events suppressed while the limiter is
            // active, 0 (no cut at all) to 1 (every cylinder, every time --
            // the default, and the only behavior this ever had before this
            // parameter existed). limiterDuration alone only controls how
            // often the cut re-triggers; at fraction 1.0 that's still a
            // total kill every single cycle, just modulated in frequency --
            // that's what actually produces the sharp, popping "brap"
            // bounce-off-the-limiter bark real hard-cut exotic/race ECUs
            // have (unburned fuel dumping into a hot exhaust on every cut).
            // A short duration does NOT make that soft; it just makes the
            // bark stutter faster. A genuinely soft/rolling-cut calibration
            // -- what most ordinary passenger cars and trucks actually run,
            // since nobody wants their commuter car popping like a race car
            // -- only drops a rotating fraction of cylinders each cycle, so
            // the engine keeps making most of its torque continuously and
            // just gently stops accelerating instead of slamming to zero
            // and back.
            double limiterCutFraction = 1.0;

            // Max spark retard (radians) applied once rpm is a full
            // overrun band past revLimit, ramping in smoothly from 0
            // right at revLimit -- see getTimingAdvance(). Unlike
            // limiterCutFraction at ANY setting, this never skips a
            // combustion event -- every cylinder still fires every cycle,
            // just later and weaker. That's what actually makes it sound
            // soft: no missing-pop texture at all, just the engine going
            // dull and running out of steam, versus even a finely-rolled
            // partial spark cut still being audibly a rapid string of
            // dropped cylinders. Default 0, so nothing changes unless an
            // engine file asks for it.
            double limiterMaxRetard = 0.0;
        };

        struct SparkPlug {
            double angle = 0;
            bool ignitionEvent = false;
            bool enabled = false;
        };

    public:
        IgnitionModule();
        virtual ~IgnitionModule();

        virtual void destroy();

        void initialize(const Parameters &params);
        void setFiringOrder(int cylinderIndex, double angle);
        void reset();
        void update(double dt);

        bool getIgnitionEvent(int index) const;
        void resetIgnitionEvents();

        double getTimingAdvance();

        // Live additive retard (radians subtracted from the timing curve's
        // advance) -- anti-lag's hook into spark timing. 0 by default, so
        // engines that never touch this behave exactly as before.
        void setTimingRetard(double retard) { m_timingRetard = retard; }
        double getTimingRetard() const { return m_timingRetard; }

        // Transmission's hook for actually killing power mid-shift (see
        // Transmission::updateClutchTorqueLimits()) -- same spark
        // suppression the rev limiter uses, just driven by "a shift is in
        // progress" instead of "over redline". A timing retard alone isn't
        // a strong enough effect to reliably unload the drivetrain for the
        // synchro/dog/clutch-pack to move freely, so this is a real cut,
        // not a reduction. False by default, so nothing changes unless
        // something asks for it.
        void setShiftCut(bool cut) { m_shiftCut = cut; }
        bool getShiftCut() const { return m_shiftCut; }

        bool m_enabled;

    protected:
        SparkPlug *getPlug(int i);

        Function *m_timingCurve;
        SparkPlug *m_plugs;
        Crankshaft *m_crankshaft;
        int m_cylinderCount;

        double m_lastCrankshaftAngle;
        double m_revLimit;
        double m_revLimitTimer;
        double m_limiterDuration;
        double m_limiterCutFraction;
        double m_limiterCutAccumulator;
        double m_limiterMaxRetard;
        double m_timingRetard = 0.0;
        bool m_shiftCut = false;
};

#endif /* ATG_ENGINE_SIM_IGNITION_MODULE_H */
