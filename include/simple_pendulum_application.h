#ifndef ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H
#define ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H

#include "engine_sim_application.h"

class SimplePendulumApplication : public EngineSimApplication {
public:
    struct PhysicsState {
        double Angle;
        double AngularVelocity;

        double Position_X;
        double Position_Y;
        double Velocity_X;
        double Velocity_Y;

        double Force_X;
        double Force_Y;
        double Torque;

        double ConstrainedPosition_X;
        double ConstrainedPosition_Y;

        double Radius;
    };

public:
    SimplePendulumApplication();
    virtual ~SimplePendulumApplication();

protected:
    virtual void initialize();
    virtual void process(float dt);
    virtual void render();

    void updatePhysics(double dt);

    ysTexture *m_texture;

    float m_t;

    PhysicsState m_state;
    dbasic::TextRenderer m_textRenderer;
};

#endif /* ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H */
