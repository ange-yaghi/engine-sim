#include "../include/transmission.h"

#include "../include/constants.h"
#include "../include/units.h"

#include <algorithm>
#include <cmath>

// Floor for the auto-shift cooldown -- without it, a car sitting right on
// a shift-point threshold would hunt between two gears every tick. The
// actual cooldown (see shiftToGear()) is at least this, but is stretched
// to cover the current type's own shift-transition dip plus a little
// margin: firing another shift while the previous one's torque dip is
// still resolving is exactly what let RPM run away from road speed
// instead of resyncing to the new gear ratio (looked like wheelspin, but
// wasn't -- this sim doesn't model tires losing grip at all).
static constexpr double kAutoShiftCooldownFloor = 0.4;
static constexpr double kShiftResyncMargin = 0.15;

// Torque converter lockup clutch. Two thresholds, not one: it engages once
// slip drops below kLockupEngageSlip and only releases again once slip
// climbs back past kLockupReleaseSlip. A single threshold (what this used
// to be, despite the comment claiming otherwise) chatters in and out every
// tick when slip sits right on it, which is exactly the failure mode
// kAutoShiftCooldownFloor exists to prevent for gear changes.
static constexpr double kLockupEngageSlip = 8.0;  // rad/s, ~75 RPM
static constexpr double kLockupReleaseSlip = 22.0; // rad/s, ~210 RPM

// Input shaft speed below which the lockup clutch is never applied, no
// matter how little slip there is -- a real one only comes in at cruising
// speed, and letting it apply near a standstill welds the engine to a
// stopped car (see updateTorqueConverter()).
static constexpr double kLockupMinInputSpeed = 100.0; // rad/s, ~950 RPM

// Speed ratio (input shaft / engine) at and above which the launch device --
// fluid coupling or clutch pack -- is treated as fully engaged. Below it the
// car is barely moving relative to the engine, which is the only regime
// where a launch device has to make a decision at all. Expressed as a ratio
// rather than an absolute RPM so it scales with whatever the engine happens
// to be idling at instead of needing a per-engine tune.
static constexpr double kLaunchLockSpeedRatio = 0.25;

// Slip across a launch clutch (DCT/AMT/CVT -- no fluid coupling) at and
// above which it's treated as fully open, with capacity coming back
// quadratically as the two sides sync up. Sized so that an engine idling
// against a stopped car is comfortably past it -- that mismatch is exactly
// the case the clutch has to refuse to clamp down on.
static constexpr double kLaunchClutchLockSlip = 40.0; // rad/s, ~380 RPM

// Engine speed band, as a fraction of redline, over which a slipping launch
// clutch takes up. Below the floor it's fully released -- an engine down
// there is either idling or already bogging, and neither wants more load --
// and it reaches full launch clamp by the target. Roughly the flare speed a
// real launch holds: ~1800-2400rpm behind a 6500rpm engine, ~2500-4000
// behind a 9000rpm one.
static constexpr double kLaunchFlareFloor = 0.28;
static constexpr double kLaunchFlareTarget = 0.45;

Transmission::Transmission() {
    m_gear = -1;
    m_newGear = -1;
    m_gearCount = 0;
    m_gearRatios = nullptr;
    m_maxClutchTorque = units::torque(1000.0, units::ft_lb);
    m_rotatingMass = nullptr;
    m_vehicle = nullptr;
    m_engine = nullptr;
    m_clutchPressure = 0.0;
    m_straightCut = false;
    m_type = Type::Manual;
    m_driveMode = DriveMode::Drive;
    m_hasSportMode = false;
    m_allowManualShift = false;
    m_shiftUpRpmClosedThrottle = units::rpm(3200);
    m_shiftUpRpmWOT = units::rpm(6200);
    m_shiftDownRpmClosedThrottle = units::rpm(1200);
    m_shiftDownRpmWOT = units::rpm(3400);
    m_sportShiftUpRpmClosedThrottle = units::rpm(4500);
    m_sportShiftUpRpmWOT = units::rpm(6800);
    m_sportShiftDownRpmClosedThrottle = units::rpm(2200);
    m_sportShiftDownRpmWOT = units::rpm(4500);
    m_shiftTimeDrive = 0.5;
    m_shiftTimeSport = 0.35;
    m_shiftTorqueCut = 0.5;
    m_shiftPrepareTimeDrive = 0.0;
    m_shiftPrepareTimeSport = 0.0;
    m_shiftPrepareTimer = 0.0;
    m_shiftFromGearRatio = 0.0;
    m_filteredThrottleOpenness = -1.0; // sentinel: snap to the real value on first use, see updateAutoShift()
    m_cvtTargetRpm = units::rpm(3000);
    m_cvtRatioRate = 1.5;
    m_cvtRatio = 0.0;
    m_converterStallTorque = units::torque(300.0, units::ft_lb);
    m_hasLockupClutch = false;
    m_converterStallSpeed = units::rpm(2500);
    m_launchClutchCreepCapacity = 0.35;
    m_lockupEngaged = false;
    m_shiftCooldown = 0.0;
    m_shiftTransitionTimer = 0.0;
    m_downshiftProtectionTriggered = false;
}

Transmission::~Transmission() {
    if (m_gearRatios != nullptr) {
        delete[] m_gearRatios;
    }

    m_gearRatios = nullptr;
}

void Transmission::initialize(const Parameters &params) {
    m_gearCount = params.GearCount;
    m_maxClutchTorque = params.MaxClutchTorque;
    m_straightCut = params.StraightCut;
    m_type = params.TransmissionType;
    m_hasSportMode = params.HasSportMode;
    m_allowManualShift = params.AllowManualShift;
    m_shiftUpRpmClosedThrottle = params.ShiftUpRpmClosedThrottle;
    m_shiftUpRpmWOT = params.ShiftUpRpmWOT;
    m_shiftDownRpmClosedThrottle = params.ShiftDownRpmClosedThrottle;
    m_shiftDownRpmWOT = params.ShiftDownRpmWOT;
    m_sportShiftUpRpmClosedThrottle = params.SportShiftUpRpmClosedThrottle;
    m_sportShiftUpRpmWOT = params.SportShiftUpRpmWOT;
    m_sportShiftDownRpmClosedThrottle = params.SportShiftDownRpmClosedThrottle;
    m_sportShiftDownRpmWOT = params.SportShiftDownRpmWOT;
    m_shiftTimeDrive = params.ShiftTimeDrive;
    m_shiftTimeSport = params.ShiftTimeSport;
    m_shiftTorqueCut = params.ShiftTorqueCut;
    m_shiftPrepareTimeDrive = params.ShiftPrepareTimeDrive;
    m_shiftPrepareTimeSport = params.ShiftPrepareTimeSport;
    m_cvtTargetRpm = params.CvtTargetRpm;
    m_cvtRatioRate = params.CvtRatioRate;
    m_converterStallTorque = params.ConverterStallTorque;
    m_hasLockupClutch = params.HasLockupClutch;
    m_converterStallSpeed = params.ConverterStallSpeed;
    m_launchClutchCreepCapacity = std::clamp(params.LaunchClutchCreepCapacity, 0.0, 1.0);
    m_gearRatios = new double[params.GearCount];
    memcpy(m_gearRatios, params.GearRatios, sizeof(double) * m_gearCount);

    TorqueConverter::Parameters converterParams{};
    converterParams.StallTorque = params.ConverterStallTorque;
    converterParams.StallSpeed = params.ConverterStallSpeed;
    converterParams.StallTorqueRatio = params.ConverterStallTorqueRatio;
    converterParams.CouplingPoint = params.ConverterCouplingPoint;
    m_torqueConverter.initialize(converterParams);
}

void Transmission::update(double dt) {
    m_shiftTransitionTimer = std::max(0.0, m_shiftTransitionTimer - dt);

    if (m_type == Type::AutomaticTorqueConverter && m_shiftTransitionTimer > 0.0 && m_gear != -1) {
        // Sweeps m_rotatingMass's implied ratio smoothly from
        // m_shiftFromGearRatio toward m_gearRatios[m_gear] over the
        // transition instead of shiftToGear() snapping it in one tick --
        // see the sweepRatio branch there for why an instant snap isn't
        // safe for this type. Smoothstep (zero slope at both ends), not
        // the dip's own asymmetric shape -- this only exists to avoid a
        // velocity discontinuity, not to carry any of the shift's
        // character, which the dip below already does on its own.
        const double duration = m_driveMode == DriveMode::Sport ? m_shiftTimeSport : m_shiftTimeDrive;
        if (duration > 0.0) {
            const double t = std::clamp(1.0 - (m_shiftTransitionTimer / duration), 0.0, 1.0);
            const double eased = t * t * (3.0 - 2.0 * t);
            const double targetRatio = m_gearRatios[m_gear];
            applyGearRatio(m_shiftFromGearRatio + eased * (targetRatio - m_shiftFromGearRatio));
        }
    }

    updateClutchTorqueLimits();

    if (m_type == Type::Manual || m_gear == -1) return;

    m_shiftCooldown = std::max(0.0, m_shiftCooldown - dt);

    if (m_newGear != -1) {
        // A shift is already queued (see requestShift()) -- the valve
        // body/TCU solenoid still working through its own apply lag before
        // the ratio actually swaps. Nothing else gets to decide a
        // different shift while this one's still pending.
        m_shiftPrepareTimer = std::max(0.0, m_shiftPrepareTimer - dt);
        if (m_shiftPrepareTimer <= 0.0) {
            const int target = m_newGear;
            m_newGear = -1;
            shiftToGear(target);
        }
        return;
    }

    if (m_type == Type::CVT) {
        // Sport auto-shifts among the discrete steps in m_gearRatios, same
        // schedule logic as any other automated type; Manual hands full
        // control to the driver, no automatic logic runs at all.
        if (m_driveMode == DriveMode::Drive) updateCvtRatio(dt);
        else if (m_driveMode == DriveMode::Sport) updateAutoShift(dt);
    }
    else if (m_driveMode != DriveMode::Manual) {
        updateAutoShift(dt);
    }
}

void Transmission::updateAutoShift(double dt) {
    if (m_shiftCooldown > 0.0) return;

    const bool sport = m_driveMode == DriveMode::Sport;
    const double shiftUpClosed = sport ? m_sportShiftUpRpmClosedThrottle : m_shiftUpRpmClosedThrottle;
    const double shiftUpWOT = sport ? m_sportShiftUpRpmWOT : m_shiftUpRpmWOT;
    const double shiftDownClosed = sport ? m_sportShiftDownRpmClosedThrottle : m_shiftDownRpmClosedThrottle;
    const double shiftDownWOT = sport ? m_sportShiftDownRpmWOT : m_shiftDownRpmWOT;

    // Real transmission computers -- hydraulic/vacuum valve body or
    // electronic, doesn't matter -- read throttle position, not engine
    // RPM, to decide where the shift points sit: closed throttle shifts
    // early for economy, wide open throttle (WOT) holds the gear for
    // power. getThrottle(): 0 = wide open, 1 = closed.
    const double linearOpenness = (m_engine != nullptr) ? 1.0 - m_engine->getThrottle() : 0.0;

    // Squared, not linear -- a real kickdown/part-throttle-downshift line
    // barely moves through most of the pedal's travel and only really
    // ramps up near the floor. Blending linearly meant any moderate
    // tip-in (not just flooring it) already pushed the downshift point
    // most of the way to its WOT value, so it kicked down far more
    // readily than a real car would.
    const double throttleOpenness = linearOpenness * linearOpenness;

    // Lag the throttle signal feeding the *upshift* point only -- not the
    // downshift/kickdown one below, which stays on the instantaneous value.
    // Without this, flooring it to redline in a low gear and then snapping
    // off the gas instantly collapses shiftUpRpm from its WOT value down to
    // the closed-throttle one within the same tick -- since current RPM is
    // still well above that new, much lower threshold, the upshift fires
    // immediately, same frame as the lift, with none of a real valve
    // body's/TCU's response lag. A real transmission's shift schedule still
    // reacts to a hard lift fairly promptly (a "lift-throttle upshift" for
    // fuel economy/engine-braking is normal and correct) just not literally
    // instantly. The downshift/kickdown line doesn't have this problem --
    // going the other way (tip-in), a slow-to-react threshold would instead
    // delay a wanted kickdown -- so it's left alone.
    if (m_filteredThrottleOpenness < 0.0) {
        // First tick since construction -- snap straight to the real value
        // instead of ramping up/down from an arbitrary default, which would
        // otherwise itself act like a fake, unwanted throttle transient the
        // moment a drive starts (e.g. loading in already at WOT would
        // suppress shiftUpRpm toward the closed-throttle point for the
        // first ~second until the filter caught up).
        m_filteredThrottleOpenness = throttleOpenness;
    }
    else {
        // Short enough that the existing shift-schedule tests' ~0.5s
        // throttle-change windows still land where they expect (this isn't
        // meant to model any real hydraulic/electronic response time,
        // there's no spec for that -- just enough lag that a full lift
        // stops reading as a same-tick teleport).
        constexpr double kThrottleFilterRC = 0.15;
        const double alpha = dt / (dt + kThrottleFilterRC);
        m_filteredThrottleOpenness += (throttleOpenness - m_filteredThrottleOpenness) * alpha;
    }

    const double shiftUpRpm = shiftUpClosed + m_filteredThrottleOpenness * (shiftUpWOT - shiftUpClosed);
    const double shiftDownRpm = shiftDownClosed + throttleOpenness * (shiftDownWOT - shiftDownClosed);

    // Read the engine's own crank speed, not the input shaft. A real TCU's
    // shift-up trigger is about protecting the engine from the limiter, and
    // behind a slipping launch clutch (DCT/AMT launch flare) or an
    // unlocked converter the input shaft can lag the engine by thousands
    // of RPM -- reading it here left the engine pinned on the limiter for
    // as long as it took road speed alone to drag the input shaft up to
    // the WOT shift point, instead of shifting out of the flare almost
    // immediately the way a real launch does. This sim's normal running
    // direction is negative v_theta (see StarterMotor's own default), so
    // the magnitude is what matters, not the signed value.
    const double rpm = (m_engine != nullptr)
        ? std::abs(m_engine->getOutputCrankshaft()->m_body.v_theta)
        : std::abs(m_rotatingMass->v_theta);

    if (rpm > shiftUpRpm && m_gear < m_gearCount - 1) {
        requestShift(m_gear + 1);
    }
    else if (rpm < shiftDownRpm && m_gear > 0
        && (m_engine == nullptr || m_engine->getRedline() <= 0.0
            || impliedInputSpeed(m_gear - 1) <= m_engine->getRedline())
        && impliedInputSpeed(m_gear - 1) < shiftUpRpm) {
        // Downshift protection -- don't drop into a gear that would send
        // the engine past redline at the current road speed. In practice
        // the schedule itself (shiftDownRpm sitting well below shiftUpRpm)
        // keeps this from tripping often, but it's a real backstop for
        // anything unusual (an aggressively part-throttle-blended
        // schedule, coasting in a taller gear than the speed calls for).
        //
        // Second clause: hunt guard. Don't drop into a gear that would
        // land the road-speed-implied input speed back above the CURRENT
        // upshift line -- otherwise the very next tick's upshift branch
        // fires again and it bounces between the two gears forever. This
        // is normally covered by shiftDownRpm sitting well below
        // shiftUpRpm for any given throttle, but shiftUpRpm is lagged
        // (m_filteredThrottleOpenness above) while shiftDownRpm reacts
        // instantly -- a hard tip-in spikes the instant shiftDownRpm
        // (correctly asking for a kickdown) before the filtered
        // shiftUpRpm has caught up to that same higher throttle, and
        // without this the transmission would downshift into a gear it
        // immediately wants to leave again. This just holds the current
        // gear until the filter catches up enough to clear it, instead of
        // firing the shift and immediately reversing it.
        requestShift(m_gear - 1);
    }
}

void Transmission::updateCvtRatio(double dt) {
    double minRatio = m_gearRatios[0];
    double maxRatio = m_gearRatios[0];
    for (int i = 1; i < m_gearCount; ++i) {
        minRatio = std::min(minRatio, m_gearRatios[i]);
        maxRatio = std::max(maxRatio, m_gearRatios[i]);
    }

    const double wheelOmega = m_vehicle->getSpeed() / m_vehicle->getTireRadius();
    const double targetRatio = (wheelOmega > 1e-3)
        ? std::clamp(m_cvtTargetRpm / (wheelOmega * m_vehicle->getDiffRatio()), minRatio, maxRatio)
        : maxRatio; // stopped/near-stopped -- sit at the lowest (launch) ratio

    const double maxDelta = m_cvtRatioRate * dt;
    m_cvtRatio += std::clamp(targetRatio - m_cvtRatio, -maxDelta, maxDelta);

    applyGearRatio(m_cvtRatio);
}

void Transmission::updateClutchTorqueLimits() {
    if (m_gear == -1) {
        m_clutchConstraint.m_minTorque = 0;
        m_clutchConstraint.m_maxTorque = 0;
        m_torqueConverter.setEnabled(false);
        m_lockupEngaged = false;
        if (m_engine != nullptr) m_engine->getIgnitionModule()->setShiftCut(false);
        return;
    }

    double torqueCapacity = m_maxClutchTorque;

    if (m_type == Type::Manual) {
        torqueCapacity = m_maxClutchTorque * m_clutchPressure;
    }
    else if (m_type == Type::AutomaticTorqueConverter) {
        torqueCapacity = updateTorqueConverter();
    }
    else {
        // DCT/AMT/CVT: no fluid coupling, so the launch device is a
        // slipping clutch pack (or belt) that has to be modulated instead.
        torqueCapacity = updateLaunchClutch();
    }

    double dip = 0.0;
    if (m_type != Type::Manual && m_shiftTransitionTimer > 0.0) {
        const double duration = m_driveMode == DriveMode::Sport ? m_shiftTimeSport : m_shiftTimeDrive;
        if (duration > 0.0) {
            const double t = 1.0 - (m_shiftTransitionTimer / duration);

            if (m_type == Type::AutomaticTorqueConverter) {
                // A fluid coupling doesn't release and reapply
                // symmetrically the way a clutch pack does. The outgoing
                // clutch pack/band vents its own apply pressure and goes
                // slack quickly -- the engine flares almost immediately,
                // there's nothing holding it back. But the incoming one
                // has to build pressure gradually or it'd slam the
                // drivetrain, so reeling that flare back in takes most of
                // the shift. That's the classic "RPM hangs up there for a
                // beat before it catches" feel a slushy old box makes
                // obvious; a modern tight-tolerance converter box (e.g. a
                // ZF8HP) still shifts this same shape, just fast enough
                // overall that the asymmetry is barely perceptible.
                // Quarter-duration quick release (quarter sine, easing
                // into full dip), three-quarter-duration slow re-catch
                // (quarter cosine, easing back out of it).
                constexpr double kReleaseFraction = 0.25;
                if (t < kReleaseFraction) {
                    dip = m_shiftTorqueCut
                        * std::sin((constants::pi / 2.0) * (t / kReleaseFraction));
                }
                else {
                    const double recatchT = (t - kReleaseFraction) / (1.0 - kReleaseFraction);
                    dip = m_shiftTorqueCut * std::cos((constants::pi / 2.0) * recatchT);
                }
            }
            else {
                // DCT/AMT/CVT: a real clutch pack (or belt) both
                // disengaging and reengaging is a much more symmetric
                // event -- there's no fluid coupling left stewing slack
                // while apply pressure slowly builds back up. Half-sine
                // dip: full capacity at the start/end of the transition,
                // down to (1 - ShiftTorqueCut) at the midpoint.
                dip = m_shiftTorqueCut * std::sin(constants::pi * t);
            }

            torqueCapacity *= (1.0 - dip);

            // The dip has to reach the fluid coupling too, or an unlocked
            // converter would sail through the shift at full strength while
            // every other type eases off.
            m_torqueConverter.setEngagement(
                m_torqueConverter.isEnabled()
                    ? m_torqueConverter.getEngagement() * (1.0 - dip)
                    : 0.0);
        }
    }

    m_clutchConstraint.m_minTorque = -torqueCapacity;
    m_clutchConstraint.m_maxTorque = torqueCapacity;

    // DCT/AMT/CVT genuinely need this: there's a real clutch pack (or
    // belt) that has to move freely for the handoff, and a timing retard
    // alone isn't a strong enough effect to reliably unload it -- so the
    // ECU cuts power outright for the moment of the shift, felt at the
    // engine itself and not just the coupling above.
    //
    // A torque-converter automatic is a different animal: there's no
    // synchro/dog/clutch-pack for the engine's own power to interfere
    // with, just planetary clutch packs shifting inside a fluid coupling
    // that's already carrying the dip above. Cutting ignition there too
    // would kill the engine's own torque right as the coupling goes slack
    // -- exactly backwards from the real thing, where the engine keeps
    // making full power through a momentarily unloaded coupling and
    // FLARES up before the next gear's clutch pack catches and pulls it
    // back down. That flare-then-catch, not a power cut, is the
    // characteristic feel (slushy old-school ones flare harder and longer
    // than a crisp modern box, same shift_time_drive/shift_torque_cut
    // knobs already capture that). Independent of anti-lag's own use of
    // timing retard (see IgnitionModule::setShiftCut() / setTimingRetard()).
    if (m_engine != nullptr) {
        const bool clutchPackHandoff = m_type != Type::Manual && m_type != Type::AutomaticTorqueConverter;
        m_engine->getIgnitionModule()->setShiftCut(clutchPackHandoff && m_shiftTransitionTimer > 0.0);
    }
}

double Transmission::launchEngagement() const {
    // Once the input shaft is turning at a decent fraction of engine speed
    // the car is genuinely moving, and the launch device has nothing left to
    // decide -- hand it full engagement and let the coupling's own physics
    // take over from there.
    const double engineOmega = (m_engine != nullptr)
        ? std::abs(m_engine->getOutputCrankshaft()->m_body.v_theta)
        : 0.0;
    const double inputOmega = std::abs(m_rotatingMass->v_theta);
    const double speedRatio = (engineOmega > 1e-6) ? inputOmega / engineOmega : 1.0;
    const double rolling = std::clamp(speedRatio / kLaunchLockSpeedRatio, 0.0, 1.0);

    // Below that the car is at or near a standstill, and how much torque the
    // launch device should carry stops being a question about the drivetrain
    // and becomes one about the driver. Off the gas the answer is
    // essentially none: a real TCU backs converter apply pressure off toward
    // neutral at a stop ("neutral idle", standard on basically every modern
    // automatic) and a real DCT simply holds its launch clutch open. It
    // feeds back in as the pedal goes down. getThrottle(): 0 = WOT,
    // 1 = closed.
    const double pedal = (m_engine != nullptr) ? 1.0 - m_engine->getThrottle() : 0.0;

    return std::max(rolling, pedal);
}

double Transmission::updateTorqueConverter() {
    if (m_engine == nullptr) {
        m_torqueConverter.setEnabled(false);
        return m_maxClutchTorque;
    }

    const double engineOmega = m_engine->getOutputCrankshaft()->m_body.v_theta;
    const double slip = std::abs(engineOmega - m_rotatingMass->v_theta);

    // Modern converters lock solidly once slip settles down (steady cruise)
    // for efficiency; an old one with no lockup clutch never does and just
    // always runs on the fluid coupling -- mushier, and always shedding a
    // bit of power as heat even at a steady speed. Latched between two
    // thresholds so it can't chatter in and out on the boundary.
    //
    // Low slip on its own isn't enough to justify locking, though: a
    // stationary car with a stalling engine also has almost no slip across
    // the converter, and bridging it there bolts the engine straight to a
    // parked mass -- a guaranteed kill, and the exact thing the fluid
    // coupling exists to prevent. Real transmissions only apply lockup
    // above a cruising road speed and never in first, so require the input
    // shaft to actually be turning before it's a candidate at all.
    const bool lockupPermitted = m_hasLockupClutch
        && m_gear > 0
        && std::abs(m_rotatingMass->v_theta) > kLockupMinInputSpeed;

    if (lockupPermitted) {
        if (m_lockupEngaged) m_lockupEngaged = slip <= kLockupReleaseSlip;
        else m_lockupEngaged = slip < kLockupEngageSlip;
    }
    else {
        m_lockupEngaged = false;
    }

    if (m_lockupEngaged) {
        // Mechanically bridged -- the fluid isn't carrying anything.
        m_torqueConverter.setEnabled(false);
        return m_maxClutchTorque;
    }

    // Unlocked: the fluid does all the work, and it does it by applying
    // torque, not by dragging the two sides toward a common speed. Nothing
    // rigid is in the path, so the constraint's capacity is zero and
    // TorqueConverter::apply() is what actually couples the drivetrain.
    m_torqueConverter.setEnabled(true);
    m_torqueConverter.setEngagement(launchEngagement());

    return 0.0;
}

double Transmission::updateLaunchClutch() {
    m_torqueConverter.setEnabled(false);
    m_lockupEngaged = false;

    if (m_engine == nullptr) return m_maxClutchTorque;

    // A clutch pack has no fluid to slip through, so unlike the converter it
    // stays a genuine rigid coupling -- it just isn't allowed to grab at
    // full capacity while there's still a big speed mismatch across it.
    // Bolting a running engine straight to a stationary two-tonne car is
    // precisely what was killing these; a real DCT/AMT/CVT slips its launch
    // device instead, lets the engine flare, and feeds torque in as the car
    // picks up speed.
    //
    // Capacity keys off slip rather than road speed because that's what
    // actually matters: a clutch that's already synced can be clamped shut
    // with no consequence (there's no relative motion for it to fight), and
    // one that isn't is the only case where capacity decides anything.
    const double engineOmega = std::abs(m_engine->getOutputCrankshaft()->m_body.v_theta);
    const double inputOmega = std::abs(m_rotatingMass->v_theta);
    const double slip = std::abs(
        m_engine->getOutputCrankshaft()->m_body.v_theta - m_rotatingMass->v_theta);

    // Once the input shaft is spinning at a decent fraction of engine speed,
    // the car is genuinely underway and the launch device has nothing left
    // to decide -- same "rolling" hand-off launchEngagement() already gives
    // the torque converter case. Without it, capacity here never has
    // anything pushing it past whatever the slip-based mismatch penalty and
    // the redline-relative flare/overrun bands allow (both keyed off engine
    // RPM and slip, neither of which cares that road speed -- and therefore
    // the gap it's slipping across -- keeps climbing) -- an engine that
    // settles into equilibrium against a partially-slipping clutch just
    // sits there indefinitely while the car keeps accelerating out from
    // under it, RPM pinned, speed climbing, never actually locking up.
    //
    // Unlike locked/launch below, a plain ratio has no throttle gate and is
    // dangerously sensitive right where it matters most: idling in gear at
    // a standstill, engineOmega is low (idle rpm), so it takes only a tiny
    // -- even solver-noise-level -- nonzero inputOmega to swing the ratio
    // past the rolling threshold and hand a rigid clutch full capacity
    // against a car that's still, for all practical purposes, parked. A
    // fluid coupling (the torque converter case this pattern was copied
    // from) shrugs that off -- its absorption still scales with speed
    // regardless of "engagement" -- but a rigid clutch constraint doesn't
    // have that cushion; full capacity there is a weld. So gate it behind
    // the same absolute input-shaft-speed floor the converter's own lockup
    // already uses for exactly this reason (see updateTorqueConverter()) --
    // ratio only gets to matter once the car is unambiguously moving.
    const bool genuinelyRolling = inputOmega > kLockupMinInputSpeed;
    const double speedRatio = (engineOmega > 1e-6) ? inputOmega / engineOmega : 1.0;
    const double rolling = genuinelyRolling
        ? std::clamp(speedRatio / kLaunchLockSpeedRatio, 0.0, 1.0)
        : 0.0;

    // A DCT's incoming clutch pack is already synced to (or very close to)
    // the new gear's speed before the handoff lands -- the only reason slip
    // reads high right at a DCT shift instant is this sim collapsing both
    // physical input shafts into one m_rotatingMass and swapping its
    // implied speed in a single tick (see shiftToGear()/applyGearRatio()),
    // not anything a real DCT's driveline actually experiences. Treating
    // that modeling artifact as a genuine big mismatch would clamp capacity
    // down through this formula on top of the shift's own dedicated dip
    // curve (see updateClutchTorqueLimits()), double-counting the same
    // "reduce torque for the handoff" intent and dragging a DCT into the
    // same slow, capacity-limited fight a single-clutch AMT genuinely has
    // to make (compare DctShiftBarelyDipsTorque to
    // AmtShiftNearlyCutsTorqueMidTransition) -- the whole reason to have
    // two clutches in the first place. So: while a DCT shift is actively in
    // transition, let the dip curve be the sole authority here instead.
    // Every other type -- and DCT itself outside a shift, e.g. an actual
    // launch -- still pays the mismatch penalty in full, since that's a
    // real gap with nothing pre-spun to bridge it.
    const bool dctMidShift = m_type == Type::DualClutch && m_shiftTransitionTimer > 0.0;
    const double mismatch = dctMidShift
        ? 0.0
        : std::clamp(slip / kLaunchClutchLockSlip, 0.0, 1.0);
    const double locked = (1.0 - mismatch) * (1.0 - mismatch);

    // While it IS slipping, apply pressure is the TCU's to modulate, and it
    // modulates against two things: driver demand, and how the engine is
    // actually coping. Demand alone isn't enough -- a fixed slipping
    // capacity is a race between the engine's flywheel and two tonnes of
    // stationary car, and the flywheel loses, which is a bog and then a
    // stall. A real launch clutch (or a centrifugal one, which does this
    // mechanically rather than electronically) instead lets the engine come
    // up onto its torque first and takes more clamp the higher it gets, so
    // the load backs off the moment the engine starts sagging.
    //
    // The flare band is scaled off redline rather than a per-transmission
    // tune, so it lands sensibly whether it's bolted to a 6500rpm economy
    // four or a 9000rpm V10 -- same trick Engine::getSafetyFactor() uses to
    // find an idle floor without being told one.
    const double redline = m_engine->getRedline();
    double flare = 1.0;
    double overrun = 0.0;
    if (redline > 0.0) {
        const double floorSpeed = kLaunchFlareFloor * redline;
        const double targetSpeed = kLaunchFlareTarget * redline;
        flare = std::clamp(
            (engineOmega - floorSpeed) / (targetSpeed - floorSpeed), 0.0, 1.0);

        // Past the flare target the clutch is already carrying everything
        // LaunchClutchCreepCapacity rates it for. If the engine is STILL
        // climbing from there toward redline at full throttle, that's the
        // creep cap alone turning out not to be enough to hold it -- an
        // engine whose peak torque outruns its creep capacity would
        // otherwise just sit flat on the limiter until road speed alone
        // closed the slip gap (see updateAutoShift()'s engine-vs-input-
        // shaft fix for the other half of that failure mode). Keep
        // clamping down harder the closer it gets to redline, same "more
        // clamp as the engine comes up" idea the flare band itself uses,
        // just continued past it instead of flatlining, up to full
        // capacity right at the limiter.
        if (targetSpeed < redline) {
            overrun = std::clamp(
                (engineOmega - targetSpeed) / (redline - targetSpeed), 0.0, 1.0);
        }
    }

    const double pedal = 1.0 - m_engine->getThrottle();
    const double creep = m_launchClutchCreepCapacity * pedal * flare;
    const double launch = creep + overrun * pedal * (1.0 - creep);

    return m_maxClutchTorque * std::max({ locked, launch, rolling });
}

void Transmission::addToSystem(
    atg_scs::RigidBodySystem *system,
    atg_scs::RigidBody *rotatingMass,
    Vehicle *vehicle,
    Engine *engine)
{
    m_rotatingMass = rotatingMass;
    m_vehicle = vehicle;
    m_engine = engine;

    m_clutchConstraint.setBody1(&engine->getOutputCrankshaft()->m_body);
    m_clutchConstraint.setBody2(m_rotatingMass);

    system->addConstraint(&m_clutchConstraint);

    // The impeller is bolted to the flexplate (so, the output crankshaft);
    // the turbine drives the input shaft, which is what m_rotatingMass is.
    // Registered unconditionally -- it's inert unless
    // updateTorqueConverter() enables it, and only an
    // AutomaticTorqueConverter ever calls that.
    m_torqueConverter.connect(&engine->getOutputCrankshaft()->m_body, m_rotatingMass);
    system->addForceGenerator(&m_torqueConverter);
}

bool Transmission::changeGear(int newGear) {
    m_downshiftProtectionTriggered = false;

    if (newGear < -1 || newGear >= m_gearCount) return false;

    if (m_type == Type::Manual) {
        // Manual transmissions need the clutch disengaged to change gears.
        if (newGear != -1 && m_clutchPressure > 0.1) return false;
    }
    else {
        // Automated transmissions only accept a direct gear pick once
        // already in DriveMode::Manual -- Drive/Sport are computer-
        // controlled, and neutral isn't reachable this way. Use
        // shiftUp()/shiftDown() to drive the ladder that gets here.
        if (newGear == -1 || m_driveMode != DriveMode::Manual) return false;

        // Downshift protection: refuse a paddle-shift request that would
        // send the engine past redline once the new gear ratio takes
        // hold. Real manumatic/DCT/AMT modes do exactly this -- a driver
        // spamming the downshift paddle at speed shouldn't be able to
        // grenade the engine through the transmission the way an actual
        // manual's clutch pedal (no computer in the loop at all) still
        // lets you.
        if (newGear < m_gear && m_engine != nullptr && m_engine->getRedline() > 0.0
            && impliedInputSpeed(newGear) > m_engine->getRedline()) {
            m_downshiftProtectionTriggered = true;
            return false;
        }
    }

    requestShift(newGear);
    return true;
}

bool Transmission::shiftUp() {
    m_downshiftProtectionTriggered = false; // an upshift can never trip it; keep stale state from bleeding in

    if (m_type == Type::Manual) return changeGear(m_gear + 1);

    if (m_gear == -1) {
        shiftToGear(0);
        m_driveMode = DriveMode::Drive;
        return true;
    }

    switch (m_driveMode) {
        case DriveMode::Drive:
            if (m_hasSportMode) {
                m_driveMode = DriveMode::Sport;
                if (m_type == Type::CVT) snapToNearestDiscreteGear();
                return true;
            }
            if (m_allowManualShift) {
                m_driveMode = DriveMode::Manual;
                if (m_type == Type::CVT) snapToNearestDiscreteGear();
                return true;
            }
            return false; // Drive is the only mode this transmission has.

        case DriveMode::Sport:
            if (!m_allowManualShift) return false;
            m_driveMode = DriveMode::Manual;
            return true;

        default: // DriveMode::Manual
            return changeGear(m_gear + 1);
    }
}

bool Transmission::shiftDown() {
    m_downshiftProtectionTriggered = false; // cleared up front; changeGear() sets it back if it actually trips

    if (m_type == Type::Manual) return changeGear(m_gear - 1);

    if (m_gear == -1) return false;

    switch (m_driveMode) {
        case DriveMode::Manual:
            if (m_gear > 0) return changeGear(m_gear - 1);
            // Floor of Manual -- drop back to whatever's below it.
            m_driveMode = m_hasSportMode ? DriveMode::Sport : DriveMode::Drive;
            return true;

        case DriveMode::Sport:
            m_driveMode = DriveMode::Drive;
            return true;

        default: // DriveMode::Drive
            m_gear = -1;
            return true;
    }
}

std::string Transmission::getGearLabel() const {
    if (m_gear == -1) return "N";
    if (m_type == Type::Manual) return std::to_string(m_gear + 1);

    const char modeChar = (m_driveMode == DriveMode::Sport) ? 'S'
        : (m_driveMode == DriveMode::Manual) ? 'M' : 'D';

    if (m_type == Type::CVT && m_driveMode == DriveMode::Drive) {
        return std::string(1, modeChar); // Continuous ratio -- no discrete gear to show.
    }

    return std::string(1, modeChar) + std::to_string(m_gear + 1);
}

double Transmission::impliedInputSpeed(int gear) const {
    const double wheelOmega = m_vehicle->getSpeed() / m_vehicle->getTireRadius();
    return wheelOmega * m_vehicle->getDiffRatio() * m_gearRatios[gear];
}

void Transmission::applyGearRatio(double gearRatio) {
    const double car = m_vehicle->getMass();
    const double diff_ratio = m_vehicle->getDiffRatio();
    const double tire_radius = m_vehicle->getTireRadius();
    const double f = tire_radius / (diff_ratio * gearRatio);

    const double new_I = car * f * f;
    const double E_r =
        0.5 * m_rotatingMass->I * m_rotatingMass->v_theta * m_rotatingMass->v_theta;
    const double new_v_theta = m_rotatingMass->v_theta < 0
        ? -std::sqrt(E_r * 2 / new_I)
        : std::sqrt(E_r * 2 / new_I);

    m_rotatingMass->I = new_I;
    m_rotatingMass->p_x = m_rotatingMass->p_y = 0;
    m_rotatingMass->m = car;
    m_rotatingMass->v_theta = new_v_theta;
}

void Transmission::requestShift(int newGear) {
    const double prepareTime = m_driveMode == DriveMode::Sport
        ? m_shiftPrepareTimeSport : m_shiftPrepareTimeDrive;

    if (prepareTime <= 0.0) {
        // Same-tick passthrough -- every file that doesn't set
        // ShiftPrepareTimeDrive/Sport shifts exactly as instantly as
        // before this existed.
        shiftToGear(newGear);
        return;
    }

    m_newGear = newGear;
    m_shiftPrepareTimer = prepareTime;
}

void Transmission::shiftToGear(int newGear) {
    double cooldown = kAutoShiftCooldownFloor;
    const int previousGear = m_gear;

    if (newGear != -1) {
        // AutomaticTorqueConverter mid-drive shifts sweep the ratio in
        // gradually instead (see update()) rather than snapping it here --
        // an instant snap changes m_rotatingMass's implied speed in a
        // single tick, and since the engine is only connected to it
        // through the fluid coupling (a force generator, not a rigid
        // constraint), that shows up as a sudden, huge slip spike across a
        // coupling that's still at (or near) full engagement -- nothing
        // has had time to dip yet. TorqueConverter::solve()'s absorption
        // scales with the square of impeller speed, so at any real rpm
        // well above stall speed that transient alone is easily
        // thousands of lb-ft for a moment, yanking the engine down far
        // harder and faster than any real shift does -- violent regardless
        // of how gently shift_time_drive/shift_torque_cut are tuned,
        // because it happens before either has a chance to matter. A real
        // planetary auto's ratio doesn't change in zero time either -- the
        // clutch/band apply stroke IS what moves the effective ratio from
        // old to new, gradually, which is what the sweep below actually
        // models. DCT/AMT/CVT and the initial Neutral engage are unaffected.
        const bool sweepRatio = m_type == Type::AutomaticTorqueConverter && previousGear != -1;
        if (sweepRatio) {
            m_shiftFromGearRatio = m_gearRatios[previousGear];
        }
        else {
            applyGearRatio(m_gearRatios[newGear]);
        }

        // A DCT's incoming clutch pack is already spinning at the new
        // gear's speed before the handoff lands -- that's the whole point
        // of having two of them. That's exactly what m_rotatingMass now
        // reads as (applyGearRatio() above just set it), so there's no gap
        // for the engine to fall into the way a single-clutch AMT has to
        // fight through -- but closing the remaining gap to the engine
        // still happens through the ordinary capacity-limited clutch
        // constraint, over the (short) shift transition below, not an
        // instant velocity snap. Bolting the crank straight to the input
        // shaft's speed in a single tick was a real DCT's fast handoff in
        // name only: a genuine 100-200ms overlap still has *some* slip
        // and torque flowing through it, however brief, whereas the snap
        // was a true zero-time discontinuity in engine speed -- audible
        // as a hard pitch pop on every shift instead of the quick, tight
        // dip a real DCT actually makes. That crisp-but-not-instant feel
        // is what a fast ShiftTimeDrive/Sport with a shallow
        // ShiftTorqueCut already models (see DctShiftBarelyDipsTorque).

        // Keeps the CVT's continuous ratio picking up from wherever a
        // discrete shift (Sport/Manual, or the initial engage) left it.
        m_cvtRatio = m_gearRatios[newGear];

        // Engaging out of Neutral isn't a shift. There's no outgoing gear to
        // unload, so there's nothing for a torque dip to help with and no
        // reason to cut ignition -- moving a real lever from N to D doesn't
        // interrupt the engine, it just brings the launch device up against
        // it. Cutting spark here meant the engine spent the whole
        // ShiftTimeDrive making no torque at idle, which killed it outright
        // long before the coupling was ever a factor.
        if (m_type != Type::Manual && previousGear != -1) {
            const double duration = m_driveMode == DriveMode::Sport ? m_shiftTimeSport : m_shiftTimeDrive;
            m_shiftTransitionTimer = duration;
            cooldown = std::max(cooldown, duration + kShiftResyncMargin);
        }
    }

    m_gear = newGear;
    m_shiftCooldown = cooldown;
}

void Transmission::snapToNearestDiscreteGear() {
    if (m_gearCount <= 0) return;

    int nearest = 0;
    double nearestDelta = std::abs(m_gearRatios[0] - m_cvtRatio);
    for (int i = 1; i < m_gearCount; ++i) {
        const double delta = std::abs(m_gearRatios[i] - m_cvtRatio);
        if (delta < nearestDelta) {
            nearest = i;
            nearestDelta = delta;
        }
    }

    shiftToGear(nearest);
}
