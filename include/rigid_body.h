#ifndef ENGINE_SIM_RIGID_BODY_H
#define ENGINE_SIM_RIGID_BODY_H

class RigidBody {
public:
    RigidBody();
    ~RigidBody();

    double m_positionX;
    double m_positionY;

    double m_velocityX;
    double m_velocityY;

    double m_orientation;
    double m_angularVelocity;

    int m_index;

    double m_mass;
    double m_inertiaTensor;
};

#endif /* ENGINE_SIM_RIGID_BODY_H */
