#include "../include/combustion_chamber.h"

#include "../include/constants.h"

#include <cmath>

CombustionChamber::CombustionChamber() {
    m_pressure = 0.0;
    m_temperature = 0.0;
    m_volume = 0.0;
    m_crankcasePressure = 0.0;
    m_bank = nullptr;
    m_piston = nullptr;
}

CombustionChamber::~CombustionChamber() {
    /* void */
}

void CombustionChamber::initialize(double p0, double t0) {
    m_pressure = m_crankcasePressure = p0;
    m_pressure = m_crankcasePressure * 10;
    m_temperature = t0;
    m_volume = volume();
}

double CombustionChamber::volume() {
    const double area = (m_bank->m_bore * m_bank->m_bore / 4.0) * Constants::pi;
    const double s =
        m_piston->relativeX() * m_bank->m_dx + m_piston->relativeY() * m_bank->m_dy;
    const double displacement = area * (m_bank->m_deckHeight - s);
    const double volume = displacement * area;

    return volume;
}

void CombustionChamber::adiabaticCompression() {
    const double gamma = 7 / 5.0;
    const double newVolume = volume();

    const double k2 = m_pressure * m_volume / m_temperature;

    m_pressure = m_pressure * std::pow((m_volume / newVolume), gamma);
    m_volume = newVolume;

    m_temperature = m_pressure * m_volume / k2;
}

void CombustionChamber::apply(atg_scs::SystemState *system) {
    const double area = (m_bank->m_bore * m_bank->m_bore / 4.0) * Constants::pi;

    const double force = area * (m_pressure - m_crankcasePressure);
    const double F_x = -m_bank->m_dx * force;
    const double F_y = -m_bank->m_dy * force;

    const double v_s =
        m_piston->m_body.v_x * m_bank->m_dx + m_piston->m_body.v_y * m_bank->m_dy;

    const double F_fric = (v_s > 0)
        ? -1000
        : 1000;
    const double F_damping = -v_s * 10.0;

    system->applyForce(
        0.0,
        0.0,
        F_x + (F_fric + F_damping) * m_bank->m_dx,
        F_y + (F_fric + F_damping) * m_bank->m_dy,
        m_piston->m_body.index);
}
