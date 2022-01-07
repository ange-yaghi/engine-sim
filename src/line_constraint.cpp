#include "../include/line_constraint.h"

#include <cmath>

LineConstraint::LineConstraint() : Constraint(1) {
    m_body = nullptr;
    m_p0X = m_p0Y = 0.0;
    m_dX = m_dY = 0.0;
    m_bodyX = m_bodyY = 0.0;
    m_ks = 500.0;
    m_kd = 20.0;
}

LineConstraint::~LineConstraint() {
    /* void */
}

void LineConstraint::updateBodies() {
    m_bodies[0] = m_body->m_index;
    m_bodyCount = 1;
}

void LineConstraint::calculate(
        Output *output,
        int body_i,
        OdeSolver::System *system)
{
    if (body_i != m_body->m_index) {
        output->n = 0;
        return;
    }

    output->n = 1;

    const double q1 = system->Position_X[body_i];
    const double q2 = system->Position_Y[body_i];
    const double q3 = system->Angles[body_i];

    const double cos_q3 = std::cos(q3);
    const double sin_q3 = std::sin(q3);

    const double bodyX = q1 + cos_q3 * m_bodyX - sin_q3 * m_bodyY;
    const double bodyY = q2 + sin_q3 * m_bodyX + cos_q3 * m_bodyY;

    const double perpX = -m_dY;
    const double perpY = m_dX;

    const double deltaX = bodyX - m_p0X;
    const double deltaY = bodyY - m_p0Y;

    const double C = deltaX * perpX + deltaY * perpY;

    const double dC_dq1 = 1.0 * perpX;
    const double dC_dq2 = 1.0 * perpY;
    const double dC_dq3 =
        (-sin_q3 * m_bodyX - cos_q3 * m_bodyY) * perpX +
        (cos_q3 * m_bodyX - sin_q3 * m_bodyY) * perpY;

    const double d2C_dq1_2 = 0.0;
    const double d2C_dq2_2 = 0.0;
    const double d2C_dq3_2 =
        (-cos_q3 * m_bodyX + sin_q3 * m_bodyY) * perpX +
        (sin_q3 * m_bodyX - cos_q3 * m_bodyY) * perpY;

    output->dC_dq[0][0] = dC_dq1;
    output->dC_dq[0][1] = dC_dq2;
    output->dC_dq[0][2] = dC_dq3;

    output->d2C_dq2[0][0] = d2C_dq1_2;
    output->d2C_dq2[0][1] = d2C_dq2_2;
    output->d2C_dq2[0][2] = d2C_dq3_2;

    output->ks[0] = m_ks * C;
    output->kd[0] = m_kd;
}
