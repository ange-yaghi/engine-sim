#ifndef ATG_ENGINE_SIM_DYNAMOMETER_H
#define ATG_ENGINE_SIM_DYNAMOMETER_H

#include "crankshaft_load.h"

class Dynamometer {
    public:
        Dynamometer();
        ~Dynamometer();

        void initialize(CrankshaftLoad *constraint);

        double readTorque() const;

        void setEnabled(bool enabled);
        bool isEnabled() const { return m_constraint->m_enableDyno; }

        void setSpeed(double speed) { m_constraint->m_dynoSpeed = speed; }
        double getSpeed() const { return m_constraint->m_dynoSpeed; }

    protected:
        CrankshaftLoad *m_constraint;
};

#endif /* ATG_ENGINE_SIM_DYNAMOMETER_H */
