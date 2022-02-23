#include "../include/crankshaft_friction.h"

#include "../include/units.h"

#include <cmath>

CrankshaftFriction::CrankshaftFriction() {
    m_friction = 0;
    m_damping = 0;
    m_crankshaft = nullptr;
    m_constraint = nullptr;
}

CrankshaftFriction::~CrankshaftFriction() {
    /* void */
}

void CrankshaftFriction::apply(atg_scs::SystemState *system) {
    const double normalForce = std::sqrt(
        m_constraint->F_x[0][0] * m_constraint->F_x[0][0] / 25.0 +
        m_constraint->F_y[1][0] * m_constraint->F_y[1][0] / 25.0);
    const double F_fric = (m_crankshaft->m_body.v_theta > 0)
        ? -m_friction * normalForce
        : m_friction * normalForce;

    system->t[m_crankshaft->m_body.index] += F_fric * 25.0;

    if (system->v_theta[m_crankshaft->m_body.index] > -units::rpm(200)) {
        system->t[m_crankshaft->m_body.index] -= units::torque(200, units::ft_lb);
    }
}
