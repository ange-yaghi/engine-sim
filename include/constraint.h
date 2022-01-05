#ifndef ENGINE_SIM_CONSTRAINT_H
#define ENGINE_SIM_CONSTRAINT_H

#include "ode_solver.h"

class Constraint {
public:
    struct Output {
        double dC_dq[8][3];
        double d2C_dq2[8][3];
        double ks[8];
        double kd[8];
        int n;
    };

public:
    Constraint(int constraintCount);
    virtual ~Constraint();

    virtual void calculate(Output *output, int body_i, OdeSolver::System *system);

    int m_index;
    int m_constraintCount;
};

#endif /* ENGINE_SIM_CONSTRAINT_H */
