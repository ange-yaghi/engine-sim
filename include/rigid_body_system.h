#ifndef ENGINE_SIM_RIGID_BODY_SYSTEM_H
#define ENGINE_SIM_RIGID_BODY_SYSTEM_H

#include "rigid_body.h"

#include "constraint.h"
#include "matrix.h"

#include <vector>

class RigidBodySystem {
public:
    RigidBodySystem();
    ~RigidBodySystem();

    void addRigidBody(RigidBody *body);
    void removeRigidBody(RigidBody *body);

    void addConstraint(Constraint *constraint);
    void removeConstraint(Constraint *constraint);

    void process(double dt);
    void processConstraints(
            OdeSolver::System *in,
            OdeSolver::System *out,
            Matrix *M,
            double dt);

    int getRigidBodyCount() const { return (int)m_rigidBodies.size(); }
    int getConstraintCount() const { return (int)m_constraints.size(); }

    int getFullConstraintCount() const;

protected:
    std::vector<RigidBody *> m_rigidBodies;
    std::vector<Constraint *> m_constraints;
};

#endif /* ENGINE_SIM_RIGID_BODY_SYSTEM_H */
