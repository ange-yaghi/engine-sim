#include "../include/engine_simulator.h"

#include <cmath>
#include <assert.h>

EngineSimulator::EngineSimulator() {
    m_engine = nullptr;
    m_steps = 1;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
}

EngineSimulator::~EngineSimulator() {
    assert(m_crankConstraints == nullptr);
    assert(m_cylinderWallConstraints == nullptr);
    assert(m_linkConstraints == nullptr);
}

void EngineSimulator::synthesize(Engine *engine) {
    static constexpr double pi = 3.14159265359;

    m_system.initialize(
        new atg_scs::GaussianEliminationSleSolver,
        new atg_scs::EulerOdeSolver);

    m_engine = engine;

    const int crankCount = engine->getCrankshaftCount();
    const int cylinderCount = engine->getCylinderCount();
    const int linkCount = cylinderCount * 2;

    m_crankConstraints = new atg_scs::FixedPositionConstraint[crankCount];
    m_cylinderWallConstraints = new atg_scs::LineConstraint[cylinderCount];
    m_linkConstraints = new atg_scs::LinkConstraint[linkCount];

    for (int i = 0; i < crankCount; ++i) {
        m_crankConstraints[i].setBody(&engine->getCrankshaft(i)->m_body);
        m_crankConstraints[i].setWorldPosition(
                engine->getCrankshaft(i)->m_p_x,
                engine->getCrankshaft(i)->m_p_y);
        m_crankConstraints[i].setLocalPosition(0.0, 0.0);

        engine->getCrankshaft(i)->m_body.p_x = engine->getCrankshaft(i)->m_p_x;
        engine->getCrankshaft(i)->m_body.p_y = engine->getCrankshaft(i)->m_p_y;
        engine->getCrankshaft(i)->m_body.m =
            engine->getCrankshaft(i)->m_m + engine->getCrankshaft(i)->m_flywheelMass;
        engine->getCrankshaft(i)->m_body.I = engine->getCrankshaft(i)->m_I;

        m_system.addRigidBody(&engine->getCrankshaft(i)->m_body);
        m_system.addConstraint(&m_crankConstraints[i]);
    }

    for (int i = 0; i < cylinderCount; ++i) {
        Piston *piston = engine->getPiston(i);
        ConnectingRod *connectingRod = piston->m_rod;
        Crankshaft *crankshaft = connectingRod->m_crankshaft;

        CylinderBank *bank = piston->m_bank;
        const double dx = std::cos(bank->m_angle + pi / 2);
        const double dy = std::sin(bank->m_angle + pi / 2);

        m_cylinderWallConstraints[i].setBody(&piston->m_body);
        m_cylinderWallConstraints[i].m_dx = dx;
        m_cylinderWallConstraints[i].m_dy = dy;
        m_cylinderWallConstraints[i].m_local_x = 0.0;
        m_cylinderWallConstraints[i].m_local_y = piston->m_wristPinLocation;
        m_cylinderWallConstraints[i].m_p0_x = crankshaft->m_p_x;
        m_cylinderWallConstraints[i].m_p0_y = crankshaft->m_p_y;

        m_linkConstraints[i * 2 + 0].setBody1(&connectingRod->m_body);
        m_linkConstraints[i * 2 + 0].setBody2(&piston->m_body);
        m_linkConstraints[i * 2 + 0]
            .setLocalPosition1(0.0, connectingRod->getLittleEndLocal());
        m_linkConstraints[i * 2 + 0].setLocalPosition2(0.0, piston->m_wristPinLocation);

        double journal_x = 0.0, journal_y = 0.0;
        crankshaft->getRodJournalPositionLocal(
                connectingRod->m_journal,
                &journal_x,
                &journal_y);

        m_linkConstraints[i * 2 + 1].setBody1(&connectingRod->m_body);
        m_linkConstraints[i * 2 + 1].setBody2(&crankshaft->m_body);
        m_linkConstraints[i * 2 + 1]
            .setLocalPosition1(0.0, connectingRod->getBigEndLocal());
        m_linkConstraints[i * 2 + 1]
            .setLocalPosition2(journal_x, journal_y);

        piston->m_body.m = piston->m_mass;
        piston->m_body.I = 1.0;
        
        connectingRod->m_body.m = connectingRod->m_m;
        connectingRod->m_body.I = connectingRod->m_I;

        m_system.addRigidBody(&piston->m_body);
        m_system.addRigidBody(&connectingRod->m_body);
        m_system.addConstraint(&m_linkConstraints[i * 2 + 0]);
        m_system.addConstraint(&m_linkConstraints[i * 2 + 1]);
        m_system.addConstraint(&m_cylinderWallConstraints[i]);
    }
}

void EngineSimulator::update(float dt) {
    m_system.process(dt, m_steps);
}

void EngineSimulator::destroy() {
    delete[] m_crankConstraints;
    delete[] m_cylinderWallConstraints;
    delete[] m_linkConstraints;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;

    m_engine = nullptr;
}
