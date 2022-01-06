#ifndef ENGINE_SIM_LINK_CONSTRAINT_H
#define ENGINE_SIM_LINK_CONSTRAINT_H

#include "constraint.h"

#include "rigid_body.h"

class LinkConstraint : public Constraint {
public:
    LinkConstraint();
    virtual ~LinkConstraint();

    void setBody1Position(double x, double y) { m_body1PositionX = x; m_body1PositionY = y; }
    void setBody2Position(double x, double y) { m_body2PositionX = x; m_body2PositionY = y; }

    virtual void calculate(Output *output, int body_i, OdeSolver::System *system);

    RigidBody *m_body1;
    RigidBody *m_body2;

    double m_body1PositionX;
    double m_body1PositionY;

    double m_body2PositionX;
    double m_body2PositionY;

    double m_ks;
    double m_kd;
};

#endif /* ENGINE_SIM_LINK_CONSTRAINT_H */
