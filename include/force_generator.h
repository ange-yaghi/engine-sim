#ifndef ENGINE_SIM_FORCE_GENERATOR_H
#define ENGINE_SIM_FORCE_GENERATOR_H

#include "ode_solver.h"

class ForceGenerator {
public:
    ForceGenerator();
    virtual ~ForceGenerator();

    virtual void apply(OdeSolver::System *system) = 0;

    int m_index;
};

#endif /* ENGINE_SIM_FORCE_GENERATOR_H */
