#include "../include/euler_solver.h"

EulerSolver::EulerSolver() {
    /* void */
}

EulerSolver::~EulerSolver() {
    /* void */
}

void EulerSolver::start(System *initial, double dt) {
    OdeSolver::start(initial, dt);

    copySystem(initial, &m_initial);
}

bool EulerSolver::step(System *system) {
    copySystem(&m_initial, system);

    system->dt = m_dt;
    return true;
}

void EulerSolver::solve(System *in, System *out) {
    initializeSystem(out, m_bodyCount, m_dt);

    out->dt = m_dt;

    for (int i = 0; i < m_bodyCount; ++i) {
        out->Acceleration_X[i] = in->Acceleration_X[i];
        out->Acceleration_Y[i] = in->Acceleration_Y[i];
        out->Angles[i] = in->Angles[i] + in->AngularVelocity[i] * in->dt;
        out->AngularAcceleration[i] = in->AngularAcceleration[i];
        out->AngularVelocity[i] = in->AngularVelocity[i] + in->AngularAcceleration[i] * in->dt;
        out->Position_X[i] = in->Position_X[i] + in->Velocity_X[i] * in->dt;
        out->Position_Y[i] = in->Position_Y[i] + in->Velocity_Y[i] * in->dt;
        out->Velocity_X[i] = in->Velocity_X[i] + in->Acceleration_X[i] * in->dt;
        out->Velocity_Y[i] = in->Velocity_Y[i] + in->Acceleration_Y[i] * in->dt;
    }
}

void EulerSolver::end() {
    destroySystem(&m_initial);
}
