#ifndef ENGINE_SIM_STATIC_FORCE_GENERATOR_H
#define ENGINE_SIM_STATIC_FORCE_GENERATOR_H

#include "force_generator.h"

#include "rigid_body.h"

class StaticForceGenerator : public ForceGenerator {
public:
    StaticForceGenerator();
    virtual ~StaticForceGenerator();

    virtual void apply(OdeSolver::System *system);

    void setForce(double f_x, double f_y);
    void setPosition(double p_x, double p_y);

    double m_forceX;
    double m_forceY;

    double m_positionX;
    double m_positionY;

    RigidBody *m_body;
};

#endif /* ENGINE_SIM_STATIC_FORCE_GENERATOR_H */
