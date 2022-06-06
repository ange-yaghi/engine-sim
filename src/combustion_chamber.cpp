#include "../include/combustion_chamber.h"

#include "../include/constants.h"
#include "../include/units.h"
#include "../include/piston.h"
#include "../include/connecting_rod.h"
#include "../include/utilities.h"
#include "../include/exhaust_system.h"

#include <cmath>

CombustionChamber::CombustionChamber() {
    m_crankcasePressure = 0.0;
    m_bank = nullptr;
    m_piston = nullptr;
    m_head = nullptr;
    m_blowbyK = 3E-4;
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
}

CombustionChamber::~CombustionChamber() {
    /* void */
}

void CombustionChamber::initialize(double p0, double t0) {
    m_crankcasePressure = p0;
    m_system.initialize(p0, volume(), t0);

    m_totalPropagationToTurbulence = new Function;
    m_totalPropagationToTurbulence->initialize(1, 0.1);
    m_totalPropagationToTurbulence->addSample(0.0, 0.8);
    m_totalPropagationToTurbulence->addSample(0.1, 0.95);
    m_totalPropagationToTurbulence->addSample(0.2, 1.0);
    m_totalPropagationToTurbulence->addSample(0.3, 1.0);
    m_totalPropagationToTurbulence->addSample(0.4, 1.0);
    m_totalPropagationToTurbulence->addSample(0.5, 1.0);
    m_totalPropagationToTurbulence->addSample(0.6, 1.0);
    m_totalPropagationToTurbulence->addSample(0.7, 1.0);
    m_totalPropagationToTurbulence->addSample(0.8, 1.0);
    m_totalPropagationToTurbulence->addSample(0.9, 0.95);
    m_totalPropagationToTurbulence->addSample(1.0, 0.8);

    m_turbulentFlameSpeed = new Function;
    m_turbulentFlameSpeed->initialize(1, units::rpm(1000));
    m_turbulentFlameSpeed->addSample(units::rpm(1000), 12.0);
    m_turbulentFlameSpeed->addSample(units::rpm(2000), 12.0);
    m_turbulentFlameSpeed->addSample(units::rpm(3000), 12.0);
    m_turbulentFlameSpeed->addSample(units::rpm(4000), 12.0);
    m_turbulentFlameSpeed->addSample(units::rpm(5000), 12.0);
    m_turbulentFlameSpeed->addSample(units::rpm(6000), 12.0);
    m_turbulentFlameSpeed->addSample(units::rpm(7000), 12.0);
}

double CombustionChamber::volume() const {
    const double combustionPortVolume = m_head->m_combustionChamberVolume;

    const double area = m_bank->boreSurfaceArea();
    const double s =
        m_piston->relativeX() * m_bank->m_dx + m_piston->relativeY() * m_bank->m_dy;
    const double displacement =
        area * (m_bank->m_deckHeight - s - m_piston->m_compressionHeight);

    return displacement + combustionPortVolume;
}

void CombustionChamber::ignite() {
    if (!m_lit) {
        m_flameEvent.lastVolume = volume();
        m_flameEvent.travel_x = 0;
        m_flameEvent.travel_y = 0;
        m_flameEvent.lit_n = 0;
        m_flameEvent.total_n = m_system.n();
        m_flameEvent.percentageLit = 0;
        m_flameEvent.globalMix = m_system.mix();
        m_lit = true;

        const double fastFlameSpeed = units::distance(15, units::m);
        const double slowFlameSpeed = units::distance(10, units::m);    

        const double fuel_air_low = 0;
        const double fuel_air_high = 4.0 / 25;
        const double fuel_air_ratio = std::max(
            std::fmin(m_system.mix().p_fuel / m_system.mix().p_o2, fuel_air_high),
            fuel_air_low);
        const double r = (double)rand() / RAND_MAX;
        const double s = ((fuel_air_ratio - fuel_air_low) / (fuel_air_high - fuel_air_low)) * (r * 0.5 + 0.5);

        m_flameEvent.efficiency = 0.7 + 0.3 * ((double)rand() / RAND_MAX);
        m_flameEvent.flameSpeed = 0.8 * (s * fastFlameSpeed + (1 - s) * slowFlameSpeed);

        if (rand() % 4 == 0) {
            m_flameEvent.efficiency = 1;
            m_flameEvent.flameSpeed = fastFlameSpeed;
        }
    }
}

void CombustionChamber::start() {
    m_system.start();
}

void CombustionChamber::update(double dt) {
    m_system.start();
    m_system.setVolume(volume());
    m_system.end();

    m_intakeFlowRate = m_head->intakeFlowRate(m_piston->m_cylinderIndex);
    m_exhaustFlowRate = m_head->exhaustFlowRate(m_piston->m_cylinderIndex);
}

void CombustionChamber::flow(double dt) {
    if (m_system.temperature() > m_peakTemperature) {
        m_peakTemperature = m_system.temperature();
    }

    m_system.flow(m_blowbyK, dt, m_crankcasePressure, units::celcius(25.0));

    const double start_n = m_system.n();

    const double intakeFlow = m_system.flow(
            m_intakeFlowRate,
            dt,
            &m_head->m_intakes[m_piston->m_cylinderIndex]->m_system);

    const double exhaustFlow = m_system.flow(
            m_exhaustFlowRate,
            dt,
            &m_head->m_exhaustSystems[m_piston->m_cylinderIndex]->m_system);

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
        const double totalTravel_x = m_bank->m_bore / 2;
        const double totalTravel_y = volume() / m_bank->boreSurfaceArea();
        const double expansion = volume() / m_flameEvent.lastVolume;
        const double lastTravel_x = m_flameEvent.travel_x;
        const double lastTravel_y = m_flameEvent.travel_y * expansion;

        //const double turbulence = erfApproximation(m_turbulence * m_flameEvent.turbulence);
        //const double turbulentFlameSpeed = units::distance(30.0, units::m);
        const double flameSpeed = m_flameEvent.flameSpeed;
        //    turbulence * turbulentFlameSpeed + (1 - turbulence) * units::distance(0.03, units::m);

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
            const double n = (litVolume / volume()) * m_system.n();
            //m_system.changeTemperature(m_flameEvent.temperature, n);

            const double octaneMolarMass = units::mass(114.23, units::g);
            const double fuelBurned = m_system.react(n * m_flameEvent.efficiency, m_flameEvent.globalMix);
            m_system.changeEnergy(
                units::convert(octaneMolarMass, units::g) * fuelBurned * units::energy(48.1, units::kJ));

            m_flameEvent.lit_n += n;
            m_flameEvent.percentageLit += litVolume / volume();

            // temp
            //const double massAir = units::mass(30, units::g);
            //const double massFuel = units::mass(100, units::g);
            //const double afr = (1 / 14.7);
            //const double mixMass = afr * massFuel + (1 - afr) * massAir;
            //const double totalMass = mixMass * n;
            //const double totalFuelMass = afr * totalMass;
            
            m_nBurntFuel += octaneMolarMass * fuelBurned;
        }
        else {
            m_lit = false;
        }

        m_flameEvent.lastVolume = volume();
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
    else return (oxygenMolarMass * totalOxygen) / (totalFuel * octaneMolarMass);
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
    const double area = (m_bank->m_bore * m_bank->m_bore / 4.0) * constants::pi;
    const double v_x = system->v_x[m_piston->m_body.index];
    const double v_y = system->v_y[m_piston->m_body.index];

    const double v_s =
        v_x * m_bank->m_dx + v_y * m_bank->m_dy;

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
        (force + F_fric) * m_bank->m_dx,
        (force + F_fric) * m_bank->m_dy,
        m_piston->m_body.index);
}

double CombustionChamber::getFrictionForce() const {
    const double v_x = m_piston->m_body.v_x;
    const double v_y = m_piston->m_body.v_y;

    const double v_s =
        v_x * m_bank->m_dx + v_y * m_bank->m_dy;

    return calculateFrictionForce(v_s);
}
