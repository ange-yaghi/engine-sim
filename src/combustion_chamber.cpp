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
    m_flameEvent.lastVolume = volume();
    m_flameEvent.travel = 0;
    m_lit = true;
}

void CombustionChamber::update(double dt) {
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
        const double totalTravel = volume() / m_bank->boreSurfaceArea();
        const double expansion = volume() / m_flameEvent.lastVolume;
        const double lastTravel = m_flameEvent.travel * expansion;
        m_flameEvent.lastVolume = volume();

        m_flameEvent.travel =
            std::fmin(lastTravel + dt * 100.5, totalTravel);

        if (lastTravel < m_flameEvent.travel) {
            const double litVolume =
                (m_flameEvent.travel - lastTravel) * m_bank->boreSurfaceArea();
            const double n = m_system.n(litVolume);
            m_system.changeTemperature(units::celcius(2138), n);
        }

        if (m_flameEvent.travel >= totalTravel) {
            const double finalTemp = m_system.temperature();
            m_lit = false;
        }
    }

    m_system.end();
}

void CombustionChamber::apply(atg_scs::SystemState *system) {
    const double area = (m_bank->m_bore * m_bank->m_bore / 4.0) * Constants::pi;

    const double v_s =
        m_piston->m_body.v_x * m_bank->m_dx + m_piston->m_body.v_y * m_bank->m_dy;

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

    const double F_fric = (v_s > 0)
        ? -cylinderWallForce * 0.1 - 500
        : cylinderWallForce * 0.1 + 500;
    const double F_damping = -v_s * 0.0;

    system->applyForce(
        0.0,
        0.0,
        F_x + (F_fric + F_damping) * m_bank->m_dx,
        F_y + (F_fric + F_damping) * m_bank->m_dy,
        m_piston->m_body.index);
}
