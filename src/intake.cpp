#include "../include/intake.h"

#include "../include/units.h"

#include <cmath>

Intake::Intake() {
    m_inputFlowK = 0;
    m_idleFlowK = 0;
    m_flow = 0;
    m_throttle = 1.0;
    m_idleThrottlePlatePosition = 0.0;
    m_flowRate = 0;
    m_totalFuelInjected = 0;
    m_molecularAfr = 0;
    m_flowAttenuationGamma = 1.0;
}

Intake::~Intake() {
    /* void */
}

void Intake::initialize(Parameters &params) {
    m_system.initialize(
            units::pressure(1.0, units::atm),
            params.Volume,
            units::celcius(25.0));
    m_inputFlowK = params.InputFlowK;
    m_molecularAfr = params.MolecularAfr;
    m_idleFlowK = params.IdleFlowK;
    m_idleThrottlePlatePosition = params.IdleThrottlePlatePosition;
    m_flowAttenuationGamma = params.FlowAttenuationGamma;
}

void Intake::destroy() {
    /* void */
}

void Intake::start() {
    m_system.start();
}

void Intake::process(double dt) {
    const double ideal_afr = 0.8 * m_molecularAfr * 4;
    const double current_afr = (m_system.mix().p_o2 + m_system.mix().p_inert) / m_system.mix().p_fuel;

    const double p_air = ideal_afr / (1 + ideal_afr);
    GasSystem::Mix fuelAirMix;
    fuelAirMix.p_fuel = 1 - p_air;
    fuelAirMix.p_inert = p_air * 0.75;
    fuelAirMix.p_o2 = p_air * 0.25;

    const double idle_afr = 2.0;
    const double p_idle_air = idle_afr / (1 + idle_afr);
    GasSystem::Mix fuelMix;
    fuelMix.p_fuel = (1.0 - p_idle_air);
    fuelMix.p_inert = p_idle_air * 0.75;
    fuelMix.p_o2 = p_idle_air * 0.25;

    const double throttle = getThrottlePlatePosition();
    const double flowAttenuation = std::pow(std::cos(throttle * 3.14159 / 2), m_flowAttenuationGamma);

    m_flow =
        m_system.flow(
                flowAttenuation * m_inputFlowK,
                dt,
                units::pressure(1.0, units::atm),
                units::celcius(25),
                fuelAirMix);

    const double idleCircuitFlow = m_system.flow(
        m_idleFlowK,
        dt,
        units::pressure(1.0, units::atm),
        units::celcius(25),
        fuelMix);

    if (m_flow < 0) {
        m_totalFuelInjected += -fuelAirMix.p_fuel * m_flow;
    }

    if (idleCircuitFlow < 0) {
        m_totalFuelInjected += -fuelMix.p_fuel * idleCircuitFlow;
    }

    //m_system.changeMix(fuelAirMix);
}

void Intake::end() {
    m_system.end();
}
