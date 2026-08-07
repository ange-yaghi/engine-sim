#include "../include/vehicle_drag_constraint.h"

#include "../include/constants.h"
#include "../include/units.h"
#include "../include/vehicle.h"

VehicleDragConstraint::VehicleDragConstraint() : Constraint(1, 1) {
    m_ks = 10.0;
    m_kd = 1.0;

    m_vehicle = nullptr;
}

VehicleDragConstraint::~VehicleDragConstraint() {
    /* void */
}

void VehicleDragConstraint::initialize(atg_scs::RigidBody *rotatingMass, Vehicle *vehicle) {
    m_bodies[0] = rotatingMass;
    m_vehicle = vehicle;
}

void VehicleDragConstraint::calculate(Output *output, atg_scs::SystemState *system) {
    output->C[0] = 0;

    output->J[0][0] = 0.0;
    output->J[0][1] = 0.0;
    output->J[0][2] = -1.0;

    output->J[0][3] = 0.0;
    output->J[0][4] = 0.0;
    output->J[0][5] = 1.0;

    output->J_dot[0][0] = 0;
    output->J_dot[0][1] = 0;
    output->J_dot[0][2] = 0;

    output->J_dot[0][3] = 0;
    output->J_dot[0][4] = 0;
    output->J_dot[0][5] = 0;

    output->kd[0] = m_kd;
    output->ks[0] = m_ks;

    output->v_bias[0] = 0;

    constexpr double airDensity =
        units::AirMolecularMass * units::pressure(1.0, units::atm)
        / (constants::R * units::celcius(25.0));
    const double v = m_vehicle->getSpeed();
    const double v_squared = v * v;
    const double c_d = m_vehicle->getDragCoefficient();
    const double A = m_vehicle->getCrossSectionArea();
    const double rollingResistance = m_vehicle->getRollingResistance();

    // Brakes, unlike drag/rolling resistance, can push back in either
    // direction -- they just resist whatever way the wheels are currently
    // turning. This constraint already drives relative velocity to zero
    // (C[0] == 0 above), clamped to [limits[0][0], limits[0][1]], so widening
    // that range symmetrically is enough: the solver only uses as much of it
    // as needed to arrest the current motion.
    const double brakeTorque =
        m_vehicle->linearForceToVirtualTorque(
            m_vehicle->getBrakeConstant() * m_vehicle->getBrakeInput());

    output->limits[0][0] =
        -m_vehicle->linearForceToVirtualTorque(rollingResistance + 0.5 * airDensity * v_squared * c_d * A)
        - brakeTorque;
    output->limits[0][1] = brakeTorque;
}
