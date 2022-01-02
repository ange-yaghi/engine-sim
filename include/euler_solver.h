#ifndef ENGINE_SIM_EULER_SOLVER_H
#define ENGINE_SIM_EULER_SOLVER_H

#include "ode_solver.h"

class EulerSolver : public OdeSolver {
public:
    EulerSolver();
    virtual ~EulerSolver();

    virtual void start(System *initial, double dt);
    virtual bool step(System *system);
    virtual void solve(System *in, System *out);
    virtual void end();

protected:
    System m_initial;
};

#endif /* ENGINE_SIM_EULER_SOLVER_H */
