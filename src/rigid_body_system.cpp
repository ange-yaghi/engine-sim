#include "../include/rigid_body_system.h"

#include "../include/matrix.h"
#include "../include/gauss_seidel_sle_solver.h"
#include "../include/rk4_solver.h"
#include "../include/euler_solver.h"

RigidBodySystem::RigidBodySystem() {
    m_sleSolver = nullptr;
    m_odeSolver = nullptr;
}

RigidBodySystem::~RigidBodySystem() {
    /* void */
}

void RigidBodySystem::initialize() {
    m_sleSolver = new GaussSeidelSleSolver(); 
    m_odeSolver = new Rk4Solver();
}

void RigidBodySystem::addRigidBody(RigidBody *body) {
    m_rigidBodies.push_back(body);
    body->m_index = (int)m_rigidBodies.size() - 1;
}

void RigidBodySystem::removeRigidBody(RigidBody *body) {
    m_rigidBodies[body->m_index] = m_rigidBodies.back();
    m_rigidBodies[body->m_index]->m_index = body->m_index;
    m_rigidBodies.resize(m_rigidBodies.size() - 1);
}

void RigidBodySystem::addConstraint(Constraint *constraint) {
    m_constraints.push_back(constraint);
    constraint->m_index = (int)m_constraints.size() - 1;
}

void RigidBodySystem::removeConstraint(Constraint *constraint) {
    m_constraints[constraint->m_index] = m_constraints.back();
    m_constraints[constraint->m_index]->m_index = constraint->m_index;
    m_constraints.resize(m_constraints.size() - 1);
}

void RigidBodySystem::addForceGenerator(ForceGenerator *forceGenerator) {
    m_forceGenerators.push_back(forceGenerator);
    forceGenerator->m_index = (int)m_forceGenerators.size() - 1;
}

void RigidBodySystem::removeForceGenerator(ForceGenerator *forceGenerator) {
    m_forceGenerators[forceGenerator->m_index] = m_forceGenerators.back();
    m_forceGenerators[forceGenerator->m_index]->m_index = forceGenerator->m_index;
    m_forceGenerators.resize(m_forceGenerators.size() - 1);
}

void RigidBodySystem::process(double dt) {
    const int n = getRigidBodyCount();
    const int m_f = getFullConstraintCount();
    const int m = getConstraintCount();

    OdeSolver::System system;
    OdeSolver::initializeSystem(&system, n, 0.0);
    for (int i = 0; i < n; ++i) {
        system.Velocity_X[i] = m_rigidBodies[i]->m_velocityX;
        system.Velocity_Y[i] = m_rigidBodies[i]->m_velocityY;
        
        system.Position_X[i] = m_rigidBodies[i]->m_positionX;
        system.Position_Y[i] = m_rigidBodies[i]->m_positionY;

        system.AngularVelocity[i] = m_rigidBodies[i]->m_angularVelocity;
        system.Angles[i] = m_rigidBodies[i]->m_orientation;
    }

    for (int i = 0; i < m; ++i) {
        m_constraints[i]->updateBodies();
    }

    Matrix M_inv(3 * n, 3 * n, 0.0);
    Matrix M(3 * n, 3 * n, 0.0);
    for (int i = 0; i < n; ++i) {
        M.set(i * 3 + 0, i * 3 + 0, m_rigidBodies[i]->m_mass);
        M.set(i * 3 + 1, i * 3 + 1, m_rigidBodies[i]->m_mass);
        M.set(i * 3 + 2, i * 3 + 2, m_rigidBodies[i]->m_inertiaTensor);

        M_inv.set(i * 3 + 0, i * 3 + 0, 1 / m_rigidBodies[i]->m_mass);
        M_inv.set(i * 3 + 1, i * 3 + 1, 1 / m_rigidBodies[i]->m_mass);
        M_inv.set(i * 3 + 2, i * 3 + 2, 1 / m_rigidBodies[i]->m_inertiaTensor);
    }

    Matrix lambda(1, m_f, 0.0);
    Matrix R(m_f, n * 3, 0.0);

    const int steps = 1;
    for (int i = 0; i < steps; ++i) {
        m_odeSolver->start(&system, dt / steps);

        while (true) {
            const bool done = m_odeSolver->step(&system);
            processForces(&system);
            processConstraints(&system, &system, &lambda, &R, &M, &M_inv);

            m_odeSolver->solve(&system, &system);

            if (done) break;
        }

        m_odeSolver->end();
    }

    for (int i = 0; i < n; ++i) {
        m_rigidBodies[i]->m_velocityX = system.Velocity_X[i];
        m_rigidBodies[i]->m_velocityY = system.Velocity_Y[i];
        
        m_rigidBodies[i]->m_positionX = system.Position_X[i];
        m_rigidBodies[i]->m_positionY = system.Position_Y[i];

        m_rigidBodies[i]->m_angularVelocity = system.AngularVelocity[i];
        m_rigidBodies[i]->m_orientation = system.Angles[i];
    }

    for (int i = 0, c_i = 0; i < m; ++i) {
        const int n_sub = m_constraints[i]->m_constraintCount;
        for (int j = 0; j < n_sub; ++j, ++c_i) {
            for (int k = 0; k < m_constraints[i]->m_bodyCount; ++k) {
                m_constraints[i]->m_forceX[k][j] =
                    R.get(c_i, m_constraints[i]->m_bodies[k] * 3 + 0);
                m_constraints[i]->m_forceY[k][j] =
                    R.get(c_i, m_constraints[i]->m_bodies[k] * 3 + 1);
                m_constraints[i]->m_torque[k][j] =
                    R.get(c_i, m_constraints[i]->m_bodies[k] * 3 + 2);
            }
        }
    }

    OdeSolver::destroySystem(&system);
}

void RigidBodySystem::processForces(OdeSolver::System *inOut) {
    const int n_f = getForceGeneratorCount();
    const int n = getRigidBodyCount();

    for (int i = 0; i < n; ++i) {
        inOut->Force_X[i] = 0.0;
        inOut->Force_Y[i] = 0.0;
        inOut->Torque[i] = 0.0;
    }

    for (int i = 0; i < n_f; ++i) {
        m_forceGenerators[i]->apply(inOut);
    }
}

void RigidBodySystem::processConstraints(
        OdeSolver::System *in,
        OdeSolver::System *out,
        Matrix *lambdaPrev,
        Matrix *R,
        Matrix *M,
        Matrix *M_inv)
{
    const int n = getRigidBodyCount();
    const int m_f = getFullConstraintCount();
    const int m = getConstraintCount();

    Matrix q_dot(1, 3 * n, 0.0);
    for (int i = 0; i < n; ++i) {
        q_dot.set(0, i * 3 + 0, in->Velocity_X[i]);
        q_dot.set(0, i * 3 + 1, in->Velocity_Y[i]);
        q_dot.set(0, i * 3 + 2, in->AngularVelocity[i]);
    }

    Matrix J(3 * n, m_f, 0.0);
    Matrix J_dot(3 * n, m_f, 0.0);
    Matrix C_ks(1, m_f, 0.0);
    Matrix C_kd(1, m_f, 0.0);

    Constraint::Output constraintOutput;
    int c_i = 0;
    for (int j = 0;  j < m; ++j) {
        for (int i = 0; i < n; ++i) {
            m_constraints[j]->calculate(&constraintOutput, i, in);
            
            for (int k = 0; k < constraintOutput.n; ++k) {
                J.set(i * 3 + 0, c_i + k, constraintOutput.dC_dq[k][0]);
                J.set(i * 3 + 1, c_i + k, constraintOutput.dC_dq[k][1]);
                J.set(i * 3 + 2, c_i + k, constraintOutput.dC_dq[k][2]);

                J_dot.set(i * 3 + 0, c_i + k,
                    constraintOutput.d2C_dq2[k][0] * in->Velocity_X[i]);
                J_dot.set(i * 3 + 1, c_i + k,
                    constraintOutput.d2C_dq2[k][1] * in->Velocity_Y[i]);
                J_dot.set(i * 3 + 2, c_i + k,
                    constraintOutput.d2C_dq2[k][2] * in->AngularVelocity[i]);

                C_ks.set(0, c_i + k, constraintOutput.ks[k]);
                C_kd.set(0, c_i + k, constraintOutput.kd[k]);
            }
        }

        c_i += m_constraints[j]->m_constraintCount;
    }

    Matrix temp0, temp1, temp2;

    temp0.initialize(1, m_f, 0.0);
    J.multiply(q_dot, &temp0);
    for (int i = 0; i < m_f; ++i) {
        C_kd.set(0, i, C_kd.get(0, i) * temp0.get(0, i));
    }
    temp0.destroy();

    Matrix J_T(m_f, 3 * n, 0.0);
    J.transpose(&J_T);

    Matrix F_ext(1, 3 * n, 0.0);
    for (int i = 0; i < n; ++i) {
        F_ext.set(0, i * 3 + 0, in->Force_X[i]);
        F_ext.set(0, i * 3 + 1, in->Force_Y[i]);
        F_ext.set(0, i * 3 + 2, in->Torque[i]);
    }

    Matrix right, left;

    right.initialize(1, m_f, 0.0);
    left.initialize(m_f, m_f, 0.0);

    temp0.initialize(3 * n, m_f, 0.0);
    J.multiply(*M_inv, &temp0);
    temp0.multiply(J_T, &left);

    temp0.initialize(1, m_f, 0.0);
    temp1.initialize(1, m_f, 0.0);
    J_dot.multiply(q_dot, &temp0);
    temp0.negate(&temp1);

    temp0.initialize(1, m_f, 0.0);
    temp2.initialize(3 * n, m_f, 0.0);
    J.multiply(*M_inv, &temp2);
    temp2.multiply(F_ext, &temp0);

    temp2.initialize(1, m_f, 0.0);
    temp1.subtract(temp0, &temp2);
    temp2.subtract(C_ks, &temp1);
    temp1.subtract(C_kd, &right);
    
    Matrix lambda(1, m_f, 0.0);
    const bool solvable = m_sleSolver->solve(&left, &right, &lambda, lambdaPrev);
    assert(solvable);

    Matrix F_C(1, n * 3, 0.0);
    J_T.multiply(lambda, &F_C);

    Matrix lambdaScale(m_f, m_f, 0.0);
    for (int i = 0; i < m_f; ++i) {
        lambdaScale.set(i, i, lambda.get(0, i));
    }

    J_T.multiply(lambdaScale, R);

    for (int i = 0; i < n; ++i) {
        const double invMass = M_inv->get(i * 3 + 0, i * 3 + 0);
        const double invInertia = M_inv->get(i * 3 + 2, i * 3 + 2);

        out->Acceleration_X[i] =
            invMass * (F_C.get(0, i * 3 + 0) + F_ext.get(0, i * 3 + 0));
        out->Acceleration_Y[i] =
            invMass * (F_C.get(0, i * 3 + 1) + F_ext.get(0, i * 3 + 1)); 
        out->AngularAcceleration[i] =
            invInertia * (F_C.get(0, i * 3 + 2) + F_ext.get(0, i * 3 + 2));

        if (std::abs(out->AngularAcceleration[i]) > 1E-6) {
            int a = 0;
        }

        out->Angles[i] = in->Angles[i];
        out->AngularVelocity[i] = in->AngularVelocity[i];

        out->Position_X[i] = in->Position_X[i];
        out->Position_Y[i] = in->Position_Y[i];
        out->Velocity_X[i] = in->Velocity_X[i];
        out->Velocity_Y[i] = in->Velocity_Y[i];
    }
}

int RigidBodySystem::getFullConstraintCount() const {
    int count = 0;
    for (Constraint *constraint: m_constraints) {
        count += constraint->m_constraintCount;
    }

    return count;
}
