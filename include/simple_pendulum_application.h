#ifndef ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H
#define ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H

#include "engine_sim_application.h"

class SimplePendulumApplication : public EngineSimApplication {
public:
    SimplePendulumApplication();
    virtual ~SimplePendulumApplication();

protected:
    virtual void initialize();
    virtual void process(float dt);
    virtual void render();

    ysTexture *m_texture;

    float m_t;
};

#endif /* ENGINE_SIM_SIMPLE_PENDULUM_APPLICATION_H */
