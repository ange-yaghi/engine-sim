#include "../include/dynamometer.h"

#include "../include/units.h"

#include <cmath>

Dynamometer::Dynamometer() : atg_scs::Constraint(1, 1) {
    m_rotationSpeed = 0.0;
    m_ks = 10.0;
    m_kd = 1.0;
    m_maxTorque = units::torque(1000.0, units::ft_lb);
    m_enabled = false;
}

Dynamometer::~Dynamometer() {
    /* void */
}

void Dynamometer::connectCrankshaft(Crankshaft *crankshaft) {
    m_bodies[0] = &crankshaft->m_body;
}

void Dynamometer::calculate(Output *output, atg_scs::SystemState *state) {
    output->J[0][0] = 0;
    output->J[0][1] = 0;
    output->J[0][2] = 1;

    output->J_dot[0][0] = 0;
    output->J_dot[0][1] = 0;
    output->J_dot[0][2] = 0;

    output->ks[0] = m_ks;
    output->kd[0] = m_kd;

    output->C[0] = 0;

    output->v_bias[0] = m_rotationSpeed;

    output->limits[0][0] = 0.0;
    output->limits[0][1] = m_enabled ? m_maxTorque : 0.0;
}

double Dynamometer::getTorque() const {
    return atg_scs::Constraint::F_t[0][0];
}
