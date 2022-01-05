#ifndef ENGINE_SIM_FIXED_POSITION_CONSTRAINT_H
#define ENGINE_SIM_FIXED_POSITION_CONSTRAINT_H

#include "constraint.h"

class FixedPositionConstraint : Constraint {
public:
    FixedPositionConstraint();
    virtual ~FixedPositionConstraint();

    void setBody(int body) { m_body = body; }
    int getBody() const { return m_body; }

    void setFixedPosition(double x, double y) { m_fixedPositionX = x; m_fixedPositionY = y; }
    double getFixedPositionX() const { return m_fixedPositionX; }
    double getFixedPositionY() const { return m_fixedPositionY; }

    void setBodyPosition(double x, double y) { m_bodyX = x; m_bodyY = y; }
    double getBodyX() const { return m_bodyX; }
    double getBodyY() const { return m_bodyY; }

    virtual void calculate(Output *output, int body_i, OdeSolver::System *system);

protected:
    int m_body;
    double m_bodyX;
    double m_bodyY;
    double m_fixedPositionX;
    double m_fixedPositionY;
};

#endif /* ENGINE_SIM_FIXED_POSITION_CONSTRAINT_H */
