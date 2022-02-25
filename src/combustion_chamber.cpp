#include "../include/combustion_chamber.h"

#include "../include/constants.h"
#include "../include/units.h"
#include "../include/piston.h"

#include <cmath>

CombustionChamber::CombustionChamber() {
    m_crankcasePressure = 0.0;
    m_manifoldPressure = units::pressure(1.0, units::atm);
    m_bank = nullptr;
    m_piston = nullptr;
    m_head = nullptr;
    m_blowbyK = 3E-4;
    m_lit = false;
    m_peakTemperature = 0;
}

CombustionChamber::~CombustionChamber() {
    /* void */
}

void CombustionChamber::initialize(double p0, double t0) {
    m_crankcasePressure = p0;
    m_system.initialize(p0, volume(), t0);
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
        m_lit = true;
    }
}

void CombustionChamber::update(double dt) {
    if (m_system.temperature() > m_peakTemperature) {
        m_peakTemperature = m_system.temperature();
    }

    m_system.start();

    m_system.setVolume(volume());
    m_system.flow(m_blowbyK, dt, m_crankcasePressure, units::celcius(25.0));

    m_system.flow(m_head->intakeFlowRate(
        m_piston->m_cylinderIndex),
        dt,
        m_manifoldPressure,
        units::celcius(25.0));
    m_system.flow(m_head->exhaustFlowRate(
        m_piston->m_cylinderIndex),
        dt,
        units::pressure(1.0, units::atm),
        units::celcius(25.0));

    if (m_lit) {
        const double totalTravel_x = m_bank->m_bore / 2;
        const double totalTravel_y = volume() / m_bank->boreSurfaceArea();
        const double expansion = volume() / m_flameEvent.lastVolume;
        const double lastTravel_x = m_flameEvent.travel_x * expansion;
        const double lastTravel_y = m_flameEvent.travel_y * expansion;

        m_flameEvent.travel_x =
            std::fmin(lastTravel_x + (dt * 7) / 2, totalTravel_x);
        m_flameEvent.travel_y =
            std::fmin(lastTravel_y + dt * 7, totalTravel_y);

        if (lastTravel_x < m_flameEvent.travel_x || lastTravel_y < m_flameEvent.travel_y) {
            const double litVolume =
                (2 * m_flameEvent.travel_x * m_flameEvent.travel_y) - (2 * lastTravel_x * lastTravel_y);
            const double n = m_system.n(litVolume);
            m_system.changeTemperature(units::celcius(2138) * 1.0, n);

            m_flameEvent.lit_n += n;
        }
        else {
            const double finalTemp = m_system.temperature();
            m_lit = false;
        }

        m_flameEvent.lastVolume = volume();
    }

    m_system.end();
}

void CombustionChamber::apply(atg_scs::SystemState *system) {
    const double area = (m_bank->m_bore * m_bank->m_bore / 4.0) * Constants::pi;
    const double v_x = system->v_x[m_piston->m_body.index];
    const double v_y = system->v_y[m_piston->m_body.index];

    const double v_s =
        v_x * m_bank->m_dx + v_y * m_bank->m_dy;

    const double pressureDifferential = m_system.pressure() - m_crankcasePressure;
    const double force = area * pressureDifferential;
    const double F_x = -m_bank->m_dx * force;
    const double F_y = -m_bank->m_dy * force;

    if (std::isnan(force) || std::isinf(force)) {
        assert(false);
    }

    const double cylinderWallForce = std::sqrt(
        m_piston->m_cylinderConstraint->F_x[0][0] * m_piston->m_cylinderConstraint->F_x[0][0]
        + m_piston->m_cylinderConstraint->F_y[0][0] * m_piston->m_cylinderConstraint->F_y[0][0]);

    const double v_s_mag = std::abs(v_s);
    const double frictionCoeff = 0.06 + v_s_mag * 0.01;
    const double ringFriction = v_s_mag * units::force(6.0, units::N);

    const double F_fric = (v_s > 0)
        ? -cylinderWallForce * frictionCoeff - ringFriction
        : cylinderWallForce * frictionCoeff + ringFriction;

    system->applyForce(
        0.0,
        0.0,
        F_x + F_fric * m_bank->m_dx,
        F_y + F_fric * m_bank->m_dy,
        m_piston->m_body.index);
}
