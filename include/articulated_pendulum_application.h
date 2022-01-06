#ifndef ENGINE_SIM_ARTICULATED_PENDULUM_APPLICATION_H
#define ENGINE_SIM_ARTICULATED_PENDULUM_APPLICATION_H

#include "engine_sim_application.h"

#include "rigid_body_system.h"
#include "static_force_generator.h"
#include "fixed_position_constraint.h"
#include "link_constraint.h"

class ArticulatedPendulumApplication : public EngineSimApplication {
public:
    ArticulatedPendulumApplication();
    virtual ~ArticulatedPendulumApplication();

protected:
    virtual void initialize();
    virtual void process(float dt);
    virtual void render();
    
    void drawPendulum(
        double hinge_x,
        double hinge_y,
        double x,
        double y,
        double angle,
        double r);

    dbasic::TextRenderer m_textRenderer;

    RigidBodySystem m_rigidBodySystem;
    RigidBody m_pendulum0;
    RigidBody m_pendulum1;

    StaticForceGenerator m_fg0;
    StaticForceGenerator m_fg1;

    FixedPositionConstraint m_c0;
    LinkConstraint m_c1;

    double m_r0;
    double m_r1;
};

#endif /* ENGINE_SIM_ARTICULATED_PENDULUM_APPLICATION_H */
