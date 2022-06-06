#include "../include/exhaust_system.h"

#include "../include/units.h"

ExhaustSystem::ExhaustSystem() {
    m_flowK = 0;
    m_flow = 0;
}

ExhaustSystem::~ExhaustSystem() {
    /* void */
}

void ExhaustSystem::initialize(Parameters &params) {
    m_system.initialize(
            units::pressure(1.0, units::atm),
            params.volume,
            units::celcius(25.0));
    m_flowK = params.flowK;
}

void ExhaustSystem::destroy() {
    /* void */
}

void ExhaustSystem::start() {
    m_system.start();
}

void ExhaustSystem::process(double dt) {
    GasSystem::Mix airMix;
    airMix.p_fuel = 0;
    airMix.p_inert = 1.0;
    airMix.p_o2 = 0.0;

    m_flow = m_system.flow(
        m_flowK,
        dt,
        units::pressure(0.9, units::atm),
        units::celcius(25),
        airMix);
}

void ExhaustSystem::end() {
    m_system.end();
}
