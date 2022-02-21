#ifndef ATG_ENGINE_SIM_DYNAMOMETER_H
#define ATG_ENGINE_SIM_DYNAMOMETER_H

#include "crankshaft.h"
#include "scs.h"

class Dynamometer {
    public:
        Dynamometer();
        ~Dynamometer();

        void initialize(Crankshaft *crankshaft, atg_scs::RigidBodySystem *system);

        double readTorque() const;

        void setEnabled(bool enabled);
        bool isEnabled() const { return m_enabled; }

        void setSpeed(double speed) { m_constraint.m_rotationSpeed = speed; }
        double getSpeed() const { return m_constraint.m_rotationSpeed; }

    protected:
        bool m_enabled;
        atg_scs::ConstantRotationConstraint m_constraint;
        atg_scs::RigidBodySystem *m_system;
};

#endif /* ATG_ENGINE_SIM_DYNAMOMETER_H */
