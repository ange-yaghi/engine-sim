#include "../include/crankshaft_friction.h"

#include "../include/units.h"

#include <cmath>

CrankshaftFriction::CrankshaftFriction() {
    m_friction = 0;
    m_damping = 0;
    m_crankshaft = nullptr;
}

CrankshaftFriction::~CrankshaftFriction() {
    /* void */
}

void CrankshaftFriction::apply(atg_scs::SystemState *system) {
    const double F_fric = (m_crankshaft->m_body.v_theta > 0)
        ? -m_friction
        : m_friction;

    system->t[m_crankshaft->m_body.index] +=
        F_fric + m_damping * m_crankshaft->m_body.v_theta;

    if (system->v_theta[m_crankshaft->m_body.index] > -units::rpm(10)) {
        system->t[m_crankshaft->m_body.index] -= 140000 / 2.0f;
    }
}
