#ifndef ENGINE_SIM_FIXED_POSITION_CONSTRAINT_H
#define ENGINE_SIM_FIXED_POSITION_CONSTRAINT_H

#include "constraint.h"

#include "rigid_body.h"

class FixedPositionConstraint : public Constraint {
public:
    FixedPositionConstraint();
    virtual ~FixedPositionConstraint();

    void setFixedPosition(double x, double y) { m_fixedPositionX = x; m_fixedPositionY = y; }
    void setBodyPosition(double x, double y) { m_bodyX = x; m_bodyY = y; }

    virtual void calculate(Output *output, int body_i, OdeSolver::System *system);

    RigidBody *m_body;
    double m_bodyX;
    double m_bodyY;
    double m_fixedPositionX;
    double m_fixedPositionY;
    double m_ks;
    double m_kd;
};

#endif /* ENGINE_SIM_FIXED_POSITION_CONSTRAINT_H */
