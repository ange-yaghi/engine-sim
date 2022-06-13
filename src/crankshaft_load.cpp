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

    double minTorque = 0;
    double maxTorque = 0;

    if (m_enableDyno) {
        minTorque = m_minDynoTorque;
        maxTorque = m_maxDynoTorque;
    }

    output->limits[0][0] = minTorque;
    output->limits[0][1] = maxTorque;
    output->v_bias[0] = (m_enableDyno)
        ? m_dynoSpeed
        : 0;
}

void CrankshaftLoad::limit(atg_scs::Matrix *lambda, atg_scs::SystemState *state) {
    /*
    const int index = state->indexMap[m_index];
    const int bcIndex = state->indexMap[m_bearingConstraint->m_index];
    const double normalForce = std::sqrt(
        lambda->get(0, bcIndex) * lambda->get(0, bcIndex)
        + lambda->get(0, bcIndex + 1) * lambda->get(0, bcIndex + 1));

    const double frictionTorque = calculateFrictionTorque(normalForce, state->v_theta[m_crankshaft->m_body.index]);
    double minTorque = -frictionTorque;
    double maxTorque = frictionTorque;
    */

    const int index = state->indexMap[m_index];
    double minTorque = 0;
    double maxTorque = 0;

    if (m_enableDyno) {
        minTorque = m_minDynoTorque;
        maxTorque = m_maxDynoTorque;
    }

    const double calculatedTorque = lambda->get(0, index);
    lambda->set(0, index, std::fmin(maxTorque, std::fmax(minTorque, calculatedTorque)));
}

double CrankshaftLoad::getDynoTorque() const {
    if (!m_enableDyno) return 0;

    const double normalForce = std::sqrt(
        m_bearingConstraint->F_x[0][0] * m_bearingConstraint->F_x[0][0]
        + m_bearingConstraint->F_y[1][0] * m_bearingConstraint->F_y[1][0]);

    const double maxFrictionTorque = calculateFrictionTorque(normalForce, m_crankshaft->m_body.v_theta);
    const double frictionTorque = (m_crankshaft->m_body.v_theta > 0)
        ? -maxFrictionTorque
        : maxFrictionTorque;

    const double rawDynoTorque = F_t[0][0];

    if (rawDynoTorque > 0) {
        return (frictionTorque < 0)
            ? std::fmax(rawDynoTorque + frictionTorque, 0)
            : std::fmax(rawDynoTorque - frictionTorque, 0);
    }
    else {
        return (frictionTorque < 0)
            ? std::fmin(rawDynoTorque + frictionTorque, 0)
            : std::fmin(rawDynoTorque - frictionTorque, 0);
    }
}

double CrankshaftLoad::getAppliedFrictionTorque() const {
    return std::abs(F_t[0][0] - getDynoTorque());
}

double CrankshaftLoad::calculateFrictionTorque(double normalForce, double v_theta) const {
    const double v_planar =
        units::distance(1.0, units::inch) * std::abs(v_theta);
    /*
    const double frictionCoeff = 0.12;
    const double frictionForce = frictionCoeff * normalForce;
    return frictionForce * units::distance(1.0, units::inch);*/

    const double F_coul = 0.06 * normalForce;
    const double v_st = units::distance(0.1, units::m) * constants::root_2;
    const double v_coul = units::distance(0.1, units::m) / 10;
    const double F_brk = units::torque(1, units::Nm) * (5 + 8) / units::distance(1.0, units::inch);
    const double v = std::abs(v_planar);

    const double F_0 = constants::root_2 * constants::e * (F_brk - F_coul);
    const double F_1 = v / v_st;
    const double F_2 = std::exp(-F_1 * F_1) * F_1;
    const double F_3 = F_coul * std::tanh(v / v_coul);
    const double F_4 = units::force(20, units::N) * v * (5 + 8);

    constexpr double constant_torque = units::force(10, units::lbf) * units::distance(1.0, units::foot);
    constexpr double view = units::convert(constant_torque, units::ft_lb);

    return (F_0 * F_2 + F_3 + F_4) * units::distance(1.0, units::inch) + constant_torque;
}
