#ifndef ENGINE_SIM_LINE_CONSTRAINT_H
#define ENGINE_SIM_LINE_CONSTRAINT_H

#include "constraint.h"

#include "rigid_body.h"

class LineConstraint : public Constraint {
public:
    LineConstraint();
    virtual ~LineConstraint();

    virtual void updateBodies();
    virtual void calculate(Output *output, int body_i, OdeSolver::System *system);

    RigidBody *m_body;
    double m_bodyX;
    double m_bodyY;
    double m_p0X;
    double m_p0Y;
    double m_dX;
    double m_dY;

    double m_ks;
    double m_kd;
};

#endif /* ENGINE_SIM_LINE_CONSTRAINT_H */
