#ifndef ENGINE_SIM_RIGID_BODY_SYSTEM_H
#define ENGINE_SIM_RIGID_BODY_SYSTEM_H

#include "rigid_body.h"
#include "constraint.h"
#include "force_generator.h"
#include "matrix.h"
#include "sle_solver.h"

#include <vector>

class RigidBodySystem {
public:
    RigidBodySystem();
    ~RigidBodySystem();

    void initialize();

    void addRigidBody(RigidBody *body);
    void removeRigidBody(RigidBody *body);

    void addConstraint(Constraint *constraint);
    void removeConstraint(Constraint *constraint);

    void addForceGenerator(ForceGenerator *generator);
    void removeForceGenerator(ForceGenerator *generator);

    void process(double dt);
    void processForces(
            OdeSolver::System *inOut);
    void processConstraints(
            OdeSolver::System *in,
            OdeSolver::System *out,
            Matrix *lambda,
            Matrix *M,
            Matrix *M_inv);

    int getRigidBodyCount() const { return (int)m_rigidBodies.size(); }
    int getConstraintCount() const { return (int)m_constraints.size(); }
    int getForceGeneratorCount() const { return (int)m_forceGenerators.size(); }

    int getFullConstraintCount() const;

protected:
    std::vector<RigidBody *> m_rigidBodies;
    std::vector<Constraint *> m_constraints;
    std::vector<ForceGenerator *> m_forceGenerators;

    SleSolver *m_sleSolver;
    OdeSolver *m_odeSolver;
};

#endif /* ENGINE_SIM_RIGID_BODY_SYSTEM_H */
