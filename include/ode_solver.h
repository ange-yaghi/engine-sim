#ifndef ENGINE_SIM_ODE_SOLVER_H
#define ENGINE_SIM_ODE_SOLVER_H

#include <assert.h>

class OdeSolver {
public:
    struct System {
        System() {
            /* void */
        }
        ~System() {
            assert(AngularAcceleration == nullptr);
        }

        double *AngularAcceleration = nullptr;
        double *AngularVelocity = nullptr;
        double *Angles = nullptr;

        double *Acceleration_X = nullptr;
        double *Acceleration_Y = nullptr;
        double *Velocity_X = nullptr;
        double *Velocity_Y = nullptr;
        double *Position_X = nullptr;
        double *Position_Y = nullptr;

        double *Force_X = nullptr;
        double *Force_Y = nullptr;
        double *Torque = nullptr;

        double dt = 0.0;

        int BodyCount = 0;
    };

    // dv/dt = a
    // dp/dt = v

public:
    OdeSolver();
    virtual ~OdeSolver();

    static void initializeSystem(System *target, int bodyCount, double dt);
    static void copySystem(System *source, System *target);
    static void destroySystem(System *target);

    virtual void start(System *initial, double dt);
    virtual bool step(System *system);
    virtual void solve(System *in, System *out);
    virtual void end();

protected:
    double m_dt;
    int m_bodyCount;
};

#endif /* ENGINE_SIM_ODE_SOLVER_H */
