#ifndef ATG_ENGINE_SIM_TRANSMISSION_H
#define ATG_ENGINE_SIM_TRANSMISSION_H

#include "vehicle.h"
#include "engine.h"
#include "torque_converter.h"
#include "scs.h"
#include "units.h"

#include <string>

class Transmission {
    public:
        enum class Type {
            Manual,
            AutomaticTorqueConverter,
            DualClutch,
            AutomatedManual,
            CVT
        };

        // Neutral isn't part of this -- it's m_gear == -1, same as Manual.
        // Drive/Sport/Manual only apply once something's engaged, entered
        // via shiftUp()/shiftDown()'s Neutral -> Drive -> Sport -> Manual
        // ladder (mirrors a real shifter's gate: forward through P-R-N-D,
        // then over to the manual gate and +/- from there).
        enum class DriveMode {
            Drive,
            Sport,
            Manual
        };

        struct Parameters {
            int GearCount;
            const double *GearRatios;
            double MaxClutchTorque;

            // Dog-engagement straight-cut gears instead of helical --
            // defaults false so existing scripts/assets don't need to
            // specify it. Purely cosmetic/audio right now (see
            // Transmission::isStraightCut() usage in the gear whine synth).
            bool StraightCut = false;

            Type TransmissionType = Type::Manual;

            // Ladder capability, only meaningful when TransmissionType !=
            // Manual. HasSportMode: whether shiftUp() can reach a
            // driver-selectable Sport mode at all (plenty of cars just
            // have D). AllowManualShift: whether it can go on from there
            // into a driver-controlled Manual mode (paddles/manumatic).
            // Both false is the "dumber older automatic" case: just Drive,
            // computer decides everything, always.
            bool HasSportMode = false;
            bool AllowManualShift = false;

            // TCU shift schedule. A real transmission -- hydraulic/vacuum
            // valve body or electronic, doesn't matter -- never decides a
            // shift off engine RPM: it reads throttle position (a cable or
            // vacuum modulator for an old hydraulic box, a TPS voltage for
            // an electronic one) and blends continuously between a
            // closed-throttle economy shift point and a wide-open-throttle
            // (WOT) "hold it for power" shift point. That blend is what
            // Transmission::updateAutoShift() does with these; there's no
            // separate on/off "electronic" behavior anymore, because a
            // dumb hydraulic box is throttle-responsive too, just less
            // precisely tuned (see ShiftTimeDrive/ShiftTorqueCut below for
            // where that "dumb vs. smart" character actually shows up).
            // Sport is the same shape, just held longer across the board.
            // Ignored for CVT's Drive mode (see CvtTargetRpm) but still
            // used for CVT's Sport mode, which auto-shifts among the
            // discrete GearRatios same as any other automated type.
            double ShiftUpRpmClosedThrottle = units::rpm(3200);
            double ShiftUpRpmWOT = units::rpm(6200);
            double ShiftDownRpmClosedThrottle = units::rpm(1200);
            double ShiftDownRpmWOT = units::rpm(3400);

            double SportShiftUpRpmClosedThrottle = units::rpm(4500);
            double SportShiftUpRpmWOT = units::rpm(6800);
            double SportShiftDownRpmClosedThrottle = units::rpm(2200);
            double SportShiftDownRpmWOT = units::rpm(4500);

            // What actually gives a shift its character isn't the ratio
            // swap (that's instantaneous either way, see shiftToGear())
            // but how long the torque coupling takes to fully re-catch
            // afterward, and how deep it dips while doing so -- a real
            // DCT overlaps the outgoing/incoming clutch and hands off in
            // ~100-200ms, a single-clutch AMT has to fully open, swap, and
            // reclose so it's slower and the dip is much deeper, and an
            // old-school torque converter auto is slower and slushier
            // still (RPM flares, the band/clutch re-catches). These are
            // per-transmission, not per-Type: a modern 8-speed
            // torque-converter auto (e.g. a ZF 8HP) shifts far crisper
            // than an old 3-speed hydraulic truck box despite both being
            // TransmissionType::AutomaticTorqueConverter. Sport is
            // typically snappier than Drive; cut fraction isn't split by
            // mode since real cars don't usually vary it that way.
            double ShiftTimeDrive = 0.5;
            double ShiftTimeSport = 0.35;
            double ShiftTorqueCut = 0.5;

            // How long a computer-decided (auto-shift) or paddle-selected
            // (changeGear) shift sits "requested" before the ratio itself
            // actually swaps -- the real mechanical lag between a valve
            // body (hydraulic) or TCU solenoid (electronic) deciding to
            // shift and the clutch pack/band actually finishing its apply
            // stroke, distinct from ShiftTimeDrive/Sport's post-swap dip
            // and re-catch. Zero by default, so every existing file shifts
            // exactly as instantly as it always has. An old-school
            // hydraulic slushbox has a much longer, mushier "gathering"
            // pause here than a modern electronically-controlled box.
            double ShiftPrepareTimeDrive = 0.0;
            double ShiftPrepareTimeSport = 0.0;

            // CVT-only: the engine speed the variator tries to hold in its
            // (Drive-mode, continuous-ratio) mode, and how fast it can
            // sweep its ratio, in ratio-units/second -- pulleys/belt
            // aren't instant.
            double CvtTargetRpm = units::rpm(3000);
            double CvtRatioRate = 1.5;

            // AutomaticTorqueConverter-only: the impeller torque the fluid
            // coupling absorbs at ConverterStallSpeed with the turbine held
            // stationary -- the classic converter rating. Absorption scales
            // with the square of impeller speed from there, and falls away
            // again as the turbine catches up (see TorqueConverter::apply()).
            // Keep this below MaxClutchTorque, which is the rigid lockup
            // capacity. HasLockupClutch true rigidly locks engine to input
            // shaft once slip settles down (steady cruise), same as
            // basically every torque-converter car since the 1980s; false is
            // an old-school converter that never locks up and just always
            // slips a little, even at cruise.
            double ConverterStallTorque = units::torque(300.0, units::ft_lb);
            bool HasLockupClutch = false;

            // AutomaticTorqueConverter-only: the classic "stall speed" spec
            // -- the RPM the engine can rev to against a fully stalled
            // output shaft, i.e. where the converter absorbs exactly
            // ConverterStallTorque. Deliberately its own tunable, separate
            // from the shift schedule: an engine that's tuned to short-shift
            // low (a lazy, low-revving truck motor) doesn't necessarily have
            // a low-stall-speed converter to match.
            double ConverterStallSpeed = units::rpm(2500);

            // AutomaticTorqueConverter-only: the stator's contribution.
            // ConverterStallTorqueRatio is how much the turbine's torque is
            // multiplied over the impeller's with the turbine stalled (~2:1
            // on a passenger car, higher on loose truck/racing units), and
            // ConverterCouplingPoint is the speed ratio (turbine/impeller)
            // at which the stator freewheels, multiplication stops, and the
            // unit becomes a plain fluid coupling.
            double ConverterStallTorqueRatio = 2.0;
            double ConverterCouplingPoint = 0.87;

            // DualClutch/AutomatedManual/CVT-only: these have no fluid
            // coupling, so their launch device is a slipping clutch pack (or
            // belt) instead. This is the fraction of MaxClutchTorque that
            // clutch is allowed to carry at wide open throttle while the car
            // is still stationary -- low enough that the engine flares and
            // feeds the car in rather than being rigidly bolted to a
            // stopped mass, which is what a real DCT/AMT launch does and
            // what keeps it from stalling on takeoff. Capacity ramps back
            // to full as the input shaft catches the engine.
            double LaunchClutchCreepCapacity = 0.35;
        };

    public:
        Transmission();
        ~Transmission();

        void initialize(const Parameters &params);
        void update(double dt);
        void addToSystem(
            atg_scs::RigidBodySystem *system,
            atg_scs::RigidBody *rotatingMass,
            Vehicle *vehicle,
            Engine *engine);

        // Directly selects a gear index (or -1 for neutral). For a Manual
        // transmission this is the normal shift path (still gated by the
        // clutch). For an automated one, it only works once already in
        // DriveMode::Manual -- Drive/Sport are computer-controlled and
        // neutral isn't reachable this way; use shiftUp()/shiftDown() to
        // drive the ladder that gets it there.
        bool changeGear(int newGear);

        // "Shift lever" input: for a Manual transmission, an ordinary
        // clutch-gated up/downshift. For an automated one, walks the
        // Neutral -> Drive -> Sport -> Manual ladder (capped by
        // HasSportMode/AllowManualShift) one step at a time, then shifts
        // gears normally once in Manual. Returns false if the request was
        // rejected (out-of-range gear, clutch still engaged, or already at
        // the top/bottom of the ladder).
        bool shiftUp();
        bool shiftDown();

        // True if the most recent shiftUp()/shiftDown()/changeGear() call
        // was refused specifically by downshift protection (see
        // changeGear()) rather than some other reason (clutch still
        // engaged, already at the top/bottom of the ladder, ...) -- lets
        // the caller show a specific "why didn't that work" message
        // instead of a generic one.
        inline bool wasBlockedByDownshiftProtection() const { return m_downshiftProtectionTriggered; }

        inline int getGear() const { return m_gear; }
        inline void setClutchPressure(double pressure) { m_clutchPressure = pressure; }
        inline double getClutchPressure() const { return m_clutchPressure; }
        inline bool isStraightCut() const { return m_straightCut; }
        inline Type getType() const { return m_type; }
        inline bool isManual() const { return m_type == Type::Manual; }
        inline bool hasSportMode() const { return m_hasSportMode; }
        inline bool allowsManualShift() const { return m_allowManualShift; }
        inline DriveMode getDriveMode() const { return m_driveMode; }

        // Current torque capacity of the rigid coupling -- the clutch for a
        // Manual, the launch/lockup clutch otherwise. Zero whenever an
        // AutomaticTorqueConverter is running on fluid rather than locked
        // up, since nothing rigid is carrying the load then; use
        // getTorqueConverter() to see what the fluid is doing. A diagnostic,
        // same spirit as getClutchPressure().
        inline double getClutchTorqueCapacity() const { return m_clutchConstraint.m_maxTorque; }

        // The fluid coupling itself, for readouts (speed ratio, torque
        // ratio, the torque each side is actually seeing). Only meaningful
        // for Type::AutomaticTorqueConverter.
        inline const TorqueConverter &getTorqueConverter() const { return m_torqueConverter; }

        // True while the lockup clutch is mechanically bridging the
        // converter -- the "converter unlocked/locked" light a real
        // transmission readout shows.
        inline bool isConverterLocked() const { return m_lockupEngaged; }

        // Dashboard-ready label: "N", a bare gear number for a real manual,
        // or D/S/M-prefixed for an automated transmission's current ladder
        // position ("D3", "S5", "M1", ...) -- just "D" for a CVT sitting in
        // its normal continuous-ratio mode, since there's no discrete gear
        // to show there.
        std::string getGearLabel() const;

    protected:
        void applyGearRatio(double gearRatio);
        void shiftToGear(int newGear);

        // The auto-shift decision (updateAutoShift()) and a paddle pick
        // (changeGear()) both route through here rather than calling
        // shiftToGear() directly -- with ShiftPrepareTimeDrive/Sport at 0
        // (the default) this is a same-tick passthrough to shiftToGear(),
        // identical to the old behavior. Above 0, the target gear is
        // queued in m_newGear and the ratio doesn't actually swap until
        // that prepare timer (see update()) elapses -- the "valve body is
        // still routing pressure" pause a real shift starts with, before
        // shiftToGear()'s own post-swap dip/re-catch even begins.
        void requestShift(int newGear);
        void snapToNearestDiscreteGear();
        void updateAutoShift(double dt);
        void updateCvtRatio(double dt);
        void updateClutchTorqueLimits();

        // Rigid-coupling capacity for an AutomaticTorqueConverter: the
        // lockup clutch's, when it's engaged. Also drives the fluid
        // coupling's own engagement as a side effect, since the two are
        // exactly complementary -- whatever the lockup clutch isn't
        // carrying, the fluid is.
        double updateTorqueConverter();

        // Rigid-coupling capacity for the types with a clutch pack or belt
        // and no fluid coupling at all (DCT/AMT/CVT).
        double updateLaunchClutch();

        // How much of its rated capacity the fluid coupling should be
        // carrying right now. Zero at a standstill with the driver off the
        // gas (nothing to transmit, and a real transmission deliberately
        // drops apply pressure to near-neutral there), rising with throttle
        // for a launch and with input shaft speed once the car is moving.
        // The launch clutch types work the same way in spirit but modulate
        // on slip and engine flare instead -- see updateLaunchClutch().
        double launchEngagement() const;

        // Input-shaft-equivalent speed a given gear implies at the
        // vehicle's current road speed, assuming perfect lock (no
        // slip/flare) -- what a real governor/downshift-protection check
        // actually reasons about, since it has no way to see engine RPM
        // directly. Used both for auto-shift's downshift branch and for
        // changeGear()'s over-rev guard.
        double impliedInputSpeed(int gear) const;

        atg_scs::ClutchConstraint m_clutchConstraint;
        TorqueConverter m_torqueConverter;
        atg_scs::RigidBody *m_rotatingMass;
        Vehicle *m_vehicle;
        Engine *m_engine;

        int m_gear;

        // -1 when no shift is pending; otherwise the gear requestShift()
        // is waiting on m_shiftPrepareTimer to elapse before actually
        // committing to via shiftToGear(). Never legitimately targets -1
        // (Neutral) itself -- dropping to Neutral goes through
        // shiftDown()'s own direct path -- so -1 is an unambiguous sentinel.
        int m_newGear;
        int m_gearCount;
        double *m_gearRatios;
        double m_maxClutchTorque;
        double m_clutchPressure;
        bool m_straightCut;
        Type m_type;
        DriveMode m_driveMode;
        bool m_hasSportMode;
        bool m_allowManualShift;
        double m_shiftUpRpmClosedThrottle;
        double m_shiftUpRpmWOT;
        double m_shiftDownRpmClosedThrottle;
        double m_shiftDownRpmWOT;
        double m_sportShiftUpRpmClosedThrottle;
        double m_sportShiftUpRpmWOT;
        double m_sportShiftDownRpmClosedThrottle;
        double m_sportShiftDownRpmWOT;
        double m_shiftTimeDrive;
        double m_shiftTimeSport;
        double m_shiftTorqueCut;
        double m_shiftPrepareTimeDrive;
        double m_shiftPrepareTimeSport;
        double m_shiftPrepareTimer;

        // AutomaticTorqueConverter mid-drive shifts only -- the gear ratio
        // m_rotatingMass is being smoothly swept away from, toward
        // m_gearRatios[m_gear], over the course of m_shiftTransitionTimer.
        // See shiftToGear()'s sweepRatio branch and update().
        double m_shiftFromGearRatio;

        // Lagged version of updateAutoShift()'s throttle-position blend
        // factor -- see the comment there for why the raw, instantaneous
        // value can't be used directly.
        double m_filteredThrottleOpenness;

        double m_cvtTargetRpm;
        double m_cvtRatioRate;
        double m_cvtRatio;
        double m_converterStallTorque;
        bool m_hasLockupClutch;
        double m_converterStallSpeed;
        double m_launchClutchCreepCapacity;
        bool m_lockupEngaged;
        double m_shiftCooldown;
        double m_shiftTransitionTimer;
        bool m_downshiftProtectionTriggered;
};

#endif /* ATG_ENGINE_SIM_TRANSMISSION_H */
