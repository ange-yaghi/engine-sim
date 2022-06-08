#include "../include/vehicle_load.h"

#include <cmath>

VehicleLoad::VehicleLoad() {
    /* void */
}

VehicleLoad::~VehicleLoad() {
    /* void */
}

void VehicleLoad::initialize(
    Crankshaft *outputShaft,
    atg_scs::RigidBody *flywheel)
{
    m_outputShaft = outputShaft;
    m_flywheel = flywheel;
}

void VehicleLoad::apply(atg_scs::SystemState *system) {
    double torque = 0;
    if (m_outputShaft->m_body.v_theta < m_flywheel->v_theta) {
        torque = 10000;
    }
    else {
        torque = -10000;
    }

    system->t[m_outputShaft->m_body.index] += torque;
    system->t[m_flywheel->index] -= torque;
}
