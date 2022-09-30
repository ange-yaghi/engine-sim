#include "../include/piston_engine_simulator.h"

#include "../include/constants.h"
#include "../include/units.h"

#include <cmath>
#include <assert.h>
#include <chrono>
#include <set>

PistonEngineSimulator::PistonEngineSimulator() {
    m_engine = nullptr;
    m_transmission = nullptr;
    m_vehicle = nullptr;
    m_delayFilters = nullptr;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_crankshaftFrictionConstraints = nullptr;
    m_crankshaftLinks = nullptr;

    m_exhaustFlowStagingBuffer = nullptr;

    m_derivativeFilter.m_dt = 1.0;
    m_fluidSimulationSteps = 8;
}

PistonEngineSimulator::~PistonEngineSimulator() {
    assert(m_crankConstraints == nullptr);
    assert(m_cylinderWallConstraints == nullptr);
    assert(m_linkConstraints == nullptr);
    assert(m_crankshaftFrictionConstraints == nullptr);
    assert(m_exhaustFlowStagingBuffer == nullptr);
    assert(m_delayFilters == nullptr);
}

void PistonEngineSimulator::loadSimulation(Engine *engine, Vehicle *vehicle, Transmission *transmission) {
    Simulator::loadSimulation(engine, vehicle, transmission);

    m_engine = engine;
    m_vehicle = vehicle;
    m_transmission = transmission;

    const int crankCount = m_engine->getCrankshaftCount();
    const int cylinderCount = m_engine->getCylinderCount();
    const int linkCount = cylinderCount * 2;

    if (crankCount <= 0) return;

    m_crankConstraints = new atg_scs::FixedPositionConstraint[crankCount];
    m_cylinderWallConstraints = new atg_scs::LineConstraint[cylinderCount];
    m_linkConstraints = new atg_scs::LinkConstraint[linkCount];
    m_crankshaftFrictionConstraints = new atg_scs::RotationFrictionConstraint[crankCount];
    m_crankshaftLinks = new atg_scs::ClutchConstraint[crankCount - 1];
    m_delayFilters = new DelayFilter[cylinderCount];

    const double ks = 5000;
    const double kd = 10;

    for (int i = 0; i < crankCount; ++i) {
        Crankshaft *outputShaft = m_engine->getCrankshaft(0);
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

        if (crankshaft != outputShaft) {
            atg_scs::ClutchConstraint *crankLink = &m_crankshaftLinks[i - 1];
            crankLink->setBody1(&outputShaft->m_body);
            crankLink->setBody2(&crankshaft->m_body);

            m_system->addConstraint(crankLink);
        }
    }

    m_transmission->addToSystem(m_system, &m_vehicleMass, m_vehicle, m_engine);
    m_vehicle->addToSystem(m_system, &m_vehicleMass);

    m_vehicleDrag.initialize(&m_vehicleMass, m_vehicle);
    m_system->addConstraint(&m_vehicleDrag);

    m_vehicleMass.reset();
    m_vehicleMass.m = 1.0;
    m_vehicleMass.I = 1.0;
    m_system->addRigidBody(&m_vehicleMass);

    for (int i = 0; i < cylinderCount; ++i) {
        Piston *piston = m_engine->getPiston(i);
        ConnectingRod *connectingRod = piston->getRod();

        CylinderBank *bank = piston->getCylinderBank();
        const double dx = std::cos(bank->getAngle() + constants::pi / 2);
        const double dy = std::sin(bank->getAngle() + constants::pi / 2);

        m_cylinderWallConstraints[i].setBody(&piston->m_body);
        m_cylinderWallConstraints[i].m_dx = dx;
        m_cylinderWallConstraints[i].m_dy = dy;
        m_cylinderWallConstraints[i].m_local_x = 0.0;
        m_cylinderWallConstraints[i].m_local_y = piston->getWristPinLocation();
        m_cylinderWallConstraints[i].m_p0_x = bank->getX();
        m_cylinderWallConstraints[i].m_p0_y = bank->getY();
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
        if (connectingRod->getMasterRod() == nullptr) {
            Crankshaft *crankshaft = connectingRod->getCrankshaft();
            crankshaft->getRodJournalPositionLocal(
                connectingRod->getJournal(),
                &journal_x,
                &journal_y);
            m_linkConstraints[i * 2 + 1].setBody2(&crankshaft->m_body);
        }
        else {
            connectingRod->getMasterRod()->getRodJournalPositionLocal(
                connectingRod->getJournal(),
                &journal_x,
                &journal_y);
            m_linkConstraints[i * 2 + 1].setBody2(&connectingRod->getMasterRod()->m_body);
        }

        m_linkConstraints[i * 2 + 1].setBody1(&connectingRod->m_body);
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

double PistonEngineSimulator::getAverageOutputSignal() const {
    double sum = 0.0;
    for (int i = 0; i < m_engine->getExhaustSystemCount(); ++i) {
        sum += m_engine->getExhaustSystem(i)->getSystem()->pressure();
    }

    return sum / m_engine->getExhaustSystemCount();
}

void PistonEngineSimulator::placeAndInitialize() {
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        ConnectingRod *rod = m_engine->getConnectingRod(i);

        if (rod->getRodJournalCount() != 0) {
            placeCylinder(i);
        }
    }

    for (int i = 0; i < cylinderCount; ++i) {
        placeCylinder(i);
    }

    for (int i = 0; i < cylinderCount; ++i) {
        m_engine->getChamber(i)->m_system.initialize(
            units::pressure(1.0, units::atm),
            m_engine->getChamber(i)->getVolume(),
            units::celcius(25.0)
        );

        Piston *piston = m_engine->getChamber(i)->getPiston();
        CylinderHead *head = m_engine->getChamber(i)->getCylinderHead();
        ExhaustSystem *exhaust = head->getExhaustSystem(piston->getCylinderIndex());
        const double exhaustLength =
            head->getHeaderPrimaryLength(piston->getCylinderIndex())
            + exhaust->getLength();
        const double speedOfSound = 343.0 * units::m / units::sec;
        const double delay = exhaustLength / speedOfSound;
        m_delayFilters[i].initialize(delay, 10000.0);
    }

    m_engine->getIgnitionModule()->reset();

    m_exhaustFlowStagingBuffer = new double[m_engine->getExhaustSystemCount()];
}

void PistonEngineSimulator::placeCylinder(int i) {
    ConnectingRod *rod = m_engine->getConnectingRod(i);
    Piston *piston = m_engine->getPiston(i);
    CylinderBank *bank = piston->getCylinderBank();

    double p_x, p_y;
    if (rod->getMasterRod() != nullptr) {
        rod->getMasterRod()->getRodJournalPositionGlobal(rod->getJournal(), &p_x, &p_y);
    }
    else {
        rod->getCrankshaft()->getRodJournalPositionGlobal(rod->getJournal(), &p_x, &p_y);
    }

    // (bank->m_x + bank->m_dx * s - p_x)^2 + (bank->m_y + bank->m_dy * s - p_y)^2 = (rod->m_length)^2
    const double a = bank->getDx() * bank->getDx() + bank->getDy() * bank->getDy();
    const double b = -2 * bank->getDx() * (p_x - bank->getX()) - 2 * bank->getDy() * (p_y - bank->getY());
    const double c =
        (p_x - bank->getX()) * (p_x - bank->getX())
        + (p_y - bank->getY()) * (p_y - bank->getY())
        - rod->getLength() * rod->getLength();

    const double det = b * b - 4 * a * c;
    if (det < 0) return;

    const double sqrt_det = std::sqrt(det);
    const double s0 = (-b + sqrt_det) / (2 * a);
    const double s1 = (-b - sqrt_det) / (2 * a);

    const double s = std::max(s0, s1);
    if (s < 0) return;

    const double e_x = s * bank->getDx() + bank->getX();
    const double e_y = s * bank->getDy() + bank->getY();

    const double theta = ((e_y - p_y) > 0)
        ? std::acos((e_x - p_x) / rod->getLength())
        : 2 * constants::pi - std::acos((e_x - p_x) / rod->getLength());
    rod->m_body.theta = theta - constants::pi / 2;

    double cl_x, cl_y;
    rod->m_body.localToWorld(0, rod->getBigEndLocal(), &cl_x, &cl_y);
    rod->m_body.p_x += p_x - cl_x;
    rod->m_body.p_y += p_y - cl_y;

    piston->m_body.p_x = e_x;
    piston->m_body.p_y = e_y;
    piston->m_body.theta = bank->getAngle() + constants::pi;
}

void PistonEngineSimulator::simulateStep_() {
    const double timestep = getTimestep();
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

    const int exhaustSystemCount = m_engine->getExhaustSystemCount();
    const int intakeCount = m_engine->getIntakeCount();
    const double fluidTimestep = timestep / m_fluidSimulationSteps;
    for (int i = 0; i < m_fluidSimulationSteps; ++i) {
        for (int j = 0; j < exhaustSystemCount; ++j) {
            m_engine->getExhaustSystem(j)->process(fluidTimestep);
        }

        for (int j = 0; j < intakeCount; ++j) {
            m_engine->getIntake(j)->process(fluidTimestep);
            m_engine->getIntake(j)->m_flowRate += m_engine->getIntake(j)->m_flow;
        }

        for (int j = 0; j < cylinderCount; ++j) {
            m_engine->getChamber(j)->flow(fluidTimestep);
        }
    }

    im->resetIgnitionEvents();
}

double PistonEngineSimulator::getTotalExhaustFlow() const {
    double totalFlow = 0.0;
    for (int i = 0; i < m_engine->getCylinderCount(); ++i) {
        totalFlow += m_engine->getChamber(i)->getLastTimestepExhaustFlow();
    }

    return totalFlow;
}

void PistonEngineSimulator::endFrame() {
    Simulator::endFrame();

    if (m_engine == nullptr) {
        return;
    }

    const double frameTimestep = simulationSteps() * getTimestep();
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < m_engine->getIntakeCount(); ++i) {
        m_engine->getIntake(i)->m_flowRate /= frameTimestep;
    }
}

void PistonEngineSimulator::destroy() {
    if (m_system != nullptr) m_system->reset();

    if (m_crankConstraints != nullptr) delete[] m_crankConstraints;
    if (m_cylinderWallConstraints != nullptr) delete[] m_cylinderWallConstraints;
    if (m_linkConstraints != nullptr) delete[] m_linkConstraints;
    if (m_crankshaftFrictionConstraints != nullptr) delete[] m_crankshaftFrictionConstraints;
    if (m_exhaustFlowStagingBuffer != nullptr) delete[] m_exhaustFlowStagingBuffer;
    if (m_system != nullptr) delete m_system;
    if (m_delayFilters != nullptr) delete[] m_delayFilters;

    m_crankConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_crankshaftFrictionConstraints = nullptr;
    m_exhaustFlowStagingBuffer = nullptr;
    m_system = nullptr;

    m_vehicle = nullptr;
    m_transmission = nullptr;
    m_engine = nullptr;
    m_delayFilters = nullptr;
}

void PistonEngineSimulator::writeToSynthesizer() {
    const int exhaustSystemCount = m_engine->getExhaustSystemCount();
    for (int i = 0; i < exhaustSystemCount; ++i) {
        m_exhaustFlowStagingBuffer[i] = 0;
    }

    const double attenuation = std::min(std::abs(filteredEngineSpeed()), 40.0) / 40.0;
    const double attenuation_3 = attenuation * attenuation * attenuation;

    static double lastValveLift[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    const double timestep = getTimestep();
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        Piston *piston = m_engine->getPiston(i);
        CylinderBank *bank = piston->getCylinderBank();
        CylinderHead *head = m_engine->getHead(bank->getIndex());
        ExhaustSystem *exhaust = head->getExhaustSystem(piston->getCylinderIndex());
        CombustionChamber *chamber = m_engine->getChamber(i);

        const double exhaustLength =
            head->getHeaderPrimaryLength(piston->getCylinderIndex())
            + exhaust->getLength();

        double exhaustFlow =
            attenuation_3 * 1600 * (
                1.0 * (chamber->m_exhaustRunnerAndPrimary.pressure() - units::pressure(1.0, units::atm))
                + 0.1 * chamber->m_exhaustRunnerAndPrimary.dynamicPressure(1.0, 0.0)
                + 0.1 * chamber->m_exhaustRunnerAndPrimary.dynamicPressure(-1.0, 0.0));

        lastValveLift[i] = head->exhaustValveLift(piston->getCylinderIndex());

        const double delayedExhaustPulse =
            m_delayFilters[i].fast_f(exhaustFlow);

        ExhaustSystem *exhaustSystem = head->getExhaustSystem(piston->getCylinderIndex());
        m_exhaustFlowStagingBuffer[exhaustSystem->getIndex()] +=
            head->getSoundAttenuation(piston->getCylinderIndex())
            * (exhaustSystem->getAudioVolume() * delayedExhaustPulse / cylinderCount)
            * (1 / (exhaustLength * exhaustLength));
    }

    synthesizer().writeInput(m_exhaustFlowStagingBuffer);
}
