#include "../include/rigid_body_system.h"

#include "../include/matrix.h"

RigidBodySystem::RigidBodySystem() {
    /* void */
}

RigidBodySystem::~RigidBodySystem() {
    /* void */
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

void RigidBodySystem::process(double dt) {
    const int n = getRigidBodyCount();
    const int m = getFullConstraintCount();

    OdeSolver::System system;
    OdeSolver::initializeSystem(&system, n, dt);
    for (int i = 0; i < n; ++i) {
        system.Velocity_X[i] = m_rigidBodies[i]->m_velocityX;
        system.Velocity_Y[i] = m_rigidBodies[i]->m_velocityY;
        
        system.Position_X[i] = m_rigidBodies[i]->m_positionX;
        system.Position_Y[i] = m_rigidBodies[i]->m_positionY;

        system.AngularVelocity[i] = m_rigidBodies[i]->m_angularVelocity;
        system.Angles[i] = m_rigidBodies[i]->m_orientation;
    }

    Matrix M_inv(3 * n, 3 * n, 0.0);
    Matrix M(3 * n, 3 * n, 0.0);
    for (int i = 0; i < n; ++i) {
        M.set(i * 3 + 0, i * 3 + 0, m_rigidBodies[i]->m_mass);
        M.set(i * 3 + 1, i * 3 + 1, m_rigidBodies[i]->m_mass);
        M.set(i * 3 + 2, i * 3 + 2, m_rigidBodies[i]->m_inertiaTensor);

        M.set(i * 3 + 0, i * 3 + 0, 1 / m_rigidBodies[i]->m_mass);
        M.set(i * 3 + 1, i * 3 + 1, 1 / m_rigidBodies[i]->m_mass);
        M.set(i * 3 + 2, i * 3 + 2, 1 / m_rigidBodies[i]->m_inertiaTensor);
    }
}

void RigidBodySystem::processConstraints(
        OdeSolver::System *in,
        OdeSolver::System *out,
        Matrix *M,
        double dt)
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

    J.multiply(q_dot, &C_kd);

    Constraint::Output constraintOutput;
    for (int i = 0; i < n; ++i) {
        for (int j = 0, c_i = 0; j < m; ++j) {
            m_constraints[j]->calculate(&constraintOutput, i, in);
            
            for (int k = 0; k < constraintOutput.n; ++k, ++c_i) {
                J.set(i * 3 + 0, c_i, constraintOutput.dC_dq[k][0]);
                J.set(i * 3 + 1, c_i, constraintOutput.dC_dq[k][1]);
                J.set(i * 3 + 2, c_i, constraintOutput.dC_dq[k][2]);

                J_dot.set(i * 3 + 0, c_i,
                    constraintOutput.d2C_dq2[k][0] * in->Velocity_X[i]);
                J_dot.set(i * 3 + 1, c_i,
                    constraintOutput.d2C_dq2[k][1] * in->Velocity_Y[i]);
                J_dot.set(i * 3 + 2, c_i,
                    constraintOutput.d2C_dq2[k][2] * in->AngularVelocity[i]);

                C_ks.set(0, c_i, constraintOutput.ks[k]);
                
            }
        }
    }

    Matrix J_T(m_f, 3 * n, 0.0);
    J.transpose(&J_T);
}

int RigidBodySystem::getFullConstraintCount() const {
    int count = 0;
    for (Constraint *constraint: m_constraints) {
        count += constraint->m_constraintCount;
    }

    return count;
}
