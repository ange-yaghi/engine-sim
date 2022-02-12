#include "../include/engine.h"

#include "../include/constants.h"

#include <assert.h>

Engine::Engine() {
    m_crankshafts = nullptr;
    m_cylinderBanks = nullptr;
    m_pistons = nullptr;
    m_connectingRods = nullptr;

    m_crankshaftCount = 0;
    m_cylinderBankCount = 0;
    m_cylinderCount = 0;
}

Engine::~Engine() {
    assert(m_crankshafts == nullptr);
    assert(m_cylinderBanks == nullptr);
    assert(m_pistons == nullptr);
    assert(m_connectingRods == nullptr);
}

void Engine::initialize(const Parameters &params) {
    m_crankshaftCount = params.CrankshaftCount;
    m_cylinderCount = params.CylinderCount;
    m_cylinderBankCount = params.CylinderBanks;

    m_crankshafts = new Crankshaft[m_crankshaftCount];
    m_cylinderBanks = new CylinderBank[m_cylinderBankCount];
    m_pistons = new Piston[m_cylinderCount];
    m_connectingRods = new ConnectingRod[m_cylinderCount];
}

void Engine::destroy() {
    for (int i = 0; i < m_crankshaftCount; ++i) {
        m_crankshafts[i].destroy();
    }

    for (int i = 0; i < m_cylinderCount; ++i) {
        m_pistons[i].destroy();
        m_connectingRods[i].destroy();
    }

    delete[] m_crankshafts;
    delete[] m_cylinderBanks;
    delete[] m_pistons;
    delete[] m_connectingRods;

    m_crankshafts = nullptr;
    m_cylinderBanks = nullptr;
    m_pistons = nullptr;
    m_connectingRods = nullptr;
}

double Engine::getRpm() const {
    if (m_crankshaftCount == 0) return 0;
    return (getCrankshaft(0)->m_body.v_theta / (2 * Constants::pi)) * 60;
}
