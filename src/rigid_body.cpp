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
