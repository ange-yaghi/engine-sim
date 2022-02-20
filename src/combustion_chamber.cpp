#include "../include/combustion_chamber.h"

#include "../include/constants.h"
#include "../include/units.h"

#include <cmath>

CombustionChamber::CombustionChamber() {
    m_crankcasePressure = 0.0;
    m_manifoldPressure = units::pressure(1.0, units::atm);
    m_bank = nullptr;
    m_piston = nullptr;
    m_head = nullptr;
    m_blowbyK = 5E-6;
}

CombustionChamber::~CombustionChamber() {
    /* void */
}

void CombustionChamber::initialize(double p0, double t0) {
    m_crankcasePressure = p0;
    m_system.initialize(p0, volume(), t0);
}

double CombustionChamber::volume() const {
    // Temp
    const double combustionPortVolume = units::volume(118, units::cc);

    const double area = (m_bank->m_bore * m_bank->m_bore / 4.0) * Constants::pi;
    const double s =
        m_piston->relativeX() * m_bank->m_dx + m_piston->relativeY() * m_bank->m_dy;
    const double displacement =
        area * (m_bank->m_deckHeight - s - m_piston->m_compressionHeight);

    return displacement + combustionPortVolume;
}

void CombustionChamber::update(double dt) {
    m_system.start();

    if (m_system.pressure() > units::pressure(160.0, units::psi)) {
        m_system.changeTemperature(units::celcius(1026.0) - m_system.temperature());
    }

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

    m_system.end();
}

void CombustionChamber::apply(atg_scs::SystemState *system) {
    const double area = (m_bank->m_bore * m_bank->m_bore / 4.0) * Constants::pi;

    const float frictionFmep = units::pressure(11.6, units::psi);

    const double v_s =
        m_piston->m_body.v_x * m_bank->m_dx + m_piston->m_body.v_y * m_bank->m_dy;

    double pressureDifferential = m_system.pressure() - m_crankcasePressure;
    pressureDifferential = v_s < 0
        ? pressureDifferential - frictionFmep
        : pressureDifferential + frictionFmep;

    const double force = area * pressureDifferential;
    const double F_x = -m_bank->m_dx * force;
    const double F_y = -m_bank->m_dy * force;

    if (std::isnan(force) || std::isinf(force)) {
        assert(false);
    }

    const double F_fric = (v_s > 0)
        ? -0
        : 0;
    const double F_damping = -v_s * 0.0;

    system->applyForce(
        0.0,
        0.0,
        F_x + (F_fric + F_damping) * m_bank->m_dx,
        F_y + (F_fric + F_damping) * m_bank->m_dy,
        m_piston->m_body.index);
}
