#include "../include/intake.h"

#include "../include/units.h"

Intake::Intake() {
    m_inputFlowK = 0;
    m_flow = 0;
    m_throttle = 1.0;
    m_flowRate = 0;
    m_totalFuelInjected = 0;
    m_molecularAfr = 0;
}

Intake::~Intake() {
    /* void */
}

void Intake::initialize(Parameters &params) {
    m_system.initialize(
            units::pressure(1.0, units::atm),
            params.volume,
            units::celcius(25.0));
    m_inputFlowK = params.inputFlowK;
    m_molecularAfr = params.molecularAfr;
}

void Intake::destroy() {
    /* void */
}

void Intake::start() {
    m_system.start();
}

void Intake::process(double dt) {
    const double p_air = (m_molecularAfr / (m_molecularAfr + 1));
    GasSystem::Mix fuelAirMix;
    fuelAirMix.p_fuel = 1 - p_air;
    fuelAirMix.p_inert = p_air * 0.75;
    fuelAirMix.p_o2 = p_air * 0.25;

    m_flow =
        m_system.flow(
                (1 - m_throttle) * m_inputFlowK,
                dt,
                units::pressure(1.0, units::atm),
                units::celcius(25),
                fuelAirMix);

    if (m_flow < 0) {
        m_totalFuelInjected += -fuelAirMix.p_fuel * m_flow;
    }
}

void Intake::end() {
    m_system.end();
}
