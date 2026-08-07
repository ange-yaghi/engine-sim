#ifndef ATG_ENGINE_SIM_VEHICLE_H
#define ATG_ENGINE_SIM_VEHICLE_H

#include "scs.h"

class Vehicle {
    public:
        struct Parameters {
            double mass;
            double dragCoefficient;
            double crossSectionArea;
            double diffRatio;
            double tireRadius;
            double rollingResistance;

            // Max brake force (N) at the wheels with the brake fully
            // applied -- see setBrakeInput(). Defaults to 0 (no brakes) so
            // existing scripts/tests that don't set it are unaffected.
            double brakeConstant = 0;
        };

    public:
        Vehicle();
        ~Vehicle();

        void initialize(const Parameters &params);
        void update(double dt);
        void addToSystem(atg_scs::RigidBodySystem *system, atg_scs::RigidBody *rotatingMass);
        inline double getMass() const { return m_mass; }
        inline double getRollingResistance() const { return m_rollingResistance; }
        inline double getDragCoefficient() const { return m_dragCoefficient; }
        inline double getCrossSectionArea() const { return m_crossSectionArea; }
        inline double getDiffRatio() const { return m_diffRatio; }
        inline double getTireRadius() const { return m_tireRadius; }
        inline double getBrakeConstant() const { return m_brakeConstant; }

        // Brake pedal position, 0 (off) to 1 (fully applied). Not clamped
        // here -- same as Transmission::setClutchPressure(), the caller is
        // expected to pass a value already in range.
        inline void setBrakeInput(double input) { m_brakeInput = input; }
        inline double getBrakeInput() const { return m_brakeInput; }

        double getSpeed() const;
        inline double getTravelledDistance() const { return m_travelledDistance; }
        inline void resetTravelledDistance() { m_travelledDistance = 0; }
        double linearForceToVirtualTorque(double force) const;

    protected:
        atg_scs::RigidBody *m_rotatingMass;

        double m_mass;
        double m_dragCoefficient;
        double m_crossSectionArea;
        double m_diffRatio;
        double m_tireRadius;
        double m_travelledDistance;
        double m_rollingResistance;
        double m_brakeConstant;
        double m_brakeInput;
};

#endif /* ATG_ENGINE_SIM_VEHICLE_H */
