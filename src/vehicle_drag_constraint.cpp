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

    output->limits[0][0] =
        -m_vehicle->linearForceToVirtualTorque(rollingResistance + 0.5 * airDensity * v_squared * c_d * A);
    output->limits[0][1] = 0;
}
