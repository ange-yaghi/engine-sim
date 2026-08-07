#include "../include/turbocharger.h"

#include "../include/exhaust_system.h"
#include "../include/intake.h"
#include "../include/supercharger.h"
#include "../include/constants.h"
#include "../include/utilities.h"

#include <algorithm>
#include <cmath>

Turbocharger::Turbocharger() {
    /* void */
}

Turbocharger::~Turbocharger() {
    /* void */
}

void Turbocharger::initialize(const Parameters &params) {
    m_parameters = params;
    m_omega = 0.0;
    m_pressureRatio = 1.0;

    // Wheel-size scaling: relative to the baseline diameters (Parameters'
    // own defaults, which reproduce a scale factor of 1 so turbos that
    // don't set these are unaffected), a bigger exducer scales up
    // turbine_flow_rate (~exducer flow area, quadratic in diameter) and a
    // bigger inducer scales up turbine_inertia (~wheel size^5). Applied
    // once here so every downstream use of these two fields (flow loop,
    // compressor referenceFlow, friction coefficient below) automatically
    // picks up the size-derived value.
    const double exducerScale =
        std::max(1e-6, params.turbineExducerDiameter) / BaselineTurbineExducerDiameter;
    const double inducerScale =
        std::max(1e-6, params.turbineInducerDiameter) / BaselineTurbineInducerDiameter;
    m_parameters.turbineRateK *= exducerScale * exducerScale;
    m_parameters.turbineInertia *= std::pow(inducerScale, 5.0);

    // Derive the (PR - 1) = gain * omega^2 curve from the user-facing
    // spool_speed/max_boost rating -- same idea as k_carb()/k_28inH2O()
    // turning an intuitive rating into an internal coefficient.
    const double maxPressureRatio =
        1.0 + m_parameters.maxBoost / units::pressure(1.0, units::atm);
    const double spoolSpeed = std::max(m_parameters.spoolSpeed, 1.0);
    m_pressureRatioGain = (maxPressureRatio - 1.0) / (spoolSpeed * spoolSpeed);

    const double spinDownTime = std::max(m_parameters.spinDownTime, 0.05);
    m_frictionCoefficient = m_parameters.turbineInertia / spinDownTime;

    GasSystem::Mix airMix;
    airMix.p_inert = 1.0;
    m_turbineOutlet.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1000.0, units::m3),
        units::celcius(25.0),
        airMix);
    m_turbineOutlet.setGeometry(
        units::distance(10.0, units::m),
        units::distance(10.0, units::m),
        1.0,
        0.0);

    // Idealized zero-impedance reservoir snapped to the compressor's
    // instantaneous characteristic pressure each substep -- same "infinite
    // boundary condition" role m_turbineOutlet/Intake's m_atmosphere play
    // elsewhere. The actual charge-pipe capacitance lives in
    // m_compressorDischarge below.
    m_compressorSource.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1000.0, units::m3),
        units::celcius(25.0),
        airMix);
    m_compressorSource.setGeometry(
        units::distance(10.0, units::m),
        units::distance(10.0, units::m),
        1.0,
        0.0);

    const double dischargeVolume = std::max(m_parameters.compressorDischargeVolume, 1E-6);
    const double dischargeLength = units::distance(0.3, units::m);
    m_compressorDischargeCrossSection = dischargeVolume / dischargeLength;
    m_compressorDischarge.initialize(
        units::pressure(1.0, units::atm),
        dischargeVolume,
        units::celcius(25.0),
        airMix);
    m_compressorDischarge.setGeometry(
        std::sqrt(m_compressorDischargeCrossSection),
        dischargeLength,
        1.0,
        0.0);
}

Turbocharger::Parameters Turbocharger::recoverSpecParameters() const {
    Parameters spec = m_parameters;

    const double exducerScale =
        std::max(1e-6, m_parameters.turbineExducerDiameter) / BaselineTurbineExducerDiameter;
    const double inducerScale =
        std::max(1e-6, m_parameters.turbineInducerDiameter) / BaselineTurbineInducerDiameter;

    spec.turbineRateK = m_parameters.turbineRateK / (exducerScale * exducerScale);
    spec.turbineInertia = m_parameters.turbineInertia / std::pow(inducerScale, 5.0);

    return spec;
}

void Turbocharger::destroy() {
    /* void */
}

void Turbocharger::addExhaustSystem(ExhaustSystem *exhaust) {
    m_exhaustSystems.push_back(exhaust);
    exhaust->setTurbocharged(true);
}

void Turbocharger::process(double dt) {
    const double atm = units::pressure(1.0, units::atm);

    GasSystem::Mix airMix;
    airMix.p_inert = 1.0;
    m_turbineOutlet.reset(atm, units::celcius(25.0), airMix);

    // Turbine side: replaces each attached exhaust system's own outlet
    // flow (see ExhaustSystem::process()), so the turbine's k_flow is what
    // sets exhaust backpressure. Extracted shaft power is the isentropic
    // enthalpy drop across whatever actually flowed, scaled by efficiency.
    double turbinePower = 0.0;
    for (ExhaustSystem *exhaust : m_exhaustSystems) {
        GasSystem *collector = exhaust->getSystem();
        const double P0 = collector->pressure();
        const double T0 = collector->temperature();

        GasSystem::FlowParameters flowParams;
        flowParams.crossSectionArea_0 = exhaust->getCollectorCrossSectionArea();
        flowParams.crossSectionArea_1 = units::area(10, units::m2);
        flowParams.direction_x = 1.0;
        flowParams.direction_y = 0.0;
        flowParams.dt = dt;
        flowParams.system_0 = collector;
        flowParams.system_1 = &m_turbineOutlet;
        flowParams.k_flow = m_parameters.turbineRateK;

        const double flow_n = GasSystem::flow(flowParams);

        collector->dissipateExcessVelocity();
        collector->updateVelocity(dt, exhaust->getVelocityDecay());

        if (flow_n > 0 && P0 > atm && T0 > 0) {
            const double hcr = collector->heatCapacityRatio();
            const double cp = hcr * constants::R / (units::AirMolecularMass * (hcr - 1.0));
            const double mdot = (flow_n / dt) * units::AirMolecularMass;
            const double pressureRatio = atm / P0;
            const double expansionFactor = 1.0 - std::pow(pressureRatio, (hcr - 1.0) / hcr);

            turbinePower +=
                m_parameters.turbineEfficiency * mdot * cp * T0
                * std::max(0.0, expansionFactor);
        }
    }

    // Compressor side: a cubic (Moore-Greitzer-style) pressure-vs-flow
    // characteristic, evaluated at the current shaft speed and last
    // substep's flow coefficient, sets the pressure this "virtual"
    // compressor outlet can sustain. Flowing that against the real,
    // stateful m_compressorDischarge plenum (rather than assigning its
    // pressure directly) is what lets the two states -- flow and plenum
    // pressure -- actually interact and oscillate.
    const double T_atm = units::celcius(25.0);
    const double hcr = GasSystem::heatCapacityRatio(5);

    // Zero-flow ("shutoff") head: same (PR - 1) = gain * omega^2 relation
    // the old model used unconditionally is now just the peak of the
    // characteristic at zero flow.
    const double shutoffHead = m_pressureRatioGain * m_omega * m_omega;

    // Reference flow scale to normalize the flow coefficient against,
    // built from the same k_flow/pressure/temperature scaling GasSystem's
    // own flow functions use internally -- not a claim of an exact mass
    // flow, just a self-consistent order-of-magnitude scale for this
    // specific turbo's rating.
    const double referenceFlow = m_parameters.turbineRateK * atm / std::sqrt(constants::R * T_atm);

    constexpr double W = 0.5; // hump semi-width, fraction of referenceFlow
    constexpr double H_fraction = 0.25; // hump semi-height, fraction of shutoff head

    const double H = H_fraction * shutoffHead;
    const double phi = (referenceFlow > 0.0) ? (m_compressorFlowRate / referenceFlow) : 0.0;
    const double x = phi / W - 1.0;
    const double head = shutoffHead + H * (1.0 + 1.5 * x - 0.5 * x * x * x);

    // This is m_compressorSource's pressure -- the supply side of a flow
    // *into* m_compressorDischarge, which the intake is simultaneously
    // draining from on the other side (see Intake::process()). Capping this
    // at exactly maxPressureRatio was a bug, not a safety margin: a capped
    // source feeding a continuously-draining plenum can only ever settle
    // *below* the cap in steady state, since sustaining any inflow at all
    // needs a positive source/plenum pressure differential. That's what was
    // causing the delivered (plenum-side) boost to under-shoot max_boost
    // under real load even though the source was pegged right at it. Give
    // the source real headroom above the setpoint so it can actually push
    // the plenum up to max_boost against consumption; the shaft-freeze
    // below (gated on the real delivered m_pressureRatio) is what actually
    // holds the *delivered* pressure at the setpoint, this is just a loose
    // backstop against the source running away entirely.
    const double maxPressureRatio = 1.0 + m_parameters.maxBoost / atm;
    const double compressorPressureRatio =
        clamp(1.0 + head, 0.3, maxPressureRatio * 1.3);
    const double idealRatio = std::pow(compressorPressureRatio, (hcr - 1.0) / hcr);

    // Twincharger staging: compressorPressureRatio is this turbo's OWN
    // multiplier (max_boost is specced relative to whatever it's fed),
    // applied on top of the upstream supercharger's real, RPM-varying
    // discharge conditions instead of flat atmosphere -- so the two
    // stages' boost genuinely multiplies (e.g. 1.4x from the blower times
    // 1.6x from the turbo, not either one alone). Falls back to plain
    // atmosphere with no upstream supercharger, reproducing the original
    // single-stage behavior exactly.
    const double inletPressure = (m_upstreamSupercharger != nullptr)
        ? m_upstreamSupercharger->getCompressorDischargePressure()
        : atm;
    const double inletTemperature = (m_upstreamSupercharger != nullptr)
        ? m_upstreamSupercharger->getCompressorDischargeTemperature()
        : T_atm;

    const double compressorSourceTemperature =
        inletTemperature + (inletTemperature * (idealRatio - 1.0)) / m_parameters.compressorEfficiency;

    m_compressorSource.reset(compressorPressureRatio * inletPressure, compressorSourceTemperature, airMix);

    GasSystem::FlowParameters compressorFlowParams;
    compressorFlowParams.crossSectionArea_0 = units::area(10, units::m2);
    compressorFlowParams.crossSectionArea_1 = m_compressorDischargeCrossSection;
    compressorFlowParams.direction_x = 1.0;
    compressorFlowParams.direction_y = 0.0;
    compressorFlowParams.dt = dt;
    compressorFlowParams.system_0 = &m_compressorSource;
    compressorFlowParams.system_1 = &m_compressorDischarge;
    compressorFlowParams.k_flow = m_parameters.turbineRateK;

    const double compressorFlow_n = GasSystem::flow(compressorFlowParams);
    m_compressorDischarge.dissipateExcessVelocity();
    m_compressorDischarge.updateVelocity(dt, 0.5);

    // m_compressorFlowRate feeds back into phi/head (and hence
    // compressorPressureRatio) on the *next* substep -- assigning it
    // directly from this substep's raw flow closes a one-step-delayed loop
    // through the cubic characteristic with no damping, which chatters
    // (flow sign flipping every single substep) well outside the throttle-
    // snap surge this model is meant to capture. Low-pass it so the
    // feedback tracks real (multi-substep) surge and load transients
    // without ringing at the substep rate.
    const double instantCompressorFlowRate = compressorFlow_n / dt;
    constexpr double flowRateSmoothing = 0.2;
    m_compressorFlowRate +=
        flowRateSmoothing * (instantCompressorFlowRate - m_compressorFlowRate);

    // Backflow (surge reversal) does no useful work on the shaft -- only
    // forward flow against the ideal isentropic rise counts.
    double compressorPower = 0.0;
    if (compressorFlow_n > 0.0) {
        const double cp = hcr * constants::R / (units::AirMolecularMass * (hcr - 1.0));
        const double mdot = m_compressorFlowRate * units::AirMolecularMass;
        const double idealWork = cp * T_atm * std::max(0.0, idealRatio - 1.0);

        compressorPower = mdot * idealWork / m_parameters.compressorEfficiency;
    }

    const double frictionPower = m_frictionCoefficient * m_omega * m_omega;
    double netPower = turbinePower - compressorPower - frictionPower;

    // Gate on shutoffHead -- a clean function of shaft speed alone -- rather
    // than the noisy, phi/hump-dependent head, or the real but
    // plenum-filtered delivered pressure (m_pressureRatio). Both of those
    // were tried and both failed in opposite directions: head's hump can
    // swing well positive *or* negative substep to substep (the compressor
    // map's choke side goes sharply negative at high flow), so freezing on
    // it froze the shaft on a lucky transient long before it had actually
    // reached the speed needed to sustain max_boost -- underboost.
    // m_pressureRatio is real and not noisy, but it's low-pass filtered by
    // the discharge plenum's own capacitance under load (the intake is
    // draining it the whole time), so by the time delivered pressure
    // visibly crossed the setpoint the shaft had already been accelerating
    // ungoverned for the plenum's whole response lag -- overboost, just
    // like the original bug. shutoffHead has neither problem: it only
    // depends on the shaft's own slowly-changing speed. Pre-discount the
    // threshold by the hump's maximum possible contribution (H_fraction)
    // so that even if the hump maxes out right after the freeze engages,
    // head (and hence the source pressure feeding the plenum) tops out at
    // the setpoint instead of H_fraction past it.
    if (shutoffHead >= (maxPressureRatio - 1.0) / (1.0 + H_fraction) && netPower > 0.0) {
        // ponytail: real wastegates bleed exhaust around the turbine to
        // hold boost; this just freezes shaft growth at the setpoint,
        // which gives the same end result (boost caps at max_boost)
        // without modeling the bypass flow itself.
        netPower = 0.0;
    }

    const double kineticEnergy = std::max(
        0.0,
        0.5 * m_parameters.turbineInertia * m_omega * m_omega + netPower * dt);
    m_omega = std::sqrt(2.0 * kineticEnergy / m_parameters.turbineInertia);

    // Read back the real plenum -- boost is now an emergent result of the
    // flow/capacitance balance above, not a formula of shaft speed alone.
    m_pressureRatio = m_compressorDischarge.pressure() / atm;
}

double Turbocharger::getCompressorDischargePressure() const {
    return m_compressorDischarge.pressure();
}

double Turbocharger::getCompressorDischargeTemperature() const {
    return m_compressorDischarge.temperature();
}
