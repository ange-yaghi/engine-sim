#ifndef ENGINE_SIM_SIMPLE_ROTATIONAL_FRICTION_GENERATOR_H
#define ENGINE_SIM_SIMPLE_ROTATIONAL_FRICTION_GENERATOR_H

#include "force_generator.h"

#include "rigid_body.h"

class SimpleRotationalFrictionGenerator : public ForceGenerator {
public:
    SimpleRotationalFrictionGenerator();
    virtual ~SimpleRotationalFrictionGenerator();

    virtual void apply(OdeSolver::System *system);

    RigidBody *m_body;
    double m_frictionCoefficient;
};

#endif /* ENGINE_SIM_SIMPLE_ROTATIONAL_FRICTION_GENERATOR_H */
