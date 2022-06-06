#include "../include/engine_simulator.h"

#include "../include/constants.h"
#include "../include/units.h"

#include <cmath>
#include <assert.h>
#include <chrono>

EngineSimulator::EngineSimulator() {
    m_engine = nullptr;
    m_steps = 1;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_combustionChambers = nullptr;
    m_crankshaftLoads = nullptr;

    m_physicsProcessingTime = 0.0;
}

EngineSimulator::~EngineSimulator() {
    assert(m_crankConstraints == nullptr);
    assert(m_cylinderWallConstraints == nullptr);
    assert(m_linkConstraints == nullptr);
    assert(m_combustionChambers == nullptr);
    assert(m_crankshaftLoads == nullptr);
}

void EngineSimulator::synthesize(Engine *engine, SystemType systemType) {
    static constexpr double pi = 3.14159265359;

    if (systemType == SystemType::NsvOptimized) {
        atg_scs::OptimizedNsvRigidBodySystem *system =
            new atg_scs::OptimizedNsvRigidBodySystem;
        system->initialize(
            new atg_scs::GaussianEliminationSleSolver);
        m_system = system;
    }
    else {
        atg_scs::GenericRigidBodySystem *system =
            new atg_scs::GenericRigidBodySystem;
        system->initialize(
            new atg_scs::GaussianEliminationSleSolver,
            new atg_scs::NsvOdeSolver);
        m_system = system;
    }

    m_engine = engine;

    const int crankCount = engine->getCrankshaftCount();
    const int cylinderCount = engine->getCylinderCount();
    const int linkCount = cylinderCount * 2;

    m_crankConstraints = new atg_scs::FixedPositionConstraint[crankCount];
    m_cylinderWallConstraints = new atg_scs::LineConstraint[cylinderCount];
    m_combustionChambers = new CombustionChamber[cylinderCount];
    m_linkConstraints = new atg_scs::LinkConstraint[linkCount];
    m_crankshaftLoads = new CrankshaftLoad[crankCount];

    const double ks = 5000;
    const double kd = 10;

    for (int i = 0; i < crankCount; ++i) {
        m_crankConstraints[i].setBody(&engine->getCrankshaft(i)->m_body);
        m_crankConstraints[i].setWorldPosition(
                engine->getCrankshaft(i)->m_p_x,
                engine->getCrankshaft(i)->m_p_y);
        m_crankConstraints[i].setLocalPosition(0.0, 0.0);
        m_crankConstraints[i].m_kd = kd;
        m_crankConstraints[i].m_ks = ks;

        engine->getCrankshaft(i)->m_body.p_x = engine->getCrankshaft(i)->m_p_x;
        engine->getCrankshaft(i)->m_body.p_y = engine->getCrankshaft(i)->m_p_y;
        engine->getCrankshaft(i)->m_body.theta = 0;
        engine->getCrankshaft(i)->m_body.m =
            engine->getCrankshaft(i)->m_m + engine->getCrankshaft(i)->m_flywheelMass;
        engine->getCrankshaft(i)->m_body.I = engine->getCrankshaft(i)->m_I;

        m_system->addRigidBody(&engine->getCrankshaft(i)->m_body);
        m_system->addConstraint(&m_crankConstraints[i]);

        m_crankshaftLoads[i].setCrankshaft(engine->getCrankshaft(i));
        m_crankshaftLoads[i].m_bearingConstraint = &m_crankConstraints[i];
        m_system->addConstraint(&m_crankshaftLoads[i]);
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
        m_cylinderWallConstraints[i].m_ks = ks;
        m_cylinderWallConstraints[i].m_kd = kd;

        piston->m_cylinderConstraint = &m_cylinderWallConstraints[i];

        m_linkConstraints[i * 2 + 0].setBody1(&connectingRod->m_body);
        m_linkConstraints[i * 2 + 0].setBody2(&piston->m_body);
        m_linkConstraints[i * 2 + 0]
            .setLocalPosition1(0.0, connectingRod->getLittleEndLocal());
        m_linkConstraints[i * 2 + 0].setLocalPosition2(0.0, piston->m_wristPinLocation);
        m_linkConstraints[i * 2 + 0].m_ks = ks;
        m_linkConstraints[i * 2 + 0].m_kd = kd;

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
        m_linkConstraints[i * 2 + 1].m_ks = ks;
        m_linkConstraints[i * 2 + 0].m_kd = kd;

        piston->m_body.m = piston->m_mass;
        piston->m_body.I = 1.0;

        connectingRod->m_body.m = connectingRod->m_m;
        connectingRod->m_body.I = connectingRod->m_I;

        m_system->addRigidBody(&piston->m_body);
        m_system->addRigidBody(&connectingRod->m_body);
        m_system->addConstraint(&m_linkConstraints[i * 2 + 0]);
        m_system->addConstraint(&m_linkConstraints[i * 2 + 1]);
        m_system->addConstraint(&m_cylinderWallConstraints[i]);

        CombustionChamber &chamber = m_combustionChambers[i];
        chamber.m_bank = bank;
        chamber.m_piston = piston;
        chamber.m_head = engine->getHead(bank->m_index);

        m_system->addForceGenerator(&chamber);
    }
}

void EngineSimulator::placeAndInitialize() {
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        ConnectingRod *rod = m_engine->getConnectingRod(i);
        Piston *piston = m_engine->getPiston(i);
        CylinderBank *bank = piston->m_bank;

        double p_x, p_y;
        rod->m_crankshaft->getRodJournalPositionLocal(rod->m_journal, &p_x, &p_y);

        // (bank->m_dx * s - p_x)^2 + (bank->m_dy * s - p_y)^2 = rod->m_length * rod->m_length

        const double a = bank->m_dx * bank->m_dx + bank->m_dy * bank->m_dy;
        const double b = -2 * bank->m_dx * p_x - 2 * bank->m_dy * p_y;
        const double c = p_x * p_x + p_y * p_y - rod->m_length * rod->m_length;

        const double det = b * b - 4 * a * c;
        if (det < 0) continue;

        const double sqrt_det = std::sqrt(det);
        const double s0 = (-b + sqrt_det) / (2 * a);
        const double s1 = (-b - sqrt_det) / (2 * a);

        const double s = std::max(s0, s1);
        if (s < 0) continue;

        const double e_x = s * bank->m_dx;
        const double e_y = s * bank->m_dy;

        const double theta = ((e_y - p_y) > 0)
            ? std::acos((e_x - p_x) / rod->m_length)
            : 2 * constants::pi - std::acos((e_x - p_x) / rod->m_length);
        rod->m_body.theta = theta - constants::pi / 2;

        double cl_x, cl_y;
        rod->m_body.localToWorld(0, rod->getBigEndLocal(), &cl_x, &cl_y);
        rod->m_body.p_x += p_x + rod->m_crankshaft->m_p_x - cl_x;
        rod->m_body.p_y += p_y + rod->m_crankshaft->m_p_y - cl_y;

        piston->m_body.p_x = e_x + rod->m_crankshaft->m_p_x;
        piston->m_body.p_y = e_y + rod->m_crankshaft->m_p_y;
        piston->m_body.theta = bank->m_angle + constants::pi;
    }

    for (int i = 0; i < cylinderCount; ++i) {
        m_combustionChambers[i].initialize(
            units::pressure(1, units::atm),
            units::celcius(25));
    }

    m_engine->getIgnitionModule()->reset();
}

void EngineSimulator::start() {
    m_simulationStart = std::chrono::steady_clock::now();
    m_currentIteration = 0;
}

bool EngineSimulator::simulateStep(double dt) {
    if (m_currentIteration >= m_steps) {
        auto s1 = std::chrono::steady_clock::now();

        const long long lastFrame =
            std::chrono::duration_cast<std::chrono::microseconds>(s1 - m_simulationStart).count();
        m_physicsProcessingTime = m_physicsProcessingTime * 0.98 + 0.02 * lastFrame;

        return false;
    }
    else if (m_currentIteration == 0) {
        for (int j = 0; j < m_engine->getIntakeCount(); ++j) {
            m_engine->getIntake(j)->m_flowRate = 0;
        }
    }

    const double dt_sub = dt / m_steps;

    m_system->process(dt_sub, 1);

    IgnitionModule *im = m_engine->getIgnitionModule();
    im->update(dt_sub);

    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        if (im->getIgnitionEvent(i)) {
            m_combustionChambers[i].ignite();
        }

        m_combustionChambers[i].update(dt_sub);
    }

    for (int i = 0; i < cylinderCount; ++i) {
        m_combustionChambers[i].resetLastTimestepExhaustFlow();
    }

    constexpr int iterations = 16;
    for (int i = 0; i < iterations; ++i) {
        for (int j = 0; j < m_engine->getExhaustSystemCount(); ++j) {
            m_engine->getExhaustSystem(j)->start();
            m_engine->getExhaustSystem(j)->process(dt_sub / iterations);
        }

        for (int j = 0; j < m_engine->getIntakeCount(); ++j) {
            m_engine->getIntake(j)->start();
            m_engine->getIntake(j)->process(dt_sub / iterations);

            m_engine->getIntake(j)->m_flowRate +=
                -m_engine->getIntake(j)->m_flow / dt;
        }

        for (int j = 0; j < cylinderCount; ++j) {
            m_combustionChambers[j].start();
            m_combustionChambers[j].flow(dt_sub / iterations);
            m_combustionChambers[j].end();
        }

        for (int j = 0; j < m_engine->getIntakeCount(); ++j) {
            m_engine->getIntake(j)->end();
        }

        for (int j = 0; j < m_engine->getExhaustSystemCount(); ++j) {
            m_engine->getExhaustSystem(j)->end();
        }
    }

    im->resetIgnitionEvents();

    ++m_currentIteration;

    return true;
}

void EngineSimulator::destroy() {
    delete[] m_crankConstraints;
    delete[] m_cylinderWallConstraints;
    delete[] m_linkConstraints;
    delete[] m_combustionChambers;
    delete[] m_crankshaftLoads;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_combustionChambers = nullptr;
    m_crankshaftLoads = nullptr;

    m_engine = nullptr;
}

CombustionChamber *EngineSimulator::getCombustionChamber(int i) {
    assert(i >= 0 && i < m_engine->getCylinderCount());

    return &m_combustionChambers[i];
}

CrankshaftLoad *EngineSimulator::getCrankshaftLoad(int i) {
    return &m_crankshaftLoads[i];
}
