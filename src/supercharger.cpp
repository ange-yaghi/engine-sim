#include "../include/supercharger.h"

#include "../include/crankshaft.h"
#include "../include/constants.h"
#include "../include/utilities.h"

#include <algorithm>
#include <cmath>

Supercharger::Supercharger() {
    /* void */
}

Supercharger::~Supercharger() {
    /* void */
}

void Supercharger::initialize(const Parameters &params) {
    m_parameters = params;
    m_omega = 0.0;
    m_pressureRatio = 1.0;
    m_loadTorque = 0.0;

    GasSystem::Mix airMix;
    airMix.p_inert = 1.0;

    const double atm = units::pressure(1.0, units::atm);
    const double T_atm = units::celcius(25.0);

    // Idealized infinite boundary the compressor draws from -- same role
    // as Intake::m_atmosphere/Turbocharger::m_compressorSource.
    m_inlet.initialize(atm, units::volume(1000.0, units::m3), T_atm, airMix);
    m_inlet.setGeometry(
        units::distance(100.0, units::m),
        units::distance(100.0, units::m),
        1.0,
        0.0);

    // Centrifugal-only: idealized reservoir snapped to the compressor
    // characteristic's instantaneous pressure each substep.
    m_compressorSource.initialize(atm, units::volume(1000.0, units::m3), T_atm, airMix);
    m_compressorSource.setGeometry(
        units::distance(10.0, units::m),
        units::distance(10.0, units::m),
        1.0,
        0.0);

    const double dischargeVolume = std::max(m_parameters.compressorDischargeVolume, 1E-6);
    const double dischargeLength = units::distance(0.3, units::m);
    m_compressorDischargeCrossSection = dischargeVolume / dischargeLength;
    m_compressorDischarge.initialize(atm, dischargeVolume, T_atm, airMix);
    m_compressorDischarge.setGeometry(
        std::sqrt(m_compressorDischargeCrossSection),
        dischargeLength,
        1.0,
        0.0);
}

void Supercharger::destroy() {
    /* void */
}

void Supercharger::process(double dt) {
    if (m_crankshaft == nullptr) {
        m_loadTorque = 0.0;
        return;
    }

    const double omega_crank = m_crankshaft->m_body.v_theta;
    const double omega_crank_abs = std::abs(omega_crank);

    if (m_parameters.clutched) {
        // clutchEngageRpm/clutchDisengageRpm are stored pre-converted to
        // rad/s (units::rpm(...), same convention as spoolSpeed/
        // maxImpellerSpeed elsewhere) -- compare directly against
        // omega_crank_abs, NOT a units::toRpm()'d value, which would
        // compare a raw RPM number against a rad/s one and trip the
        // disengage threshold at a small fraction of the intended RPM.
        if (m_clutchEngaged && omega_crank_abs > m_parameters.clutchDisengageRpm) {
            m_clutchEngaged = false;
        }
        else if (!m_clutchEngaged && omega_crank_abs < m_parameters.clutchEngageRpm) {
            m_clutchEngaged = true;
        }
    }
    else {
        m_clutchEngaged = true;
    }

    if (!m_clutchEngaged) {
        // Clutch open: the rotor isn't being driven (zero crank load) and
        // a bypass valve opens a low-restriction passage straight from
        // inlet to discharge, so whatever's downstream (a turbo's
        // compressor inlet, in the twincharger case) just sees atmosphere
        // pass through rather than a sealed, stagnant volume holding
        // whatever pressure it last had.
        m_omega = 0.0;
        m_loadTorque = 0.0;

        const double atm = units::pressure(1.0, units::atm);
        const double T_atm = units::celcius(25.0);
        GasSystem::Mix airMix;
        airMix.p_inert = 1.0;
        m_inlet.reset(atm, T_atm, airMix);

        GasSystem::FlowParameters bypassParams;
        bypassParams.crossSectionArea_0 = units::area(10, units::m2);
        bypassParams.crossSectionArea_1 = m_compressorDischargeCrossSection;
        bypassParams.direction_x = 1.0;
        bypassParams.direction_y = 0.0;
        bypassParams.dt = dt;
        bypassParams.system_0 = &m_inlet;
        bypassParams.system_1 = &m_compressorDischarge;
        bypassParams.k_flow = GasSystem::k_carb(2000.0);
        GasSystem::flow(bypassParams);

        m_compressorDischarge.dissipateExcessVelocity();
        m_compressorDischarge.updateVelocity(dt, 0.5);

        m_pressureRatio = m_compressorDischarge.pressure() / atm;
        return;
    }

    m_omega = omega_crank_abs * m_parameters.driveRatio;

    const double compressorPower = (m_parameters.type == Type::Centrifugal)
        ? processCentrifugal(dt, m_omega)
        : processPositiveDisplacement(dt, m_omega);

    const double mechanicalEfficiency = clamp(m_parameters.mechanicalEfficiency, 0.05, 1.0);
    const double shaftPower = compressorPower / mechanicalEfficiency;

    // Crank-referred load torque: by power conservation this is exactly
    // shaftPower / omega_crank (the driveRatio cancels out -- gearing
    // multiplies torque and divides speed, so referring the same power
    // back to the crank needs no explicit ratio term here). Clamp the
    // divisor rather than omega_crank_abs itself so a stalled/cranking
    // engine still sees a finite (if momentarily large) load instead of
    // one that blows up as omega_crank_abs -> 0.
    m_loadTorque = shaftPower / std::max(omega_crank_abs, 1.0);

    m_pressureRatio = m_compressorDischarge.pressure() / units::pressure(1.0, units::atm);
}

double Supercharger::processPositiveDisplacement(double dt, double omega) {
    const double atm = units::pressure(1.0, units::atm);
    const double T_atm = units::celcius(25.0);
    const double hcr = GasSystem::heatCapacityRatio(5);

    GasSystem::Mix airMix;
    airMix.p_inert = 1.0;
    m_inlet.reset(atm, T_atm, airMix);

    // A PD blower doesn't choose its pressure ratio -- it's dictated by
    // whatever the discharge plenum already holds, since the pump just
    // keeps moving swept volumes of air into it regardless.
    const double dischargePressure = m_compressorDischarge.pressure();
    const double actualPressureRatio = std::max(1.0, dischargePressure / atm);

    // Volumetric efficiency falls off with pressure ratio -- internal
    // leakback grows with the pressure the trapped pocket has to push
    // against before/while it dumps into the discharge port.
    const double volEff = std::max(
        0.2,
        m_parameters.volumetricEfficiency
            - m_parameters.leakageCoefficient * (actualPressureRatio - 1.0));

    // Swept volume this substep -> moles at inlet conditions. This is
    // what makes a PD blower a volume-metering device: flow depends on
    // shaft speed and displacement, not on how much boost is already
    // built -- the qualitative difference from the centrifugal branch.
    const double sweptVolume = (omega / (2.0 * constants::pi)) * m_parameters.displacement * dt;
    double dn = volEff * m_inlet.n(sweptVolume);

    // Bypass/relief valve: once the plenum is at or past max_boost,
    // recirculate the newly-swept charge back to inlet instead of adding
    // it, the same role a turbo's wastegate plays.
    const double maxPressureRatio = 1.0 + m_parameters.maxBoost / atm;
    if (actualPressureRatio >= maxPressureRatio) {
        const double overshoot =
            (actualPressureRatio - maxPressureRatio) / std::max(1E-3, maxPressureRatio * 0.1);
        dn *= clamp(1.0 - overshoot, 0.0, 1.0);
    }

    dn = std::min(dn, 0.9 * m_inlet.n());
    dn = std::max(dn, 0.0);

    // Real (if approximate) adiabatic compression: the charge is heated
    // on the way from inlet pressure to whatever pressure it's being
    // pushed into, scaled by efficiency to account for the extra heat a
    // real (irreversible) PD compression event adds beyond the ideal case.
    const double idealRatio = std::pow(actualPressureRatio, (hcr - 1.0) / hcr);
    const double dischargeTemperature =
        T_atm + (T_atm * (idealRatio - 1.0)) / m_parameters.compressorEfficiency;

    // Direct moles transfer rather than GasSystem::flow(): a PD pump
    // isn't a pressure-driven orifice (that's the throttle plate and the
    // centrifugal branch below), it's a fixed-volume displacement device,
    // so the transfer amount is set by speed/displacement above, not by
    // a compressible-flow formula.
    const double E_k_per_mol = GasSystem::kineticEnergyPerMol(dischargeTemperature, 5);
    m_inlet.loseN(dn, m_inlet.kineticEnergyPerMol());
    m_compressorDischarge.gainN(dn, E_k_per_mol, airMix);

    m_compressorDischarge.dissipateExcessVelocity();
    m_compressorDischarge.updateVelocity(dt, 0.5);

    if (dn <= 0.0 || dt <= 0.0) return 0.0;

    const double mdot = (dn / dt) * units::AirMolecularMass;
    const double cp = hcr * constants::R / (units::AirMolecularMass * (hcr - 1.0));
    const double idealWork = cp * T_atm * std::max(0.0, idealRatio - 1.0);

    return mdot * idealWork / m_parameters.compressorEfficiency;
}

double Supercharger::processCentrifugal(double dt, double omega) {
    const double atm = units::pressure(1.0, units::atm);
    const double T_atm = units::celcius(25.0);
    const double hcr = GasSystem::heatCapacityRatio(5);

    GasSystem::Mix airMix;
    airMix.p_inert = 1.0;

    const double maxPressureRatio = 1.0 + m_parameters.maxBoost / atm;
    const double maxSpeed = std::max(m_parameters.maxImpellerSpeed, 1.0);
    const double pressureRatioGain = (maxPressureRatio - 1.0) / (maxSpeed * maxSpeed);

    // Same (PR - 1) = gain * omega^2 characteristic Turbocharger's
    // compressor stage uses, evaluated at the current (kinematically
    // crank-derived) shaft speed instead of a free-spinning shaft's
    // integrated speed -- this is what makes boost track crank RPM
    // directly rather than needing spool-up dynamics.
    const double shutoffHead = pressureRatioGain * omega * omega;
    const double compressorPressureRatio = clamp(1.0 + shutoffHead, 0.3, maxPressureRatio);
    const double idealRatio = std::pow(compressorPressureRatio, (hcr - 1.0) / hcr);
    const double sourceTemperature =
        T_atm + (T_atm * (idealRatio - 1.0)) / m_parameters.compressorEfficiency;

    m_compressorSource.reset(compressorPressureRatio * atm, sourceTemperature, airMix);

    GasSystem::FlowParameters flowParams;
    flowParams.crossSectionArea_0 = units::area(10, units::m2);
    flowParams.crossSectionArea_1 = m_compressorDischargeCrossSection;
    flowParams.direction_x = 1.0;
    flowParams.direction_y = 0.0;
    flowParams.dt = dt;
    flowParams.system_0 = &m_compressorSource;
    flowParams.system_1 = &m_compressorDischarge;
    flowParams.k_flow = m_parameters.compressorFlowRateK;

    const double flow_n = GasSystem::flow(flowParams);
    m_compressorDischarge.dissipateExcessVelocity();
    m_compressorDischarge.updateVelocity(dt, 0.5);

    if (flow_n <= 0.0 || dt <= 0.0) return 0.0;

    const double mdot = (flow_n / dt) * units::AirMolecularMass;
    const double cp = hcr * constants::R / (units::AirMolecularMass * (hcr - 1.0));
    const double idealWork = cp * T_atm * std::max(0.0, idealRatio - 1.0);

    return mdot * idealWork / m_parameters.compressorEfficiency;
}

void Supercharger::apply(atg_scs::SystemState *state) {
    if (m_crankshaft == nullptr) return;

    const int index = m_crankshaft->m_body.index;
    if (index < 0) return;

    const double omega = state->v_theta[index];
    const double direction = (omega < 0.0) ? -1.0 : 1.0;

    // Always a drag: opposes whichever way the crank is currently
    // turning, same convention TorqueConverter/RotationFrictionConstraint
    // use for parasitic loads.
    state->t[index] += -direction * m_loadTorque;
}

double Supercharger::getCompressorDischargePressure() const {
    return m_compressorDischarge.pressure();
}

double Supercharger::getCompressorDischargeTemperature() const {
    return m_compressorDischarge.temperature();
}
