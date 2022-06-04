#include "../include/engine.h"

#include "../include/constants.h"

#include <cmath>
#include <assert.h>

Engine::Engine() {
    m_crankshafts = nullptr;
    m_cylinderBanks = nullptr;
    m_heads = nullptr;
    m_pistons = nullptr;
    m_connectingRods = nullptr;
    m_exhaustSystems = nullptr;
    m_intakes = nullptr;

    m_crankshaftCount = 0;
    m_cylinderBankCount = 0;
    m_cylinderCount = 0;
    m_intakeCount = 0;

    m_throttle = 0.0f;
}

Engine::~Engine() {
    assert(m_crankshafts == nullptr);
    assert(m_cylinderBanks == nullptr);
    assert(m_pistons == nullptr);
    assert(m_connectingRods == nullptr);
    assert(m_heads == nullptr);
    assert(m_exhaustSystems == nullptr);
    assert(m_intakes == nullptr);
}

void Engine::initialize(const Parameters &params) {
    m_crankshaftCount = params.CrankshaftCount;
    m_cylinderCount = params.CylinderCount;
    m_cylinderBankCount = params.CylinderBanks;
    m_exhaustSystemCount = params.ExhaustSystemCount;
    m_intakeCount = params.IntakeCount;

    m_crankshafts = new Crankshaft[m_crankshaftCount];
    m_cylinderBanks = new CylinderBank[m_cylinderBankCount];
    m_heads = new CylinderHead[m_cylinderBankCount];
    m_pistons = new Piston[m_cylinderCount];
    m_connectingRods = new ConnectingRod[m_cylinderCount];
    m_exhaustSystems = new ExhaustSystem[m_exhaustSystemCount];
    m_intakes = new Intake[m_intakeCount];
}

void Engine::destroy() {
    for (int i = 0; i < m_crankshaftCount; ++i) {
        m_crankshafts[i].destroy();
    }

    for (int i = 0; i < m_cylinderCount; ++i) {
        m_pistons[i].destroy();
        m_connectingRods[i].destroy();
    }

    for (int i = 0; i < m_exhaustSystemCount; ++i) {
        m_exhaustSystems[i].destroy();
    }

    for (int i = 0; i < m_intakeCount; ++i) {
        m_intakes[i].destroy();
    }

    m_ignitionModule.destroy();

    delete[] m_crankshafts;
    delete[] m_cylinderBanks;
    delete[] m_heads;
    delete[] m_pistons;
    delete[] m_connectingRods;
    delete[] m_exhaustSystems;
    delete[] m_intakes;

    m_crankshafts = nullptr;
    m_cylinderBanks = nullptr;
    m_pistons = nullptr;
    m_connectingRods = nullptr;
    m_heads = nullptr;
    m_exhaustSystems = nullptr;
    m_intakes = nullptr;
}

void Engine::setThrottle(double throttle) {
    const float throttlePlateThrottle = std::cos(throttle * 3.14159 / 2);
    for (int i = 0; i < m_intakeCount; ++i) {
        m_intakes[i].m_throttle = throttle;
    }

    m_throttle = throttle;
}

double Engine::getThrottle() const {
    return m_throttle;
}

double Engine::getDisplacement() const {
    double displacement = 0;
    for (int i = 0; i < m_cylinderCount; ++i) {
        const Piston &piston = m_pistons[i];
        const CylinderBank &bank = *piston.m_bank;
        const ConnectingRod &rod = *piston.m_rod;
        const Crankshaft &shaft = *rod.m_crankshaft;

        const double r = bank.m_bore / 2.0;
        const double V = Constants::pi * r * r * shaft.m_throw;

        displacement += V;
    }

    return displacement;
}

double Engine::getIntakeFlowRate() const {
    double airIntake = 0;
    for (int i = 0; i < m_intakeCount; ++i) {
        airIntake += m_intakes[i].m_flowRate;
    }

    return airIntake;
}

double Engine::getManifoldPressure() const {
    double averagePressure = 0.0;
    for (int i = 0; i < m_intakeCount; ++i) {
        averagePressure += m_intakes[i].m_system.pressure();
    }

    return averagePressure / m_intakeCount;
}

double Engine::getRpm() const {
    if (m_crankshaftCount == 0) return 0;
    return -(getCrankshaft(0)->m_body.v_theta / (2 * Constants::pi)) * 60;
}
