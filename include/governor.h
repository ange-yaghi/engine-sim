#ifndef ATG_ENGINE_SIM_GOVERNOR_H
#define ATG_ENGINE_SIM_GOVERNOR_H

#include "throttle.h"

class Governor : public Throttle {
public:
    struct Parameters {
        double minSpeed;
        double maxSpeed;
        double minVelocity;
        double maxVelocity;
        double k_s;
        double k_d;
        double gamma;
    };

public:
    Governor();
    virtual ~Governor();

    void initialize(const Parameters &params);

    virtual void setSpeedControl(double s);
    virtual void update(double dt, Engine *engine);

protected:
    double m_minSpeed;
    double m_maxSpeed;
    double m_minVelocity;
    double m_maxVelocity;
    double m_k_s;
    double m_k_d;
    double m_gamma;

    double m_targetSpeed;

    double m_currentThrottle;
    double m_velocity;
};

#endif /* ATG_ENGINE_SIM_GOVERNOR_H */
