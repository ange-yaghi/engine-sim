#include "..\include\engine.h"
#include "../include/engine.h"

#include "../include/constants.h"
#include "../include/units.h"
#include "../include/fuel.h"
#include "../include/piston_engine_simulator.h"
#include "../include/utilities.h"

#include <cmath>
#include <assert.h>

Engine::Engine() {
    m_name = "";

    m_crankshafts = nullptr;
    m_cylinderBanks = nullptr;
    m_heads = nullptr;
    m_pistons = nullptr;
    m_connectingRods = nullptr;
    m_exhaustSystems = nullptr;
    m_intakes = nullptr;
    m_turbochargers = nullptr;
    m_superchargers = nullptr;
    m_combustionChambers = nullptr;

    m_crankshaftCount = 0;
    m_cylinderBankCount = 0;
    m_cylinderCount = 0;
    m_intakeCount = 0;
    m_exhaustSystemCount = 0;
    m_turbochargerCount = 0;
    m_superchargerCount = 0;
    m_starterSpeed = 0;
    m_starterTorque = 0;
    m_dynoMinSpeed = 0;
    m_dynoMaxSpeed = 0;
    m_dynoHoldStep = 0;
    m_redline = 0;

    m_throttle = nullptr;
    m_throttleValue = 0.0;

    m_initialSimulationFrequency = 10000.0;
    m_initialHighFrequencyGain = 0.01;
    m_initialJitter = 0.5;
    m_initialNoise = 1.0;
}

Engine::~Engine() {
    assert(m_crankshafts == nullptr);
    assert(m_cylinderBanks == nullptr);
    assert(m_pistons == nullptr);
    assert(m_connectingRods == nullptr);
    assert(m_heads == nullptr);
    assert(m_exhaustSystems == nullptr);
    assert(m_intakes == nullptr);
    assert(m_turbochargers == nullptr);
    assert(m_superchargers == nullptr);
    assert(m_combustionChambers == nullptr);
}

void Engine::initialize(const Parameters &params) {
    m_crankshaftCount = params.crankshaftCount;
    m_cylinderCount = params.cylinderCount;
    m_cylinderBankCount = params.cylinderBanks;
    m_exhaustSystemCount = params.exhaustSystemCount;
    m_intakeCount = params.intakeCount;
    m_turbochargerCount = params.turbochargerCount;
    m_superchargerCount = params.superchargerCount;
    m_starterTorque = params.starterTorque;
    m_starterSpeed = params.starterSpeed;
    m_dynoMinSpeed = params.dynoMinSpeed;
    m_dynoMaxSpeed = params.dynoMaxSpeed;
    m_dynoHoldStep = params.dynoHoldStep;
    m_redline = params.redline;
    m_name = params.name;
    m_throttle = params.throttle;
    m_initialHighFrequencyGain = params.initialHighFrequencyGain;
    m_initialSimulationFrequency = params.initialSimulationFrequency;
    m_initialJitter = params.initialJitter;
    m_initialNoise = params.initialNoise;
    m_antiLagEnabled = params.antiLagEnabledByDefault;
    m_antiLagMaxRetard = params.antiLagMaxRetard;
    m_antiLagDumpRateK = params.antiLagDumpRateK;
    m_antiLagPopRate = params.antiLagPopRate;

    m_crankshafts = new Crankshaft[m_crankshaftCount];
    m_cylinderBanks = new CylinderBank[m_cylinderBankCount];
    m_heads = new CylinderHead[m_cylinderBankCount];
    m_pistons = new Piston[m_cylinderCount];
    m_connectingRods = new ConnectingRod[m_cylinderCount];
    m_exhaustSystems = new ExhaustSystem[m_exhaustSystemCount];
    m_intakes = new Intake[m_intakeCount];
    m_turbochargers = new Turbocharger[m_turbochargerCount];
    m_superchargers = new Supercharger[m_superchargerCount];
    m_combustionChambers = new CombustionChamber[m_cylinderCount];

    for (int i = 0; i < m_exhaustSystemCount; ++i) {
        m_exhaustSystems[i].m_index = i;
    }

    for (int i = 0; i < m_intakeCount; ++i) {
        m_intakes[i].m_index = i;
    }

    for (int i = 0; i < m_cylinderCount; ++i) {
        m_combustionChambers[i].setEngine(this);
    }
}

void Engine::destroy() {
    for (int i = 0; i < m_crankshaftCount; ++i) {
        m_crankshafts[i].destroy();
    }

    for (int i = 0; i < m_cylinderCount; ++i) {
        m_pistons[i].destroy();
        m_connectingRods[i].destroy();
        m_combustionChambers[i].destroy();
    }

    for (int i = 0; i < m_exhaustSystemCount; ++i) {
        m_exhaustSystems[i].destroy();
    }

    for (int i = 0; i < m_intakeCount; ++i) {
        m_intakes[i].destroy();
    }

    for (int i = 0; i < m_turbochargerCount; ++i) {
        m_turbochargers[i].destroy();
    }

    for (int i = 0; i < m_superchargerCount; ++i) {
        m_superchargers[i].destroy();
    }

    m_ignitionModule.destroy();

    if (m_throttle != nullptr) delete m_throttle;
    if (m_crankshafts != nullptr) delete[] m_crankshafts;
    if (m_cylinderBanks != nullptr) delete[] m_cylinderBanks;
    if (m_heads != nullptr) delete[] m_heads;
    if (m_pistons != nullptr) delete[] m_pistons;
    if (m_connectingRods != nullptr) delete[] m_connectingRods;
    if (m_exhaustSystems != nullptr) delete[] m_exhaustSystems;
    if (m_intakes != nullptr) delete[] m_intakes;
    if (m_turbochargers != nullptr) delete[] m_turbochargers;
    if (m_superchargers != nullptr) delete[] m_superchargers;
    if (m_combustionChambers != nullptr) delete[] m_combustionChambers;

    m_crankshafts = nullptr;
    m_cylinderBanks = nullptr;
    m_pistons = nullptr;
    m_connectingRods = nullptr;
    m_heads = nullptr;
    m_exhaustSystems = nullptr;
    m_intakes = nullptr;
    m_turbochargers = nullptr;
    m_superchargers = nullptr;
    m_combustionChambers = nullptr;
    m_throttle = nullptr;
}

Crankshaft *Engine::getOutputCrankshaft() const {
    return &m_crankshafts[0];
}

void Engine::setSpeedControl(double s) {
    m_throttle->setSpeedControl(s);
}

double Engine::getSpeedControl() {
    return m_throttle->getSpeedControl();
}

void Engine::setThrottle(double throttle) {
    for (int i = 0; i < m_intakeCount; ++i) {
        m_intakes[i].m_throttle = throttle;
    }

    m_throttleValue = throttle;
}

double Engine::getThrottle() const {
    return m_throttleValue;
}

double Engine::getThrottlePlateAngle() const {
    return (1 - m_intakes[0].getThrottlePlatePosition()) * (constants::pi / 2);
}

bool placeRod(
    const ConnectingRod &rod,
    const CylinderBank &bank,
    const Crankshaft &crankshaft,
    double crankshaftAngle,
    double *p_x,
    double *p_y,
    double *theta,
    double *s)
{
    double p_x_0, p_y_0, l_x, l_y, theta_0;
    if (rod.getMasterRod() != nullptr) {
        double s;
        const bool succeeded = placeRod(
            *rod.getMasterRod(),
            *rod.getMasterRod()->getPiston()->getCylinderBank(),
            *rod.getCrankshaft(),
            crankshaftAngle,
            &p_x_0,
            &p_y_0,
            &theta_0,
            &s);

        if (!succeeded) {
            return false;
        }

        rod.getMasterRod()->getRodJournalPositionLocal(rod.getPiston()->getCylinderIndex(), &l_x, &l_y);
    }
    else {
        theta_0 = crankshaftAngle;
        p_x_0 = rod.getCrankshaft()->getPosX();
        p_y_0 = rod.getCrankshaft()->getPosY();
        rod.getCrankshaft()->getRodJournalPositionLocal(rod.getPiston()->getCylinderIndex(), &l_x, &l_y);
    }

    const double dx = std::cos(theta_0);
    const double dy = std::sin(theta_0);
    *p_x = p_x_0 + (dx * l_x - dy * l_y);
    *p_y = p_y_0 + (dy * l_x + dx * l_y);

    // (bank->m_x + bank->m_dx * s - p_x)^2 + (bank->m_y + bank->m_dy * s - p_y)^2 = (rod->m_length)^2
    const double a = bank.getDx() * bank.getDx() + bank.getDy() * bank.getDy();
    const double b = -2 * bank.getDx() * ((*p_x) - bank.getX()) - 2 * bank.getDy() * ((*p_y) - bank.getY());
    const double c =
        ((*p_x) - bank.getX()) * ((*p_x) - bank.getX())
        + ((*p_y) - bank.getY()) * ((*p_y) - bank.getY())
        - rod.getLength() * rod.getLength();

    const double det = b * b - 4 * a * c;
    if (det < 0) return false;

    const double sqrt_det = std::sqrt(det);
    const double s0 = (-b + sqrt_det) / (2 * a);
    const double s1 = (-b - sqrt_det) / (2 * a);

    *s = std::max(s0, s1);
    if (*s < 0) return false;
   
    if (s != nullptr) {
        const double dx = (bank.getX() + bank.getDx() * (*s)) - (*p_x);
        const double dy = (bank.getY() + bank.getDy() * (*s)) - (*p_y);

        *theta = (dy > 0)
            ? std::acos(dx)
            : -std::acos(dx);
    }

    return true;
}

void Engine::calculateDisplacement() {
    // There is a closed-form/correct way to do this which I really
    // don't feel like deriving right now, so I'm just going with this
    // numerical approximation.
    constexpr int Resolution = 1000;

    double *min_s = new double[m_cylinderCount];
    double *max_s = new double[m_cylinderCount];

    for (int i = 0; i < m_cylinderCount; ++i) {
        min_s[i] = DBL_MAX;
        max_s[i] = -DBL_MAX;
    }

    for (int j = 0; j < Resolution; ++j) {
        const double crankshaftAngle = 2 * (j / static_cast<double>(Resolution)) * constants::pi;

        for (int i = 0; i < m_cylinderCount; ++i) {
            const Piston &piston = m_pistons[i];
            const CylinderBank &bank = *piston.getCylinderBank();
            const ConnectingRod &rod = *piston.getRod();
            const Crankshaft &shaft = *rod.getCrankshaft();

            double p_x, p_y;
            double theta;
            double s;
            if (!placeRod(
                rod,
                bank,
                shaft,
                crankshaftAngle,
                &p_x,
                &p_y,
                &theta,
                &s))
            {
                continue;
            }

            min_s[i] = std::min(min_s[i], s);
            max_s[i] = std::max(max_s[i], s);
        }
    }

    double displacement = 0;
    for (int i = 0; i < m_cylinderCount; ++i) {
        const Piston &piston = m_pistons[i];
        const CylinderBank &bank = *piston.getCylinderBank();

        if (min_s[i] < max_s[i]) {
            const double r = bank.getBore() / 2.0;
            displacement += constants::pi * r * r * (max_s[i] - min_s[i]);
        }
    }

    m_displacement = displacement;
}

double Engine::getIntakeFlowRate() const {
    double airIntake = 0;
    for (int i = 0; i < m_intakeCount; ++i) {
        airIntake += m_intakes[i].m_flowRate;
    }

    return airIntake;
}

void Engine::update(double dt) {
    m_throttle->update(dt, this);
    updateAntiLag(dt);
}

void Engine::updateAntiLag(double dt) {
    if (!m_antiLagEnabled || !hasTurbocharger() || !m_ignitionModule.m_enabled) {
        m_antiLagActive = false;
        m_antiLagLevel = 0.0;
        m_ignitionModule.setTimingRetard(0.0);
        return;
    }

    // Instantaneous (unsmoothed) safety factor: 0 at/below idleFloor, ramping
    // to 1 over the next taperBand above it. This has to react immediately
    // to falling RPM -- it's what stops anti-lag from popping the engine
    // all the way down to a stall. It's kept separate from m_antiLagLevel's
    // slow release below, which exists for a different reason (carrying
    // boost through a deliberate throttle tip-in) and would otherwise keep
    // full retard applied for up to half a second after RPM already
    // dropped into stalling territory.
    // m_redline is stored in rad/s (see units::rpm()); getRpm() returns
    // actual RPM. idleFloor/taperBand have to be in the same units as
    // whatever they're compared against below, or this "safety floor"
    // collapses to a near-zero RPM threshold instead of ~28% of redline --
    // full retard would then apply from the moment the starter first spins
    // the engine, before it's ever caught, killing every start attempt.
    const double redlineRpm = units::toRpm(m_redline);
    const double idleFloor = redlineRpm * 0.28;
    const double taperBand = redlineRpm * 0.12;
    const double rpmFactor = clamp((getRpm() - idleFloor) / taperBand, 0.0, 1.0);

    // getThrottle(): 0 = plate wide open, 1 = plate closed (see setThrottle()).
    // Off-throttle anti-lag only kicks in when the pedal is essentially all
    // the way up (99%+ closed), not on an ordinary partial lift.
    const double throttle = getThrottle();
    const bool offThrottle = throttle > 0.99;

    // Attack almost instantly (catch the lift immediately); release over
    // half a second (carry the charge through the tip-in) -- see the level's
    // declaration in engine.h for why a hard cutoff here is wrong.
    const double attackRate = 1.0 / 0.05;
    const double releaseRate = 1.0 / 0.5;
    const double target = offThrottle ? 1.0 : 0.0;
    if (target > m_antiLagLevel) {
        m_antiLagLevel = std::min(target, m_antiLagLevel + attackRate * dt);
    }
    else {
        m_antiLagLevel = std::max(target, m_antiLagLevel - releaseRate * dt);
    }

    const double effectiveLevel = m_antiLagLevel * rpmFactor;
    m_ignitionModule.setTimingRetard(m_antiLagMaxRetard * effectiveLevel);
    m_antiLagActive = effectiveLevel > 0.05;

    if (effectiveLevel <= 0.0) return;

    // Dump a rich raw charge straight into each header (standing in for a
    // bypass air/fuel valve) and randomly touch off a small chunk of
    // whatever's sitting in there -- an irregular, per-cylinder crackle
    // rather than a metronomic one. Both the dump rate and pop chance scale
    // with the level above, so this fades out with it instead of stopping
    // cold. This deliberately does *not* also poke the downstream exhaust
    // collector directly -- the pop happens here, in the header, and is
    // left to travel down to the collector (and turbine) through the exact
    // same header->collector flow constant a real combustion event uses, so
    // it's shaped and delayed the same way instead of teleporting energy in
    // two places at once. Likewise the resulting pressure/energy spike in
    // m_exhaustRunnerAndPrimary is picked up by the existing exhaust audio
    // synthesis with no further wiring (see
    // PistonEngineSimulator::writeToSynthesizer()) -- kept deliberately
    // gentle per-pop since pops repeat several times a second and stack; a
    // real-combustion-sized release here reads as deafening.
    //
    // m_antiLagDumpRateK has a real, fairly low ceiling: continuously
    // holding all six headers near dumpPressure for a couple of sustained
    // seconds (e.g. a hard lift from redline down through idle, with
    // effectiveLevel pinned near 1.0 the whole way) measurably loads the
    // exhaust side -- extra pumping losses on top of ordinary closed-
    // throttle engine braking -- and past a fairly low threshold that's
    // enough extra drag to stall the engine outright before it ever
    // reaches idle. Verified empirically (engine-sim-check's dyno test
    // does exactly this coast-down): the shared default below (6.0,
    // unchanged from what this system always used pre-DSL-exposure) is
    // well past that threshold, but nothing ever exercised it long/hard
    // enough to notice before this was exposed as a per-engine .mr
    // setting. 0.3 already reproduces the stall; 0.1 does not. Keep any
    // engine's anti_lag_dump_rate comfortably under ~0.2 unless you've
    // re-verified against a sustained high-RPM-to-idle coast yourself.
    GasSystem::Mix richMix;
    richMix.p_fuel = 0.20;
    richMix.p_o2 = 0.19;
    richMix.p_inert = 0.61;

    const double dumpK = GasSystem::k_carb(m_antiLagDumpRateK) * effectiveLevel;
    const double dumpPressure = units::pressure(2.5, units::atm);
    const double dumpTemperature = units::celcius(500.0);

    for (int i = 0; i < m_cylinderCount; ++i) {
        CombustionChamber &chamber = m_combustionChambers[i];
        GasSystem &header = chamber.m_exhaustRunnerAndPrimary;
        const double dumpFlow = header.flow(dumpK, dt, dumpPressure, dumpTemperature, richMix);

        // Same accounting a real exhaust event uses -- see
        // CombustionChamber::addExternalExhaustFlow() -- so this dump shows
        // up in PistonEngineSimulator::getTotalExhaustFlow() like any other
        // exhaust flow instead of being invisible to it.
        chamber.addExternalExhaustFlow(std::abs(dumpFlow));

        if (header.n_fuel() > 0.0
            && ((double)rand() / RAND_MAX) < dt * m_antiLagPopRate * effectiveLevel)
        {
            const double fuelBurned = header.react(header.n() * 0.1, header.mix());
            header.changeEnergy(fuelBurned * m_fuel.getMolecularMass() * m_fuel.getEnergyDensity());
        }
    }
}

double Engine::getManifoldPressure() const {
    double pressureSum = 0.0;
    for (int i = 0; i < m_intakeCount; ++i) {
        pressureSum += m_intakes[i].m_system.pressure();
    }

    return pressureSum / m_intakeCount;
}

double Engine::getIntakeAfr() const {
    double totalOxygen = 0.0;
    double totalFuel = 0.0;
    for (int i = 0; i < m_intakeCount; ++i) {
        totalOxygen += m_intakes[i].m_system.n_o2();
        totalFuel += m_intakes[i].m_system.n_fuel();
    }

    constexpr double octaneMolarMass = units::mass(114.23, units::g);
    constexpr double oxygenMolarMass = units::mass(31.9988, units::g);

    if (totalFuel == 0) return 0;
    else {
        return
            (oxygenMolarMass * totalOxygen / 0.21)
            / (totalFuel * octaneMolarMass);
    }
}

double Engine::getExhaustO2() const {
    double totalInert = 0.0;
    double totalOxygen = 0.0;
    double totalFuel = 0.0;
    for (int i = 0; i < m_exhaustSystemCount; ++i) {
        totalInert += m_exhaustSystems[i].m_system.n_inert();
        totalOxygen += m_exhaustSystems[i].m_system.n_o2();
        totalFuel += m_exhaustSystems[i].m_system.n_fuel();
    }

    constexpr double octaneMolarMass = units::mass(114.23, units::g);
    constexpr double oxygenMolarMass = units::mass(31.9988, units::g);
    constexpr double nitrogenMolarMass = units::mass(28.014, units::g);

    if (totalFuel == 0) return 0;
    else {
        return
            (oxygenMolarMass * totalOxygen)
            / (
                totalFuel * octaneMolarMass
                + nitrogenMolarMass * totalInert
                + oxygenMolarMass * totalOxygen);
    }
}

void Engine::resetFuelConsumption() {
    for (int i = 0; i < m_intakeCount; ++i) {
        m_intakes[i].m_totalFuelInjected = 0;
    }
}

double Engine::getTotalFuelMassConsumed() const {
    double n_fuelConsumed = 0;
    for (int i = 0; i < m_intakeCount; ++i) {
        n_fuelConsumed += m_intakes[i].m_totalFuelInjected;
    }

    return n_fuelConsumed * m_fuel.getMolecularMass();
}

double Engine::getTotalVolumeFuelConsumed() const {
    return getTotalFuelMassConsumed() / m_fuel.getDensity();
}

int Engine::getMaxDepth() const {
    int maxDepth = 0;
    for (int i = 0; i < m_crankshaftCount; ++i) {
        maxDepth = std::max(m_crankshafts[i].getRodJournalCount(), maxDepth);
    }

    return maxDepth;
}

Simulator *Engine::createSimulator(Vehicle *vehicle, Transmission *transmission) {
    PistonEngineSimulator *simulator = new PistonEngineSimulator;
    Simulator::Parameters simulatorParams;
    simulatorParams.systemType = Simulator::SystemType::NsvOptimized;
    simulator->initialize(simulatorParams);

    simulator->loadSimulation(this, vehicle, transmission);
    simulator->setFluidSimulationSteps(8);

    return static_cast<Simulator *>(simulator);
}

double Engine::getRpm() const {
    if (m_crankshaftCount == 0) return 0;
    else return std::abs(units::toRpm(getCrankshaft(0)->m_body.v_theta));
}

double Engine::getSpeed() const {
    if (m_crankshaftCount == 0) return 0;
    else return std::abs(getCrankshaft(0)->m_body.v_theta);
}

bool Engine::isSpinningCw() const {
    return getOutputCrankshaft()->m_body.v_theta <= 0;
}
