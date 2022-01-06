#include "../include/rk4_solver.h"

Rk4Solver::Rk4Solver() {
    m_stage = RkStage::Undefined;
    m_nextStage = RkStage::Undefined;
}

Rk4Solver::~Rk4Solver() {
    /* void */
}

void Rk4Solver::start(System *initial, double dt) {
    OdeSolver::start(initial, dt);

    copySystem(initial, &m_initial);

    m_stage = RkStage::Stage_1;
}

bool Rk4Solver::step(System *system) {
    RkStage nextStage = RkStage::Undefined;
    bool complete = false;

    switch (m_stage) {
    case RkStage::Stage_1:
        nextStage = RkStage::Stage_2;
        break;
    case RkStage::Stage_2:
        nextStage = RkStage::Stage_3;
        break;
    case RkStage::Stage_3:
        nextStage = RkStage::Stage_4;
        break;
    case RkStage::Stage_4:
        complete = true;
    }

    switch (m_stage) {
    case RkStage::Stage_1:
        OdeSolver::copySystem(&m_initial, system);
        break;
    case RkStage::Stage_2:
        OdeSolver::copySystem(&m_initial, system);
        
        for (int i = 0; i < m_bodyCount; ++i) {
            system->AngularVelocity[i] += m_dt * m_k1.AngularAcceleration[i] / 2.0f;
            system->Velocity_X[i] += m_dt * m_k1.Acceleration_X[i] / 2.0f;
            system->Velocity_Y[i] += m_dt * m_k1.Acceleration_Y[i] / 2.0f;

            system->Angles[i] += m_dt * m_k1.AngularVelocity[i] / 2.0f;
            system->Position_X[i] += m_dt * m_k1.Velocity_X[i] / 2.0f;
            system->Position_Y[i] += m_dt * m_k1.Velocity_Y[i] / 2.0f;
        }

        system->dt = m_dt / 2;

        break;
    case RkStage::Stage_3:
        OdeSolver::copySystem(&m_initial, system);

        for (int i = 0; i < m_bodyCount; ++i) {
            system->AngularVelocity[i] += m_dt * m_k2.AngularAcceleration[i] / 2.0f;
            system->Velocity_X[i] += m_dt * m_k2.Acceleration_X[i] / 2.0f;
            system->Velocity_Y[i] += m_dt * m_k2.Acceleration_Y[i] / 2.0f;

            system->Angles[i] += m_dt * m_k2.AngularVelocity[i] / 2.0f;
            system->Position_X[i] += m_dt * m_k2.Velocity_X[i] / 2.0f;
            system->Position_Y[i] += m_dt * m_k2.Velocity_Y[i] / 2.0f;
        }

        system->dt = m_dt / 2;
        break;
    case RkStage::Stage_4:
        OdeSolver::copySystem(&m_initial, system);

        for (int i = 0; i < m_bodyCount; ++i) {
            system->AngularVelocity[i] += m_dt * m_k3.AngularAcceleration[i];
            system->Velocity_X[i] += m_dt * m_k3.Acceleration_X[i];
            system->Velocity_Y[i] += m_dt * m_k3.Acceleration_Y[i];

            system->Angles[i] += m_dt * m_k3.AngularVelocity[i];
            system->Position_X[i] += m_dt * m_k3.Velocity_X[i];
            system->Position_Y[i] += m_dt * m_k3.Velocity_Y[i];
        }

        system->dt = m_dt;
        break;
    }

    m_nextStage = nextStage;

    return complete;
}

void Rk4Solver::solve(System *in, System *out) {
    OdeSolver::copySystem(in, out);

    switch (m_stage) {
    case RkStage::Stage_1:
        OdeSolver::copySystem(in, &m_k1);
        break;
    case RkStage::Stage_2:
        OdeSolver::copySystem(in, &m_k2);
        break;
    case RkStage::Stage_3:
        OdeSolver::copySystem(in, &m_k3);
        break;
    case RkStage::Stage_4:
        OdeSolver::copySystem(in, &m_k4);

        for (int i = 0; i < m_bodyCount; ++i) {
            out->AngularVelocity[i] =
                m_initial.AngularVelocity[i] +
                (1 / 6.0) * (
                    m_k1.AngularAcceleration[i] +
                    2.0 * m_k2.AngularAcceleration[i] +
                    2.0 * m_k3.AngularAcceleration[i] +
                    m_k4.AngularAcceleration[i]) * m_dt;
            out->Angles[i] =
                m_initial.Angles[i] +
                (1 / 6.0) * (
                    m_k1.AngularVelocity[i] +
                    2.0 * m_k2.AngularVelocity[i] +
                    2.0 * m_k3.AngularVelocity[i] +
                    m_k4.AngularVelocity[i]) * m_dt;
            out->Velocity_X[i] =
                m_initial.Velocity_X[i] +
                (1 / 6.0) * (
                    m_k1.Acceleration_X[i] +
                    2.0 * m_k2.Acceleration_X[i] +
                    2.0 * m_k3.Acceleration_X[i] +
                    m_k4.Acceleration_X[i]) * m_dt;
            out->Velocity_Y[i] =
                m_initial.Velocity_Y[i] +
                (1 / 6.0) * (
                    m_k1.Acceleration_Y[i] +
                    2.0 * m_k2.Acceleration_Y[i] +
                    2.0 * m_k3.Acceleration_Y[i] +
                    m_k4.Acceleration_Y[i]) * m_dt;
            out->Position_X[i] =
                m_initial.Position_X[i] +
                (1 / 6.0) * (
                    m_k1.Velocity_X[i] +
                    2.0 * m_k2.Velocity_X[i] +
                    2.0 * m_k3.Velocity_X[i] +
                    m_k4.Velocity_X[i]) * m_dt;
            out->Position_Y[i] =
                m_initial.Position_Y[i] +
                (1 / 6.0) * (
                    m_k1.Velocity_Y[i] +
                    2.0 * m_k2.Velocity_Y[i] +
                    2.0 * m_k3.Velocity_Y[i] +
                    m_k4.Velocity_Y[i]) * m_dt;
        }

        break;
    }

    m_stage = m_nextStage;
}

void Rk4Solver::end() {
    OdeSolver::destroySystem(&m_initial);
    OdeSolver::destroySystem(&m_k1);
    OdeSolver::destroySystem(&m_k2);
    OdeSolver::destroySystem(&m_k3);
    OdeSolver::destroySystem(&m_k4);
}
