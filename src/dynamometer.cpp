#include "../include/dynamometer.h"

#include "../include/units.h"

#include <cmath>

Dynamometer::Dynamometer() {
    m_enabled = false;
    m_system = nullptr;
}

Dynamometer::~Dynamometer() {
    /* void */
}

void Dynamometer::initialize(Crankshaft *crankshaft, atg_scs::RigidBodySystem *system) {
    m_constraint.setBody(&crankshaft->m_body);
    m_constraint.m_rotationSpeed = 0;
    m_constraint.m_maxTorque = units::torque(1000.0, units::ft_lb);
    m_constraint.m_minTorque = units::torque(-1000.0, units::ft_lb);

    m_system = system;
    m_enabled = false;
}

double Dynamometer::readTorque() const {
    return isEnabled()
        ? m_constraint.F_t[0][0]
        : 0;
}

void Dynamometer::setEnabled(bool enabled) {
    if (m_enabled) {
        m_system->removeConstraint(&m_constraint);
    }
    else {
        m_system->addConstraint(&m_constraint);
    }

    m_enabled = !m_enabled;
}
