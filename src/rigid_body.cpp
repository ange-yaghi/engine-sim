#include "../include/rigid_body.h"

RigidBody::RigidBody() {
    m_positionX = m_positionY = 0.0;
    m_velocityX = m_velocityY = 0.0;
    
    m_orientation = 0.0;
    m_angularVelocity = 0.0;

    m_index = 0;

    m_mass = 0.0;
    m_inertiaTensor = 0.0;
}

RigidBody::~RigidBody() {
    /* void */
}

double RigidBody::energy() const {
    const double speed_2 = m_velocityX * m_velocityX + m_velocityY * m_velocityY;
    const double E_k = 0.5 * m_mass * speed_2;
    const double E_r = 0.5 * m_inertiaTensor * m_angularVelocity * m_angularVelocity;

    return E_k + E_r;
}
