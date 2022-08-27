#ifndef ATG_ENGINE_SIM_THROTTLE_H
#define ATG_ENGINE_SIM_THROTTLE_H

#include "part.h"

class Engine;
class Throttle {
public:
    Throttle();
    virtual ~Throttle();

    virtual void setSpeedControl(double s);
    virtual void update(double dt, Engine *engine);

    inline double getSpeedControl() const { return m_speedControl; }

protected:
    double m_speedControl;
};

#endif /* ATG_ENGINE_SIM_THROTTLE_H */
