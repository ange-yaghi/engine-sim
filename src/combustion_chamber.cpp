#include "../include/combustion_chamber.h"

#include "../include/constants.h"
#include "../include/units.h"
#include "../include/piston.h"
#include "../include/connecting_rod.h"
#include "../include/utilities.h"
#include "../include/exhaust_system.h"
#include "../include/cylinder_bank.h"

#include <cmath>

CombustionChamber::CombustionChamber() {
    m_crankcasePressure = 0.0;
    m_piston = nullptr;
    m_head = nullptr;
    m_lit = false;
    m_peakTemperature = 0;

    m_totalPropagationToTurbulence = nullptr;
    m_turbulentFlameSpeed = nullptr;
    m_nBurntFuel = 0;
    m_turbulence = 0;

    m_lastTimestepTotalExhaustFlow = 0;
    m_exhaustFlow = 0;
    m_exhaustFlowRate = 0;
    m_intakeFlowRate = 0;

    m_fuel = nullptr;
}

CombustionChamber::~CombustionChamber() {
    /* void */
}

void CombustionChamber::initialize(const Parameters &params) {
    m_piston = params.Piston;
    m_head = params.Head;
    m_fuel = params.Fuel;
    m_crankcasePressure = params.CrankcasePressure;
}

double CombustionChamber::getVolume() const {
    const double combustionPortVolume = m_head->m_combustionChamberVolume;
    const CylinderBank *bank = m_head->m_bank;

    const double area = bank->boreSurfaceArea();
    const double s =
        m_piston->relativeX() * bank->m_dx
        + m_piston->relativeY() * bank->m_dy;
    const double displacement =
        area * (bank->m_deckHeight - s - m_piston->m_compressionHeight);

    return displacement + combustionPortVolume;
}

bool CombustionChamber::popLitLastFrame() {
    const bool lit = m_litLastFrame;
    m_litLastFrame = false;

    return lit;
}

void CombustionChamber::ignite() {
    if (!m_lit) {
        m_flameEvent.lastVolume = getVolume();
        m_flameEvent.travel_x = 0;
        m_flameEvent.travel_y = 0;
        m_flameEvent.lit_n = 0;
        m_flameEvent.total_n = m_system.n();
        m_flameEvent.percentageLit = 0;
        m_flameEvent.globalMix = m_system.mix();
        m_lit = true;
        m_litLastFrame = true;

        constexpr double fastFlameSpeed = units::distance(15, units::m);
        constexpr double slowFlameSpeed = units::distance(10, units::m);

        const double fuel_air_low = 0;
        const double fuel_air_high = 4.0 / 25;
        const double fuel_air_ratio = std::max(
            std::fmin(m_system.mix().p_fuel / m_system.mix().p_o2, fuel_air_high),
            fuel_air_low);
        const double r = (double)rand() / RAND_MAX;
        const double s = ((fuel_air_ratio - fuel_air_low) / (fuel_air_high - fuel_air_low)) * (r * 0.5 + 0.5);

        m_flameEvent.efficiency = 0.75 * (0.7 + 0.3 * ((double)rand() / RAND_MAX));
        //m_flameEvent.flameSpeed = 0.8 * (s * fastFlameSpeed + (1 - s) * slowFlameSpeed);

        //m_flameEvent.efficiency = 1.0;
        m_flameEvent.flameSpeed = fastFlameSpeed;

        if (rand() % 4 == 0) {
            //m_flameEvent.efficiency = 1;
            //m_flameEvent.flameSpeed = fastFlameSpeed;
        }
    }
}

void CombustionChamber::start() {
    m_system.start();
}

void CombustionChamber::update(double dt) {
    m_system.start();
    m_system.setVolume(getVolume());
    m_system.end();

    m_intakeFlowRate = m_head->intakeFlowRate(m_piston->m_cylinderIndex);
    m_exhaustFlowRate = m_head->exhaustFlowRate(m_piston->m_cylinderIndex);
}

void CombustionChamber::flow(double dt) {
    if (m_system.temperature() > m_peakTemperature) {
        m_peakTemperature = m_system.temperature();
    }

    m_system.flow(m_piston->m_blowby_k, dt, m_crankcasePressure, units::celcius(25.0));

    const double start_n = m_system.n();

    const double intakeFlow = m_system.flow(
            m_intakeFlowRate,
            dt,
            &m_head->m_intakes[m_piston->m_cylinderIndex]->m_system,
            -DBL_MAX,
            DBL_MAX);

    const double exhaustFlow = m_system.flow(
            m_exhaustFlowRate,
            dt,
            &m_head->m_exhaustSystems[m_piston->m_cylinderIndex]->m_system,
            -DBL_MAX,
            DBL_MAX);

    m_exhaustFlow = exhaustFlow;
    m_lastTimestepTotalExhaustFlow += exhaustFlow;
    const double netFlow = exhaustFlow + intakeFlow;

    if (netFlow <= 0) {
        m_turbulence += -0.5 * netFlow * 12 * 3;
    }
    else {
        if (start_n > 0) {
            m_turbulence -= 2 * m_turbulence * (netFlow / start_n);

            if (m_turbulence < 0) {
                m_turbulence = 0;
            }
        }
    }

    if (m_lit) {
        CylinderBank *bank = m_head->m_bank;
        const double volume = getVolume();
        const double totalTravel_x = bank->m_bore / 2;
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
            (oxygenMolarMass * totalOxygen)
            / (totalFuel * octaneMolarMass + totalInert * nitrogenMolarMass);
    }
}

double CombustionChamber::calculateFrictionForce(double v_s) const {
    const double cylinderWallForce = std::sqrt(
        m_piston->m_cylinderConstraint->F_x[0][0] * m_piston->m_cylinderConstraint->F_x[0][0]
        + m_piston->m_cylinderConstraint->F_y[0][0] * m_piston->m_cylinderConstraint->F_y[0][0]);

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

void CombustionChamber::apply(atg_scs::SystemState *system) {
    CylinderBank *bank = m_head->m_bank;
    const double area = (bank->m_bore * bank->m_bore / 4.0) * constants::pi;
    const double v_x = system->v_x[m_piston->m_body.index];
    const double v_y = system->v_y[m_piston->m_body.index];

    const double v_s =
        v_x * bank->m_dx + v_y * bank->m_dy;

    const double pressureDifferential = m_system.pressure() - m_crankcasePressure;
    const double force = -area * pressureDifferential;

    if (std::isnan(force) || std::isinf(force)) {
        assert(false);
    }

    const double F = calculateFrictionForce(v_s);
    const double F_fric = (v_s > 0)
        ? -F
        : F;

    system->applyForce(
        0.0,
        0.0,
        (force + F_fric) * bank->m_dx,
        (force + F_fric) * bank->m_dy,
        m_piston->m_body.index);
}

double CombustionChamber::getFrictionForce() const {
    CylinderBank *bank = m_head->m_bank;
    const double v_x = m_piston->m_body.v_x;
    const double v_y = m_piston->m_body.v_y;

    const double v_s =
        v_x * bank->m_dx + v_y * bank->m_dy;

    return calculateFrictionForce(v_s);
}
