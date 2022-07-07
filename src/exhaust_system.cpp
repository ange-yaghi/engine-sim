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
    m_system.setGeometry(
        units::distance(1.0, units::m),
        units::distance(3.0, units::inch),
        1.0,
        0.0);

    m_atmosphere.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1000.0, units::m3),
        units::celcius(25.0));
    m_atmosphere.setGeometry(
        units::distance(10.0, units::m),
        units::distance(10.0, units::m),
        1.0,
        0.0);

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

    GasSystem::FlowParameters flowParams;
    flowParams.accelerationTimeConstant = 0.01;
    flowParams.crossSectionArea_0 = units::area(100, units::cm2);
    flowParams.crossSectionArea_1 = units::area(100, units::cm2);
    flowParams.direction_x = 0.0;
    flowParams.direction_y = -1.0;
    flowParams.dt = dt;

    m_atmosphere.reset(units::pressure(1.0, units::atm), units::celcius(25.0), airMix);
    flowParams.system_0 = &m_atmosphere;
    flowParams.system_1 = &m_system;
    flowParams.k_flow = m_flowK;

    m_flow = m_system.flow(flowParams);
}

void ExhaustSystem::end() {
    m_system.end();
}
