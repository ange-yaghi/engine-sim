#include "../include/connecting_rod.h"

ConnectingRod::ConnectingRod() {
    m_centerOfMass = 0.0;
    m_length = 0.0;
    m_m = 0.0;
    m_I = 0.0;
    m_journal = 0;
    m_crankshaft = nullptr;
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
    m_crankshaft = params.Crankshaft;
}

double ConnectingRod::getBigEndLocal() const {
    return -(m_length / 2) + m_centerOfMass;
}

double ConnectingRod::getLittleEndLocal() const {
    return (m_length / 2) - m_centerOfMass;
}
