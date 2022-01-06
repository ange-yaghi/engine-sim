#include "../include/link_constraint.h"

#include <cmath>

LinkConstraint::LinkConstraint() : Constraint(2) {
    m_body1 = m_body2 = nullptr;

    m_body1PositionX = m_body1PositionY = 0;
    m_body2PositionX = m_body2PositionY = 0;

    m_ks = 500.0;
    m_kd = 20.0;
}

LinkConstraint::~LinkConstraint() {
    /* void */
}

void LinkConstraint::calculate(Output *output, int body_i, OdeSolver::System *system) {
    if (body_i != m_body1->m_index && body_i != m_body2->m_index) {
        output->n = 0;
        return;
    }

    double localX, localX_p, localY, localY_p;
    int linkedBody_i;

    if (body_i == m_body1->m_index) {
        localX = m_body1PositionX;
        localY = m_body1PositionY;
        localX_p = m_body2PositionX;
        localY_p = m_body2PositionY;
        linkedBody_i = m_body2->m_index;
    }
    else {
        localX = m_body2PositionX;
        localY = m_body2PositionY;
        localX_p = m_body1PositionX;
        localY_p = m_body2PositionY;
        linkedBody_i = m_body1->m_index;
    }

    output->n = 2;

    const double q1 = system->Position_X[body_i];
    const double q2 = system->Position_Y[body_i];
    const double q3 = system->Angles[body_i];

    const double q1_p = system->Position_X[linkedBody_i];
    const double q2_p = system->Position_Y[linkedBody_i];
    const double q3_p = system->Angles[linkedBody_i]; 

    const double cos_q3 = std::cos(q3);
    const double sin_q3 = std::sin(q3);

    const double cos_q3_p = std::cos(q3_p);
    const double sin_q3_p = std::sin(q3_p);

    const double bodyX = q1 + cos_q3 * localX - sin_q3 * localY;
    const double bodyY = q2 + sin_q3 * localX + cos_q3 * localY;

    const double linkedBodyX = q1_p + cos_q3_p * localX_p - sin_q3_p * localY_p;
    const double linkedBodyY = q2_p + sin_q3_p * localX_p + cos_q3_p * localY_p;

    const double dbodyX_dq1 = 1.0;
    const double dbodyX_dq2 = 0.0;
    const double dbodyX_dq3 = -sin_q3 * localX - cos_q3 * localY;

    const double dbodyY_dq1 = 0.0;
    const double dbodyY_dq2 = 1.0;
    const double dbodyY_dq3 = cos_q3 * localX - sin_q3 * localY;

    const double d2bodyX_dq1_2 = 0.0;
    const double d2bodyX_dq2_2 = 0.0;
    const double d2bodyX_dq3_2 = -cos_q3 * localX + sin_q3 * localY;

    const double d2bodyY_dq1_2 = 0.0;
    const double d2bodyY_dq2_2 = 0.0;
    const double d2bodyY_dq3_2 = -sin_q3 * localX - cos_q3 * localY;

    const double C1 = bodyX - linkedBodyX;
    const double C2 = bodyY - linkedBodyY;

    if (body_i == m_body1->m_index) {
        output->dC_dq[0][0] = dbodyX_dq1;
        output->dC_dq[0][1] = dbodyX_dq2;
        output->dC_dq[0][2] = dbodyX_dq3;

        output->dC_dq[1][0] = dbodyY_dq1;
        output->dC_dq[1][1] = dbodyY_dq2;
        output->dC_dq[1][2] = dbodyY_dq3;

        output->d2C_dq2[0][0] = d2bodyX_dq1_2;
        output->d2C_dq2[0][1] = d2bodyX_dq2_2;
        output->d2C_dq2[0][2] = d2bodyX_dq3_2;

        output->d2C_dq2[1][0] = d2bodyY_dq1_2;
        output->d2C_dq2[1][1] = d2bodyY_dq2_2;
        output->d2C_dq2[1][2] = d2bodyY_dq3_2;

        output->ks[0] = m_ks * C1;
        output->ks[1] = m_ks * C2; 
    }
    else {
        output->dC_dq[0][0] = -dbodyX_dq1;
        output->dC_dq[0][1] = -dbodyX_dq2;
        output->dC_dq[0][2] = -dbodyX_dq3;

        output->dC_dq[1][0] = -dbodyY_dq1;
        output->dC_dq[1][1] = -dbodyY_dq2;
        output->dC_dq[1][2] = -dbodyY_dq3;

        output->d2C_dq2[0][0] = -d2bodyX_dq1_2;
        output->d2C_dq2[0][1] = -d2bodyX_dq2_2;
        output->d2C_dq2[0][2] = -d2bodyX_dq3_2;

        output->d2C_dq2[1][0] = -d2bodyY_dq1_2;
        output->d2C_dq2[1][1] = -d2bodyY_dq2_2;
        output->d2C_dq2[1][2] = -d2bodyY_dq3_2;

        output->ks[0] = m_ks * -C1;
        output->ks[1] = m_ks * -C2;
    }

    output->kd[0] = output->kd[1] = m_kd;
}
