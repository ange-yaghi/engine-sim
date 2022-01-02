#ifndef ENGINE_SIM_RK4_SOLVER_H
#define ENGINE_SIM_RK4_SOLVER_H

#include "ode_solver.h"

class Rk4Solver : public OdeSolver {
public:
    enum class RkStage {
        Stage_1,
        Stage_2,
        Stage_3,
        Stage_4,
        Undefined
    };

public:
    Rk4Solver();
    virtual ~Rk4Solver();

    virtual void start(System *initial, double dt);
    virtual bool step(System *system);
    virtual void solve(System *in, System *out);
    virtual void end();

protected:
    RkStage m_stage;
    RkStage m_nextStage;

    System m_initial;
    System m_k1;
    System m_k2;
    System m_k3;
    System m_k4;
};

#endif /* ENGINE_SIM_RK4_SOLVER_H */
