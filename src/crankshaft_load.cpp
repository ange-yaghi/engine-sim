#include "../include/crankshaft_load.h"

#include "../include/units.h"

#include <cmath>

CrankshaftLoad::CrankshaftLoad() : atg_scs::Constraint(1, 1) {
    m_crankshaft = nullptr;
    m_bearingConstraint = nullptr;
    m_loadTorque = 0;
    m_enableDyno = false;
    m_dynoSpeed = 0;
    m_minDynoTorque = units::torque(-1000.0, units::ft_lb);
    m_maxDynoTorque = units::torque(1000.0, units::ft_lb);
}

CrankshaftLoad::~CrankshaftLoad() {
    /* void */
}

void CrankshaftLoad::setCrankshaft(Crankshaft *crankshaft) {
    m_crankshaft = crankshaft;
    m_bodies[0] = &m_crankshaft->m_body;
}

void CrankshaftLoad::calculate(Output *output, atg_scs::SystemState *state) {
    output->J[0][0] = 0;
    output->J[0][1] = 0;
    output->J[0][2] = 1;

    output->J_dot[0][0] = 0;
    output->J_dot[0][1] = 0;
    output->J_dot[0][2] = 0;

    output->ks[0] = 0;
    output->kd[0] = 0;

    output->C[0] = 0;

    output->v_bias[0] = (m_enableDyno)
        ? m_dynoSpeed
        : 0;
}

void CrankshaftLoad::limit(atg_scs::Matrix *lambda, atg_scs::SystemState *state) {
    const int index = state->indexMap[m_index];
    const int bcIndex = state->indexMap[m_bearingConstraint->m_index];
    const double normalForce = std::sqrt(
        lambda->get(0, bcIndex) * lambda->get(0, bcIndex)
        + lambda->get(0, bcIndex + 1) * lambda->get(0, bcIndex + 1));

    const double v_theta = units::distance(std::abs(state->v_theta[m_crankshaft->m_body.index]), units::inch);
    const double frictionCoeff = 0.06 + v_theta * 0.01;
    const double frictionForce = frictionCoeff * normalForce;
    const double staticFriction = units::torque(0.0, units::ft_lb);
    double minTorque = frictionForce * units::distance(1.0, units::inch);
    double maxTorque = frictionForce * units::distance(1.0, units::inch);

    if (m_enableDyno) {
        minTorque = m_minDynoTorque;
        maxTorque = m_maxDynoTorque;
    }

    const double calculatedTorque = lambda->get(0, index);
    lambda->set(0, index, std::fmin(maxTorque, std::fmax(minTorque, calculatedTorque)));
}
