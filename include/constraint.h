#ifndef ENGINE_SIM_CONSTRAINT_H
#define ENGINE_SIM_CONSTRAINT_H

#include "ode_solver.h"

class Constraint {
public:
    static constexpr int MaxConstraintCount = 8;
    static constexpr int MaxBodyCount = 8;

    struct Output {
        double dC_dq[MaxConstraintCount][3];
        double d2C_dq2[MaxConstraintCount][3];
        double ks[MaxConstraintCount];
        double kd[MaxConstraintCount];
        int n;
    };

public:
    Constraint(int constraintCount);
    virtual ~Constraint();

    virtual void updateBodies();
    virtual void calculate(Output *output, int body_i, OdeSolver::System *system);

    int m_index;
    int m_constraintCount;

    int m_bodyCount;
    int m_bodies[MaxBodyCount];

    double m_forceX[MaxBodyCount][MaxConstraintCount];
    double m_forceY[MaxBodyCount][MaxConstraintCount];
    double m_torque[MaxBodyCount][MaxConstraintCount];
};

#endif /* ENGINE_SIM_CONSTRAINT_H */
