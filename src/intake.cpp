#include "../include/intake.h"

#include "../include/units.h"

Intake::Intake() {
    m_inputFlowK = 0;
    m_flow = 0;
    m_throttle = 1.0;
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
}

void Intake::destroy() {
    /* void */
}

void Intake::start() {
    m_system.start();
}

void Intake::process(double dt) {
    m_flow =
        m_system.flow(
                (1 - m_throttle) * m_inputFlowK,
                dt,
                units::pressure(1.0, units::atm),
                units::celcius(25));
}

void Intake::end() {
    m_system.end();
}
