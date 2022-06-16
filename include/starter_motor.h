#ifndef ATG_ENGINE_SIM_STARTER_MOTOR_H
#define ATG_ENGINE_SIM_STARTER_MOTOR_H

#include "scs.h"

#include "crankshaft.h"

class StarterMotor : public atg_scs::Constraint {
public:
    StarterMotor();
    virtual ~StarterMotor();

    void connectCrankshaft(Crankshaft *crankshaft);
    virtual void calculate(Output *output, atg_scs::SystemState *state);

    double m_ks;
    double m_kd;
    double m_maxTorque;
    double m_rotationSpeed;
    bool m_enabled;
};

#endif /* ATG_ENGINE_SIM_STARTER_MOTOR_H */
