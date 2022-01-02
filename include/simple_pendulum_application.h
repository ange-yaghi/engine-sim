#ifndef ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H
#define ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H

#include "engine_sim_application.h"

#include "ode_solver.h"

class SimplePendulumApplication : public EngineSimApplication {
public:
    struct PhysicsState {
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

    void updatePhysics(OdeSolver::System *in, OdeSolver::System *out, double dt);

    ysTexture *m_texture;

    float m_t;

    PhysicsState m_state;
    OdeSolver::System m_system;
    dbasic::TextRenderer m_textRenderer;

    OdeSolver *m_solver;
};

#endif /* ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H */
