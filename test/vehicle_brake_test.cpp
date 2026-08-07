#include <gtest/gtest.h>

#include "../include/vehicle.h"
#include "../include/vehicle_drag_constraint.h"

TEST(VehicleBrakeTest, BrakeWidensConstraintLimitsSymmetrically) {
    Vehicle::Parameters vp{};
    vp.mass = 1500.0;
    vp.dragCoefficient = 0.3;
    vp.crossSectionArea = 2.0;
    vp.diffRatio = 3.5;
    vp.tireRadius = 0.3;
    vp.rollingResistance = 300.0;
    vp.brakeConstant = 4000.0;

    Vehicle vehicle;
    vehicle.initialize(vp);

    atg_scs::RigidBody rotatingMass;
    rotatingMass.I = 1.0;
    rotatingMass.v_theta = 50.0;
    vehicle.addToSystem(nullptr, &rotatingMass);

    VehicleDragConstraint constraint;
    constraint.initialize(&rotatingMass, &vehicle);

    atg_scs::Constraint::Output offOutput;
    vehicle.setBrakeInput(0.0);
    constraint.calculate(&offOutput, nullptr);

    atg_scs::Constraint::Output onOutput;
    vehicle.setBrakeInput(1.0);
    constraint.calculate(&onOutput, nullptr);

    // Brake off: only rolling resistance/drag can push back, and only
    // against forward motion (upper limit pinned at 0).
    EXPECT_DOUBLE_EQ(offOutput.limits[0][1], 0.0);

    // Brake on: the allowed torque range widens symmetrically by the brake
    // torque on both ends, since a brake resists motion in either direction
    // (unlike drag/rolling resistance, which only ever opposes forward
    // travel in this model).
    const double brakeTorque = onOutput.limits[0][1];
    EXPECT_GT(brakeTorque, 0.0);
    EXPECT_DOUBLE_EQ(onOutput.limits[0][0], offOutput.limits[0][0] - brakeTorque);
}
