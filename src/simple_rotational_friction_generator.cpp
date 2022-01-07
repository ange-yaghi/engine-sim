#include "../include/simple_rotational_friction_generator.h"

#include <cmath>

SimpleRotationalFrictionGenerator::SimpleRotationalFrictionGenerator() {
    m_body0 = nullptr;
    m_body1 = nullptr;
    m_constraint = nullptr;
    m_frictionCoefficient = 1.0;
}

SimpleRotationalFrictionGenerator::~SimpleRotationalFrictionGenerator() {
    /* void */
}

void SimpleRotationalFrictionGenerator::apply(OdeSolver::System *system) {
    const int n = m_constraint->m_constraintCount;

    double forceX = 0.0, forceY = 0.0;
    for (int i = 0; i < n; ++i) {
         forceX += m_constraint->m_forceX[i][0];
         forceY += m_constraint->m_forceY[i][0];
    }

    const double totalForce = std::sqrt(forceX * forceX + forceY * forceY);
    RigidBody *bodies[2] = { m_body0, m_body1 };

    const double relativeAngularVelocity = (m_body1 == nullptr)
        ? system->AngularVelocity[m_body0->m_index]
        : system->AngularVelocity[m_body0->m_index] - system->AngularVelocity[m_body1->m_index];

    const double globalSign = relativeAngularVelocity > 0
        ? -1.0
        : 1.0;
    
    for (int i = 0; i < 2; ++i) {
        if (bodies[i] == nullptr) continue;
        
        const double sign = (i == 0)
            ? globalSign
            : -globalSign;

        const double frictionTorque = sign * totalForce * m_frictionCoefficient;

        system->Torque[bodies[i]->m_index] += frictionTorque;
    }
}
