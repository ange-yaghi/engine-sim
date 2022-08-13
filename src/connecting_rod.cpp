#include "../include/connecting_rod.h"

#include <cmath>

ConnectingRod::ConnectingRod() {
    m_centerOfMass = 0.0;
    m_length = 0.0;
    m_m = 0.0;
    m_I = 0.0;
    m_journal = 0;
    m_master = nullptr;
    m_crankshaft = nullptr;
    m_piston = nullptr;
    m_slaveThrow = 0;

    m_rodJournalAngles = nullptr;
    m_rodJournalCount = 0;
}

ConnectingRod::~ConnectingRod() {
    /* void */
}

void ConnectingRod::initialize(const Parameters &params) {
    m_centerOfMass = params.CenterOfMass;
    m_length = params.Length;
    m_m = params.Mass;
    m_I = params.MomentOfInertia;
    m_journal = params.Journal;
    m_crankshaft = params.crankshaft;
    m_piston = params.piston;

    m_rodJournalAngles = new double[params.RodJournals];
    m_rodJournalCount = params.RodJournals;
    m_slaveThrow = params.SlaveThrow;
    m_master = params.Master;
}

double ConnectingRod::getBigEndLocal() const {
    return -(m_length / 2) + m_centerOfMass;
}

double ConnectingRod::getLittleEndLocal() const {
    return (m_length / 2) - m_centerOfMass;
}

void ConnectingRod::setRodJournalAngle(int i, double angle) {
    m_rodJournalAngles[i] = angle;
}

void ConnectingRod::getRodJournalPositionLocal(int i, double *x, double *y) {
    const double journalAngle = getRodJournalAngle(i);
    const double journal_x_local = std::cos(journalAngle) * m_slaveThrow;
    const double journal_y_local = std::sin(journalAngle) * m_slaveThrow;

    *x = journal_x_local;
    *y = journal_y_local + getBigEndLocal();
}

void ConnectingRod::getRodJournalPositionGlobal(int i, double *x, double *y) {
    double lx, ly;
    getRodJournalPositionLocal(i, &lx, &ly);

    const double angle = m_body.theta;
    const double dx = std::cos(angle);
    const double dy = std::sin(angle);

    *x = (dx * lx - dy * ly) + m_body.p_x;
    *y = (dy * lx + dx * ly) + m_body.p_y;
}

int ConnectingRod::getLayer() const {
    if (m_master != nullptr) {
        return m_master->getLayer();
    }
    else {
        return getJournal();
    }
}
