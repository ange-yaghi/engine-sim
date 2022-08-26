#include "../include/vehicle.h"

#include <cmath>

Vehicle::Vehicle() {
    m_rotatingMass = nullptr;
    m_mass = 0;
    m_dragCoefficient = 0;
    m_crossSectionArea = 0;
    m_diffRatio = 0;
    m_tireRadius = 0;
    m_travelledDistance = 0;
    m_rollingResistance = 0;
}

Vehicle::~Vehicle() {
    /* void */
}

void Vehicle::initialize(const Parameters &params) {
    m_mass = params.mass;
    m_dragCoefficient = params.dragCoefficient;
    m_crossSectionArea = params.crossSectionArea;
    m_diffRatio = params.diffRatio;
    m_tireRadius = params.tireRadius;
    m_rollingResistance = params.rollingResistance;
}

void Vehicle::update(double dt) {
    m_travelledDistance += getSpeed() * dt;
}

void Vehicle::addToSystem(atg_scs::RigidBodySystem *system, atg_scs::RigidBody *rotatingMass) {
    m_rotatingMass = rotatingMass;
}

double Vehicle::getSpeed() const {
    const double E_r = 0.5 * m_rotatingMass->I * m_rotatingMass->v_theta * m_rotatingMass->v_theta;
    const double vehicleSpeed = std::sqrt(2 * E_r / m_mass);

    return vehicleSpeed;

    // E_r = 0.5 * I * v_theta^2
    // E_k = 0.5 * m * v^2
}

double Vehicle::linearForceToVirtualTorque(double force) const {
    const double rotationToKineticRatio =
        std::sqrt(m_rotatingMass->I / m_mass);
    return rotationToKineticRatio * force;
}
