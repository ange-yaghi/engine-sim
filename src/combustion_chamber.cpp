#include "../include/combustion_chamber.h"

#include "../include/constants.h"
#include "../include/units.h"
#include "../include/piston.h"
#include "../include/connecting_rod.h"
#include "../include/utilities.h"
#include "../include/exhaust_system.h"
#include "../include/cylinder_bank.h"
#include "../include/engine.h"

#include <cmath>

CombustionChamber::CombustionChamber() {
    m_crankcasePressure = 0.0;
    m_piston = nullptr;
    m_head = nullptr;
    m_lit = false;
    m_litLastFrame = false;
    m_peakTemperature = 0;

    m_meanPistonSpeedToTurbulence = nullptr;
    m_nBurntFuel = 0;

    m_lastTimestepTotalExhaustFlow = 0;
    m_exhaustFlow = 0;
    m_exhaustFlowRate = 0;
    m_intakeFlowRate = 0;

    m_fuel = nullptr;

    // temp
    m_intakeVelocity = 0;
    m_exhaustVelocity = 0;
}

CombustionChamber::~CombustionChamber() {
    /* void */
}

void CombustionChamber::initialize(const Parameters &params) {
    m_piston = params.Piston;
    m_head = params.Head;
    m_fuel = params.Fuel;
    m_crankcasePressure = params.CrankcasePressure;
    m_meanPistonSpeedToTurbulence = params.MeanPistonSpeedToTurbulence;

    m_pistonSpeed = new double[StateSamples];
    m_pressure = new double[StateSamples];
    for (int i = 0; i < StateSamples; ++i) {
        m_pistonSpeed[i] = 0;
        m_pressure[i] = 0;
    }
}

double CombustionChamber::getVolume() const {
    const double combustionPortVolume = m_head->getCombustionChamberVolume();
    const CylinderBank *bank = m_head->getCylinderBank();

    const double area = bank->boreSurfaceArea();
    const double s =
        m_piston->relativeX() * bank->getDx()
        + m_piston->relativeY() * bank->getDy();
    const double displacement =
        area * (bank->getDeckHeight() - s - m_piston->getCompressionHeight());

    return displacement + combustionPortVolume;
}

double CombustionChamber::pistonSpeed() const {
    const CylinderBank *bank = m_head->getCylinderBank();
    return
        m_piston->m_body.v_x * bank->getDx()
        + m_piston->m_body.v_y * bank->getDy();
}

double CombustionChamber::calculateMeanPistonSpeed() const {
    double avg = 0;
    for (int i = 0; i < StateSamples; ++i) {
        avg += m_pistonSpeed[i];
    }

    avg /= StateSamples;
    return avg;
}

double CombustionChamber::calculateFiringPressure() const {
    double firingPressure = 0;
    for (int i = 0; i < StateSamples; ++i) {
        if (m_pressure[i] > firingPressure) {
            firingPressure = m_pressure[i];
        }
    }

    return firingPressure;
}

bool CombustionChamber::popLitLastFrame() {
    const bool lit = m_litLastFrame;
    m_litLastFrame = false;

    return lit;
}

void CombustionChamber::ignite() {
    if (!m_lit) {
        if (m_system.mix().p_fuel == 0) return;

        const double afr = m_system.mix().p_o2 / m_system.mix().p_fuel;
        const double equivalenceRatio = afr / m_fuel->getMolecularAfr();
        if (equivalenceRatio < 0.5) return;
        else if (equivalenceRatio > 1.9) return;

        m_flameEvent.lastVolume = getVolume();
        m_flameEvent.travel_x = 0;
        m_flameEvent.travel_y = 0;
        m_flameEvent.lit_n = 0;
        m_flameEvent.total_n = m_system.n();
        m_flameEvent.percentageLit = 0;
        m_flameEvent.globalMix = m_system.mix();
        m_lit = true;
        m_litLastFrame = true;

        m_flameEvent.efficiency = 0.75 * (0.7 + 0.3 * ((double)rand() / RAND_MAX));

        const double turbulence = m_meanPistonSpeedToTurbulence->sampleTriangle(
            calculateMeanPistonSpeed());

        m_flameEvent.flameSpeed = m_fuel->flameSpeed(
            turbulence,
            afr,
            m_system.temperature(),
            m_system.pressure(),
            calculateFiringPressure(),
            units::pressure(160, units::psi));
    }
}

void CombustionChamber::start() {
    m_system.start();
}

void CombustionChamber::update(double dt) {
    m_system.start();
    m_system.setVolume(getVolume());
    m_system.end();

    updateCycleStates();

    m_intakeFlowRate = m_head->intakeFlowRate(m_piston->getCylinderIndex());
    m_exhaustFlowRate = m_head->exhaustFlowRate(m_piston->getCylinderIndex());
}

void CombustionChamber::flow(double dt) {
    if (m_system.temperature() > m_peakTemperature) {
        m_peakTemperature = m_system.temperature();
    }

    m_system.flow(m_piston->getBlowbyK(), dt, m_crankcasePressure, units::celcius(25.0));

    const double start_n = m_system.n();

    /*
    Intake *intake = m_head->getIntake(m_piston->getCylinderIndex());
    const double intakeFlow = m_system.flow(
            m_intakeFlowRate,
            dt,
            &intake->m_system,
            -DBL_MAX,
            DBL_MAX);

    const double exhaustFlow = m_system.flow(
            m_exhaustFlowRate,
            dt,
            &m_head->getExhaustSystem(m_piston->getCylinderIndex())->m_system,
            -DBL_MAX,
            DBL_MAX);*/

    const double exhaustFlow = doExhaustFlow(dt);
    const double intakeFlow = doIntakeFlow(dt);
    m_exhaustFlow = exhaustFlow;
    m_lastTimestepTotalExhaustFlow += m_exhaustFlow;

    if (std::abs(intakeFlow) > 1E-9 && m_lit) {
        m_lit = false;
    }

    if (m_lit) {
        CylinderBank *bank = m_head->getCylinderBank();
        const double volume = getVolume();
        const double totalTravel_x = bank->getBore() / 2;
        const double totalTravel_y = volume / bank->boreSurfaceArea();
        const double expansion = volume / m_flameEvent.lastVolume;
        const double lastTravel_x = m_flameEvent.travel_x;
        const double lastTravel_y = m_flameEvent.travel_y * expansion;
        const double flameSpeed = m_flameEvent.flameSpeed;

        m_flameEvent.travel_x =
            std::fmin(lastTravel_x + dt * flameSpeed, totalTravel_x);
        m_flameEvent.travel_y =
            std::fmin(lastTravel_y + dt * flameSpeed, totalTravel_y);

        if (lastTravel_x < m_flameEvent.travel_x || lastTravel_y < m_flameEvent.travel_y) {
            const double burnedVolume =
                m_flameEvent.travel_x * m_flameEvent.travel_x
                * constants::pi * m_flameEvent.travel_y;
            const double prevBurnedVolume =
                lastTravel_x * lastTravel_x * constants::pi * lastTravel_y;
            const double litVolume = burnedVolume - prevBurnedVolume;
            const double n = (litVolume / volume) * m_system.n();

            const double fuelBurned =
                m_system.react(n * m_flameEvent.efficiency, m_flameEvent.globalMix);
            const double massFuelBurned = fuelBurned * m_fuel->getMolecularMass();
            m_system.changeEnergy(
                massFuelBurned * m_fuel->getEnergyDensity());

            m_flameEvent.lit_n += n;
            m_flameEvent.percentageLit += litVolume / volume;

            m_nBurntFuel += massFuelBurned;
        }
        else {
            m_lit = false;
        }

        m_flameEvent.lastVolume = volume;
    }
}

void CombustionChamber::end() {
    m_system.end();
}

double CombustionChamber::lastEventAfr() const {
    const double totalFuel = m_flameEvent.globalMix.p_fuel * m_flameEvent.total_n;
    const double totalOxygen = m_flameEvent.globalMix.p_o2 * m_flameEvent.total_n;
    const double totalInert = m_flameEvent.globalMix.p_inert * m_flameEvent.total_n;

    constexpr double octaneMolarMass = units::mass(114.23, units::g);
    constexpr double oxygenMolarMass = units::mass(31.9988, units::g);
    constexpr double nitrogenMolarMass = units::mass(28.014, units::g);

    if (totalFuel == 0) return 0;
    else {
        return
            (oxygenMolarMass * totalOxygen + totalInert * nitrogenMolarMass)
            / (totalFuel * octaneMolarMass);
    }
}

double CombustionChamber::doExhaustFlow(double dt) {
    // temp
    ExhaustSystem *exhaust = m_head->getExhaustSystem(m_piston->getCylinderIndex());
    double P_0 = m_system.pressure(), P_1 = exhaust->m_system.pressure();
    double exhaustFlow = 0;
    if (m_exhaustVelocity < 0) {
        P_1 += exhaust->m_system.approximateDensity() * m_exhaustVelocity * m_exhaustVelocity;
    }
    else {
        P_0 += m_system.approximateDensity() * m_exhaustVelocity * m_exhaustVelocity;
    }

    exhaustFlow = dt * GasSystem::flowRate(
        m_exhaustFlowRate,
        P_0,
        P_1,
        m_system.temperature(),
        exhaust->m_system.temperature(),
        m_system.heatCapacityRatio(),
        m_system.cachedChokedFlowLimit(),
        m_system.cachedChokedFlowRate()
    );

    if (exhaustFlow < 0) {
        const double maxFlowRate = m_system.pressureEquilibriumMaxFlow(
            P_1,
            exhaust->m_system.temperature()
        );

        exhaustFlow = std::fmax(-exhaust->m_system.n(), exhaustFlow);
        //exhaustFlow = std::fmax(maxFlowRate, exhaustFlow);
    }
    else {
        const double maxFlowRate = exhaust->m_system.pressureEquilibriumMaxFlow(
            P_0,
            m_system.temperature()
        );

        exhaustFlow = std::fmin(m_system.n(), exhaustFlow);
        //exhaustFlow = std::fmin(-maxFlowRate, exhaustFlow);
    }

    m_system.flow(
        exhaustFlow,
        exhaust->m_system.kineticEnergyPerMol(),
        exhaust->m_system.mix());
    exhaust->m_system.flow(
        -exhaustFlow,
        m_system.kineticEnergyPerMol(),
        m_system.mix());

    // PV = nRT
    constexpr double exhaustRunnerArea =
        constants::pi * units::distance(0.9, units::inch) * units::distance(0.9, units::inch);
    const double exhaustVolume =
        exhaustFlow * constants::R * exhaust->m_system.temperature()
        / exhaust->m_system.pressure();
    double newExhaustVelocity = (exhaustVolume / exhaustRunnerArea) / dt;

    constexpr double c = units::distance(343, units::m) / units::sec;
    newExhaustVelocity = std::fmax(-c, std::fmin(newExhaustVelocity, c));

    const double f_c = 6000;
    const double RC = 1 / (2 * constants::pi * f_c);
    const double alpha = dt / (RC + dt);
    m_exhaustVelocity = (1 - alpha) * m_exhaustVelocity + alpha * newExhaustVelocity;
    //m_intakeVelocity = newIntakeVelocity;

    // end temp

    return exhaustFlow;
}

double CombustionChamber::doIntakeFlow(double dt) {
    // temp
    Intake *intake = m_head->getIntake(m_piston->getCylinderIndex());
    double P_0 = m_system.pressure(), P_1 = intake->m_system.pressure();
    double intakeFlow = 0;
    if (m_intakeVelocity < 0) {
        P_1 += intake->m_system.approximateDensity() * m_intakeVelocity * m_intakeVelocity;
    }
    else {
        P_0 += m_system.approximateDensity() * m_intakeVelocity * m_intakeVelocity;
    }

    intakeFlow = dt * GasSystem::flowRate(
        m_intakeFlowRate,
        P_0,
        P_1,
        m_system.temperature(),
        intake->m_system.temperature(),
        m_system.heatCapacityRatio(),
        m_system.cachedChokedFlowLimit(),
        m_system.cachedChokedFlowRate()
    );

    if (intakeFlow < 0) {
        const double maxFlowRate = m_system.pressureEquilibriumMaxFlow(
            P_1,
            intake->m_system.temperature()
        );

        intakeFlow = std::fmax(-intake->m_system.n(), intakeFlow);
        intakeFlow = std::fmax(maxFlowRate, intakeFlow);
    }
    else {
        const double maxFlowRate = intake->m_system.pressureEquilibriumMaxFlow(
            P_0,
            m_system.temperature()
        );

        intakeFlow = std::fmin(m_system.n(), intakeFlow);
        intakeFlow = std::fmin(-maxFlowRate, intakeFlow);
    }

    m_system.flow(
        intakeFlow,
        intake->m_system.kineticEnergyPerMol(),
        intake->m_system.mix());
    intake->m_system.flow(
        -intakeFlow,
        m_system.kineticEnergyPerMol(),
        m_system.mix());

    // PV = nRT
    constexpr double intakeRunnerArea =
        constants::pi * units::distance(0.9, units::inch) * units::distance(0.9, units::inch);
    const double intakeVolume =
        intakeFlow * constants::R * intake->m_system.temperature()
        / intake->m_system.pressure();
    double newIntakeVelocity = (intakeVolume / intakeRunnerArea) / dt;

    constexpr double c = units::distance(343, units::m) / units::sec;
    newIntakeVelocity = std::fmax(-c, std::fmin(newIntakeVelocity, c));

    const double f_c = 600;
    const double RC = 1 / (2 * constants::pi * f_c);
    const double alpha = dt / (RC + dt);
    m_intakeVelocity = (1 - alpha) * m_intakeVelocity + alpha * newIntakeVelocity;
    //m_intakeVelocity = newIntakeVelocity;

    //end temp

    return intakeFlow;
}

double CombustionChamber::calculateFrictionForce(double v_s) const {
    const double cylinderWallForce = m_piston->calculateCylinderWallForce();

    const double F_coul = m_frictionModel.frictionCoeff * cylinderWallForce;
    const double v_st = m_frictionModel.breakawayFrictionVelocity * constants::root_2;
    const double v_coul = m_frictionModel.breakawayFrictionVelocity / 10;
    const double F_brk = m_frictionModel.breakawayFriction;
    const double v = std::abs(v_s);

    const double F_0 = constants::root_2 * constants::e * (F_brk - F_coul);
    const double F_1 = v / v_st;
    const double F_2 = std::exp(-F_1 * F_1) * F_1;
    const double F_3 = F_coul * std::tanh(v / v_coul);
    const double F_4 = m_frictionModel.viscousFrictionCoefficient * v;

    return F_0 * F_2 + F_3 + F_4;
}

void CombustionChamber::updateCycleStates() {
    const double crankAngle = m_engine->getOutputCrankshaft()->getCycleAngle();
    const int i = std::round((crankAngle / (4 * constants::pi)) * (StateSamples - 1));

    m_pistonSpeed[i] = std::abs(pistonSpeed());
    m_pressure[i] = m_system.pressure();
}

void CombustionChamber::apply(atg_scs::SystemState *system) {
    CylinderBank *bank = m_head->getCylinderBank();
    const double area = (bank->getBore() * bank->getBore() / 4.0) * constants::pi;
    const double v_x = system->v_x[m_piston->m_body.index];
    const double v_y = system->v_y[m_piston->m_body.index];

    const double v_s =
        v_x * bank->getDx() + v_y * bank->getDy();

    const double pressureDifferential = m_system.pressure() - m_crankcasePressure;
    const double force = -area * pressureDifferential;

    if (std::isnan(force) || std::isinf(force)) {
        assert(false);
    }

    constexpr double limit = 1E-3;
    const double abs_v_s = std::fmin(std::abs(v_s), limit);
    const double attenuation = abs_v_s / limit;

    const double F = calculateFrictionForce(v_s) * attenuation;
    const double F_fric = (v_s > 0)
        ? -F
        : F;

    system->applyForce(
        0.0,
        0.0,
        (force + F_fric) * bank->getDx(),
        (force + F_fric) * bank->getDy(),
        m_piston->m_body.index);
}

double CombustionChamber::getFrictionForce() const {
    CylinderBank *bank = m_head->getCylinderBank();
    const double v_x = m_piston->m_body.v_x;
    const double v_y = m_piston->m_body.v_y;

    const double v_s =
        v_x * bank->getDx() + v_y * bank->getDy();

    return calculateFrictionForce(v_s);
}
