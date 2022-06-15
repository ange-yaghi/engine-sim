#include "..\include\simulator.h"
#include "../include/simulator.h"

#include "../include/constants.h"
#include "../include/units.h"
#include "../include/vehicle_load.h"

#include <cmath>
#include <assert.h>
#include <chrono>

Simulator::Simulator() {
    m_engine = nullptr;
    i_steps = 1;
    m_currentIteration = 0;
    m_simulationSpeed = 1.0;
    m_targetSynthesizerLatency = 0.1;
    m_simulationFrequency = 13000;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_crankshaftLoads = nullptr;
    m_system = nullptr;

    m_physicsProcessingTime = 0.0;
    m_exhaustFlowStagingBuffer = nullptr;

    m_travelledDistance = 0.0;
}

Simulator::~Simulator() {
    assert(m_crankConstraints == nullptr);
    assert(m_cylinderWallConstraints == nullptr);
    assert(m_linkConstraints == nullptr);
    assert(m_crankshaftLoads == nullptr);
    assert(m_system == nullptr);
}

void Simulator::initialize(Engine *engine, SystemType systemType) {
    static constexpr double pi = 3.14159265359;

    atg_scs::ConjugateGradientSleSolver *cgs = new atg_scs::ConjugateGradientSleSolver;
    cgs->setMaxError(1E-2);
    cgs->setMinError(1E-2);

    if (systemType == SystemType::NsvOptimized) {
        atg_scs::OptimizedNsvRigidBodySystem *system =
            new atg_scs::OptimizedNsvRigidBodySystem;
        system->initialize(
            new atg_scs::GaussSeidelSleSolver);
        //system->initialize(cgs);
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
    }

    //VehicleLoad *load = new VehicleLoad;
    m_clutchConstraint.setBody1(&m_engine->getCrankshaft(0)->m_body);
    m_clutchConstraint.setBody2(&m_vehicleMass);
    m_clutchConstraint.m_maxTorque = units::torque(1000, units::ft_lb);
    m_clutchConstraint.m_minTorque = -units::torque(1000, units::ft_lb);

    constexpr double m_car = 1597;
    constexpr double gear_ratio = 4.0;
    constexpr double diff_ratio = 4.10;
    constexpr double tire_radius = units::distance(10, units::inch);
    constexpr double f = tire_radius / (diff_ratio * gear_ratio);

    m_vehicleMass.I = m_car * f * f;
    m_vehicleMass.p_x = m_vehicleMass.p_y = 0;
    m_vehicleMass.m = 1000;
    m_vehicleMass.theta = 0;
    m_vehicleMass.v_theta = -100;

    //load->initialize(m_engine->getCrankshaft(0), &m_vehicleMass);

    //m_system->addForceGenerator(load);

    m_system->addConstraint(&m_clutchConstraint);
    m_system->addRigidBody(&m_vehicleMass);

    m_crankshaftLoads[0].setCrankshaft(engine->getCrankshaft(0));
    //m_crankshaftLoads[0].m_bodies[0] = &m_vehicleMass;
    m_crankshaftLoads[0].m_bearingConstraint = &m_crankConstraints[0];
    m_system->addConstraint(&m_crankshaftLoads[0]);

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
        m_system->addForceGenerator(engine->getChamber(i));
    }

    placeAndInitialize();
    initializeSynthesizer();
}

double Simulator::getVehicleSpeed() const {
    const double E_r = 0.5 * m_vehicleMass.I * m_vehicleMass.v_theta * m_vehicleMass.v_theta;
    const double vehicleSpeed = std::sqrt(2 * E_r / m_vehicleMass.m);

    return vehicleSpeed;
}

void Simulator::placeAndInitialize() {
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

    m_engine->getIgnitionModule()->reset();
}

void Simulator::startFrame(double dt) {
    m_simulationStart = std::chrono::steady_clock::now();
    m_currentIteration = 0;
    m_synthesizer.setInputSampleRate(m_simulationFrequency * m_simulationSpeed);

    const double timestep = getTimestep();
    i_steps = (int)std::round((dt * m_simulationSpeed) / timestep);

    const int targetLatency = getSynthesizerInputLatencyTarget();
    if (m_synthesizer.getInputWriteOffset() < targetLatency) {
        ++i_steps;
    }
    else if (m_synthesizer.getInputWriteOffset() > targetLatency) {
        i_steps -= (m_synthesizer.getInputWriteOffset() - targetLatency);
        if (i_steps < 0) {
            i_steps = 0;
        }
    }

    for (int i = 0; i < m_engine->getIntakeCount(); ++i) {
        m_engine->getIntake(i)->m_flowRate = 0;
    }
}

void Simulator::startAudioRenderingThread() {
    m_synthesizer.startAudioRenderingThread();
}

void Simulator::endAudioRenderingThread() {
    m_synthesizer.endAudioRenderingThread();
}

int Simulator::getSynthesizerInputLatencyTarget() const {
    return (int)std::ceil(m_targetSynthesizerLatency * m_simulationFrequency * m_simulationSpeed);
}

bool Simulator::simulateStep() {
    if (m_currentIteration >= i_steps) {
        auto s1 = std::chrono::steady_clock::now();

        const long long lastFrame =
            std::chrono::duration_cast<std::chrono::microseconds>(s1 - m_simulationStart).count();
        m_physicsProcessingTime = m_physicsProcessingTime * 0.98 + 0.02 * lastFrame;

        return false;
    }

    const double timestep = 1.0 / m_simulationFrequency;
    m_system->process(timestep, 1);

    m_travelledDistance += getVehicleSpeed() * timestep;

    for (int i = 0; i < m_engine->getCrankshaftCount(); ++i) {
        m_engine->getCrankshaft(i)->resetAngle();
    }

    IgnitionModule *im = m_engine->getIgnitionModule();
    im->update(timestep);

    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        if (im->getIgnitionEvent(i)) {
            m_engine->getChamber(i)->ignite();
        }

        m_engine->getChamber(i)->update(timestep);
    }

    for (int i = 0; i < cylinderCount; ++i) {
        m_engine->getChamber(i)->resetLastTimestepExhaustFlow();
    }

    constexpr int iterations = 16;
    for (int i = 0; i < iterations; ++i) {
        for (int j = 0; j < m_engine->getExhaustSystemCount(); ++j) {
            m_engine->getExhaustSystem(j)->start();
            m_engine->getExhaustSystem(j)->process(timestep / iterations);
        }

        for (int j = 0; j < m_engine->getIntakeCount(); ++j) {
            m_engine->getIntake(j)->start();
            m_engine->getIntake(j)->process(timestep / iterations);

            m_engine->getIntake(j)->m_flowRate +=
                -m_engine->getIntake(j)->m_flow;
        }

        for (int j = 0; j < cylinderCount; ++j) {
            m_engine->getChamber(j)->start();
            m_engine->getChamber(j)->flow(timestep / iterations);
            m_engine->getChamber(j)->end();
        }

        for (int j = 0; j < m_engine->getIntakeCount(); ++j) {
            m_engine->getIntake(j)->end();
        }

        for (int j = 0; j < m_engine->getExhaustSystemCount(); ++j) {
            m_engine->getExhaustSystem(j)->end();
        }
    }

    im->resetIgnitionEvents();

    writeToSynthesizer();
    if (m_currentIteration % 8 == 0 && m_currentIteration > 0) {
        m_synthesizer.endInputBlock();
    }

    ++m_currentIteration;

    return true;
}

double Simulator::getTotalExhaustFlow() const {
    double totalFlow = 0.0;
    for (int i = 0; i < m_engine->getCylinderCount(); ++i) {
        totalFlow += m_engine->getChamber(i)->getLastTimestepExhaustFlow();
    }

    return totalFlow;
}

int Simulator::readAudioOutput(int samples, int16_t *target) {
    return m_synthesizer.readAudioOutput(samples, target);
}

void Simulator::endFrame() {
    const double frameTimestep = i_steps * getTimestep();
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < m_engine->getIntakeCount(); ++i) {
        m_engine->getIntake(i)->m_flowRate /= frameTimestep;
    }

    m_synthesizer.endInputBlock();
}

void Simulator::destroy() {
    delete[] m_crankConstraints;
    delete[] m_cylinderWallConstraints;
    delete[] m_linkConstraints;
    delete[] m_crankshaftLoads;
    delete m_system;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_crankshaftLoads = nullptr;
    m_system = nullptr;

    m_engine = nullptr;

    m_synthesizer.destroy();
}

CrankshaftLoad *Simulator::getCrankshaftLoad(int i) {
    return &m_crankshaftLoads[i];
}

void Simulator::setGear(int gear) {
    if (gear < 0 || gear >= 6) return;

    const double gearRatios[] = {
        2.97,
        2.07,
        1.43,
        1.00,
        0.84,
        0.56
    };

    const double m_car = units::mass(1597, units::kg);
    const double gear_ratio = gearRatios[gear];
    const double diff_ratio = 3.42;
    constexpr double tire_radius = units::distance(10, units::inch);
    const double f = tire_radius / (diff_ratio * gear_ratio);

    const double new_I = m_car * f * f;
    const double E_r = 0.5 * m_vehicleMass.I * m_vehicleMass.v_theta * m_vehicleMass.v_theta;
    const double new_v_theta = -std::sqrt(E_r * 2 / new_I);

    m_vehicleMass.I = new_I;
    m_vehicleMass.p_x = m_vehicleMass.p_y = 0;
    m_vehicleMass.m = m_car;
    m_vehicleMass.v_theta = new_v_theta;
}

void Simulator::setClutch(double pressure) {
    m_clutchConstraint.m_maxTorque = pressure * units::torque(1000.0, units::ft_lb);
    m_clutchConstraint.m_minTorque = -pressure * units::torque(1000.0, units::ft_lb);
}

void Simulator::initializeSynthesizer() {
    Synthesizer::Parameters synthParams;
    synthParams.AudioBufferSize = 44100;
    synthParams.AudioSampleRate = 44100;
    synthParams.InputBufferSize = 2048 * 2 * 2 * 2;
    synthParams.InputChannelCount = m_engine->getExhaustSystemCount();
    synthParams.InputSampleRate = (double)m_simulationFrequency;
    m_synthesizer.initialize(synthParams);

    m_exhaustFlowStagingBuffer = new double[m_engine->getExhaustSystemCount()];
}

void Simulator::writeToSynthesizer() {
    const int exhaustSystemCount = m_engine->getExhaustSystemCount();
    for (int i = 0; i < exhaustSystemCount; ++i) {
        m_exhaustFlowStagingBuffer[i] = 0;
    }

    const double timestep = getTimestep();
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        Piston *piston = m_engine->getPiston(i);
        CylinderBank *bank = piston->m_bank;
        CylinderHead *head = m_engine->getHead(bank->m_index);

        const double exhaustFlow =
            m_engine->getChamber(i)->getLastTimestepExhaustFlow();
        ExhaustSystem *exhaustSystem = head->m_exhaustSystems[piston->m_cylinderIndex];
        m_exhaustFlowStagingBuffer[exhaustSystem->m_index] += 50000 * exhaustFlow / timestep;
    }

    m_synthesizer.writeInput(m_exhaustFlowStagingBuffer);
}
