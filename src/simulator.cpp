#include "../include/simulator.h"

#include "../include/constants.h"
#include "../include/units.h"

#include <cmath>
#include <assert.h>
#include <chrono>

Simulator::Simulator() {
    m_engine = nullptr;
    m_transmission = nullptr;
    m_vehicle = nullptr;

    i_steps = 1;
    m_currentIteration = 0;
    m_simulationSpeed = 1.0;
    m_targetSynthesizerLatency = 0.1;
    m_simulationFrequency = 5000;//11000;//9000;//13000

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_crankshaftFrictionConstraints = nullptr;
    m_system = nullptr;

    m_physicsProcessingTime = 0.0;
    m_exhaustFlowStagingBuffer = nullptr;

    m_filteredEngineSpeed = 0.0;
    m_dynoTorque = 0.0;
}

Simulator::~Simulator() {
    assert(m_crankConstraints == nullptr);
    assert(m_cylinderWallConstraints == nullptr);
    assert(m_linkConstraints == nullptr);
    assert(m_crankshaftFrictionConstraints == nullptr);
    assert(m_system == nullptr);
    assert(m_exhaustFlowStagingBuffer == nullptr);
}

void Simulator::initialize(const Parameters &params) {
    if (params.SystemType == SystemType::NsvOptimized) {
        atg_scs::OptimizedNsvRigidBodySystem *system =
            new atg_scs::OptimizedNsvRigidBodySystem;
        system->initialize(
            new atg_scs::GaussSeidelSleSolver);
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

    m_engine = params.Engine;
    m_vehicle = params.Vehicle;
    m_transmission = params.Transmission;

    const int crankCount = m_engine->getCrankshaftCount();
    const int cylinderCount = m_engine->getCylinderCount();
    const int linkCount = cylinderCount * 2;

    m_crankConstraints = new atg_scs::FixedPositionConstraint[crankCount];
    m_cylinderWallConstraints = new atg_scs::LineConstraint[cylinderCount];
    m_linkConstraints = new atg_scs::LinkConstraint[linkCount];
    m_crankshaftFrictionConstraints = new atg_scs::RotationFrictionConstraint[crankCount];

    const double ks = 5000;
    const double kd = 10;

    for (int i = 0; i < crankCount; ++i) {
        Crankshaft *crankshaft = m_engine->getCrankshaft(i);

        m_crankConstraints[i].setBody(&crankshaft->m_body);
        m_crankConstraints[i].setWorldPosition(
                crankshaft->getPosX(),
                crankshaft->getPosY());
        m_crankConstraints[i].setLocalPosition(0.0, 0.0);
        m_crankConstraints[i].m_kd = kd;
        m_crankConstraints[i].m_ks = ks;

        crankshaft->m_body.p_x = crankshaft->getPosX();
        crankshaft->m_body.p_y = crankshaft->getPosY();
        crankshaft->m_body.theta = 0;
        crankshaft->m_body.m =
                crankshaft->getMass() + crankshaft->getFlywheelMass();
        crankshaft->m_body.I = crankshaft->getMomentOfInertia();

        m_crankshaftFrictionConstraints[i].m_minTorque = -crankshaft->getFrictionTorque();
        m_crankshaftFrictionConstraints[i].m_maxTorque = crankshaft->getFrictionTorque();
        m_crankshaftFrictionConstraints[i].setBody(&m_engine->getCrankshaft(i)->m_body);

        m_system->addRigidBody(&m_engine->getCrankshaft(i)->m_body);
        m_system->addConstraint(&m_crankConstraints[i]);
        m_system->addConstraint(&m_crankshaftFrictionConstraints[i]);
    }

    m_transmission->addToSystem(m_system, &m_vehicleMass, m_vehicle, m_engine);
    m_vehicle->addToSystem(m_system, &m_vehicleMass);

    m_vehicleMass.m = 1.0;
    m_vehicleMass.I = 1.0;
    m_system->addRigidBody(&m_vehicleMass);

    for (int i = 0; i < cylinderCount; ++i) {
        Piston *piston = m_engine->getPiston(i);
        ConnectingRod *connectingRod = piston->getRod();
        Crankshaft *crankshaft = connectingRod->getCrankshaft();

        CylinderBank *bank = piston->getCylinderBank();
        const double dx = std::cos(bank->getAngle() + constants::pi / 2);
        const double dy = std::sin(bank->getAngle() + constants::pi / 2);

        m_cylinderWallConstraints[i].setBody(&piston->m_body);
        m_cylinderWallConstraints[i].m_dx = dx;
        m_cylinderWallConstraints[i].m_dy = dy;
        m_cylinderWallConstraints[i].m_local_x = 0.0;
        m_cylinderWallConstraints[i].m_local_y = piston->getWristPinLocation();
        m_cylinderWallConstraints[i].m_p0_x = crankshaft->getPosX();
        m_cylinderWallConstraints[i].m_p0_y = crankshaft->getPosY();
        m_cylinderWallConstraints[i].m_ks = ks;
        m_cylinderWallConstraints[i].m_kd = kd;

        piston->setCylinderConstraint(&m_cylinderWallConstraints[i]);

        m_linkConstraints[i * 2 + 0].setBody1(&connectingRod->m_body);
        m_linkConstraints[i * 2 + 0].setBody2(&piston->m_body);
        m_linkConstraints[i * 2 + 0]
            .setLocalPosition1(0.0, connectingRod->getLittleEndLocal());
        m_linkConstraints[i * 2 + 0].setLocalPosition2(0.0, piston->getWristPinLocation());
        m_linkConstraints[i * 2 + 0].m_ks = ks;
        m_linkConstraints[i * 2 + 0].m_kd = kd;

        double journal_x = 0.0, journal_y = 0.0;
        crankshaft->getRodJournalPositionLocal(
                connectingRod->getJournal(),
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

        piston->m_body.m = piston->getMass();
        piston->m_body.I = 1.0;

        connectingRod->m_body.m = connectingRod->getMass();
        connectingRod->m_body.I = connectingRod->getMomentOfInertia();

        m_system->addRigidBody(&piston->m_body);
        m_system->addRigidBody(&connectingRod->m_body);
        m_system->addConstraint(&m_linkConstraints[i * 2 + 0]);
        m_system->addConstraint(&m_linkConstraints[i * 2 + 1]);
        m_system->addConstraint(&m_cylinderWallConstraints[i]);
        m_system->addForceGenerator(m_engine->getChamber(i));
    }

    m_dyno.connectCrankshaft(m_engine->getOutputCrankshaft());
    m_system->addConstraint(&m_dyno);

    m_starterMotor.connectCrankshaft(m_engine->getOutputCrankshaft());
    m_starterMotor.m_maxTorque = m_engine->getStarterTorque();
    m_starterMotor.m_rotationSpeed = -m_engine->getStarterSpeed();
    m_system->addConstraint(&m_starterMotor);

    placeAndInitialize();
    initializeSynthesizer();
}

void Simulator::placeAndInitialize() {
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        ConnectingRod *rod = m_engine->getConnectingRod(i);
        Piston *piston = m_engine->getPiston(i);
        CylinderBank *bank = piston->getCylinderBank();

        double p_x, p_y;
        rod->getCrankshaft()->getRodJournalPositionLocal(rod->getJournal(), &p_x, &p_y);

        // (bank->m_dx * s - p_x)^2 + (bank->m_dy * s - p_y)^2 = rod->m_length * rod->m_length

        const double a = bank->getDx() * bank->getDx() + bank->getDy() * bank->getDy();
        const double b = -2 * bank->getDx() * p_x - 2 * bank->getDy() * p_y;
        const double c = p_x * p_x + p_y * p_y - rod->getLength() * rod->getLength();

        const double det = b * b - 4 * a * c;
        if (det < 0) continue;

        const double sqrt_det = std::sqrt(det);
        const double s0 = (-b + sqrt_det) / (2 * a);
        const double s1 = (-b - sqrt_det) / (2 * a);

        const double s = std::max(s0, s1);
        if (s < 0) continue;

        const double e_x = s * bank->getDx();
        const double e_y = s * bank->getDy();

        const double theta = ((e_y - p_y) > 0)
            ? std::acos((e_x - p_x) / rod->getLength())
            : 2 * constants::pi - std::acos((e_x - p_x) / rod->getLength());
        rod->m_body.theta = theta - constants::pi / 2;

        double cl_x, cl_y;
        rod->m_body.localToWorld(0, rod->getBigEndLocal(), &cl_x, &cl_y);
        rod->m_body.p_x += p_x + rod->getCrankshaft()->getPosX() - cl_x;
        rod->m_body.p_y += p_y + rod->getCrankshaft()->getPosY() - cl_y;

        piston->m_body.p_x = e_x + rod->getCrankshaft()->getPosX();
        piston->m_body.p_y = e_y + rod->getCrankshaft()->getPosY();
        piston->m_body.theta = bank->getAngle() + constants::pi;
    }

    for (int i = 0; i < cylinderCount; ++i) {
        m_engine->getChamber(i)->m_system.initialize(
            units::pressure(1.0, units::atm),
            m_engine->getChamber(i)->getVolume(),
            units::celcius(25.0)
        );
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

    if (i_steps > 0) {
        for (int i = 0; i < m_engine->getIntakeCount(); ++i) {
            m_engine->getIntake(i)->m_flowRate = 0;
        }
    }
}

void Simulator::startAudioRenderingThread() {
    m_synthesizer.startAudioRenderingThread();
}

void Simulator::endAudioRenderingThread() {
    m_synthesizer.endAudioRenderingThread();
}

int Simulator::getSynthesizerInputLatencyTarget() const {
    return std::max(
        10,
        (int)std::ceil(m_targetSynthesizerLatency * m_simulationFrequency * m_simulationSpeed));
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

    m_dynoTorque = 0.99 * m_dynoTorque + 0.01 * m_dyno.getTorque();

    m_vehicle->update(timestep);
    m_transmission->update(timestep);

    updateFilteredEngineSpeed(timestep);

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
        m_engine->getChamber(i)->resetLastTimestepIntakeFlow();
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
    delete[] m_crankshaftFrictionConstraints;
    delete[] m_exhaustFlowStagingBuffer;
    delete m_system;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_crankshaftFrictionConstraints = nullptr;
    m_exhaustFlowStagingBuffer = nullptr;
    m_system = nullptr;

    m_engine = nullptr;

    m_synthesizer.destroy();
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

void Simulator::updateFilteredEngineSpeed(double dt) {
    const double alpha = dt / (100 + dt);
    m_filteredEngineSpeed = alpha * m_filteredEngineSpeed + (1 - alpha) * m_engine->getRpm();
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
        CylinderBank *bank = piston->getCylinderBank();
        CylinderHead *head = m_engine->getHead(bank->getIndex());

        const double exhaustFlow = m_engine->getChamber(i)->getLastTimestepExhaustFlow();
        const double attenuation = std::min(std::abs(m_filteredEngineSpeed), 1E-2) / 1E-2;

        ExhaustSystem *exhaustSystem = head->getExhaustSystem(piston->getCylinderIndex());
        m_exhaustFlowStagingBuffer[exhaustSystem->m_index] += attenuation * 50000 * exhaustFlow / timestep;
    }

    m_synthesizer.writeInput(m_exhaustFlowStagingBuffer);
}
