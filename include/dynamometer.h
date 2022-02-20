#ifndef ATG_ENGINE_SIM_DYNAMOMETER_H
#define ATG_ENGINE_SIM_DYNAMOMETER_H

#include "engine_load.h"

class Dynamometer {
    public:
        enum class State {
            Idle,
            Measuring,
            Timeout,
            Complete
        };

        struct Measurement {
            double Speed = 0;
            double Torque = 0;

            double Window = 0;
            int Samples = 0;
        };

    public:
        Dynamometer();
        ~Dynamometer();

        void initialize(Crankshaft *crankshaft);

        bool isComplete() const { return m_state == State::Complete; }
        bool isReady() const;
        void startMeasurement(double window, double speed);
        Measurement getLastMeasurement() const { return m_lastMeasurement; }

        EngineLoad *getEngineLoad() { return &m_device; }

        void update(double dt);

        double m_timeout;
        double m_maxTorque;

    protected:
        State measure(double dt);

        EngineLoad m_device;

        Measurement m_lastMeasurement;
        double m_measurementTimeLeft;
        double m_timeoutTimeLeft;
        double m_maxSpeedDeviation;

        double m_torqueVelocity;

        State m_state;
};

#endif /* ATG_ENGINE_SIM_DYNAMOMETER_H */
