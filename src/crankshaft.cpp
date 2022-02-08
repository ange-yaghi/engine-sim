#include "../include/crankshaft.h"

#include <cmath>
#include <assert.h>

Crankshaft::Crankshaft() {
    m_rodJournalAngles = nullptr;
    m_rodJournalCount = 0;
    m_throw = 0.0;
    m_m = 0.0;
    m_I = 0.0;
    m_flywheelMass = 0.0;
    m_p_x = m_p_y = 0.0;
}

Crankshaft::~Crankshaft() {
    assert(m_rodJournalAngles == nullptr);
}

void Crankshaft::initialize(Parameters &params) {
    m_m = params.Mass;
    m_flywheelMass = params.FlywheelMass;
    m_I = params.MomentOfInertia;
    m_throw = params.CrankThrow;
    m_rodJournalCount = params.RodJournals;
    m_rodJournalAngles = new double[m_rodJournalCount];
    m_p_x = params.Pos_x;
    m_p_y = params.Pos_y;
}

void Crankshaft::destroy() {
    if (m_rodJournalAngles != nullptr) delete[] m_rodJournalAngles;

    m_rodJournalAngles = nullptr;
}

void Crankshaft::getRodJournalPositionLocal(int i, double *x, double *y) {
    const double theta = m_rodJournalAngles[i];

    *x = std::cos(theta) * m_throw;
    *y = std::sin(theta) * m_throw;
}

void Crankshaft::setRodJournalAngle(int i, double angle) {
    assert(i < m_rodJournalCount && i > 0);

    m_rodJournalAngles[i] = angle;
}
