#ifndef ATG_ENGINE_SIM_DIRECT_THROTTLE_LINKAGE_H
#define ATG_ENGINE_SIM_DIRECT_THROTTLE_LINKAGE_H

#include "throttle.h"

class DirectThrottleLinkage : public Throttle {
public:
    struct Parameters {
        double gamma;
    };

public:
    DirectThrottleLinkage();
    virtual ~DirectThrottleLinkage();

    void initialize(const Parameters &params);

    virtual void setSpeedControl(double s);
    virtual void update(double dt, Engine *engine);

protected:
    double m_gamma;
    double m_throttlePosition;
};

#endif /* ATG_ENGINE_SIM_DIRECT_THROTTLE_LINKAGE_H */
