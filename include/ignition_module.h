#ifndef ATG_ENGINE_SIM_IGNITION_MODULE_H
#define ATG_ENGINE_SIM_IGNITION_MODULE_H

#include "part.h"

#include "crankshaft.h"
#include "function.h"
#include "units.h"

class IgnitionModule : public Part {
    public:
        struct Parameters {
            int CylinderCount;
            Crankshaft *Crankshaft;
            Function *TimingCurve;
            double RevLimit = units::rpm(6000.0);
            double LimiterDuration = 0.5 * units::sec;
        };

        struct SparkPlug {
            double Angle = 0;
            bool IgnitionEvent = false;
            bool Enabled = false;
        };

    public:
        IgnitionModule();
        virtual ~IgnitionModule();

        virtual void destroy();

        void initialize(const Parameters &params);
        void setFiringOrder(int cylinderIndex, double angle);
        void reset();
        void update(double dt);

        bool getIgnitionEvent(int index) const;
        void resetIgnitionEvents();

        double getTimingAdvance();

        bool m_enabled;

    protected:
        SparkPlug *getPlug(int i);

        Function *m_timingCurve;
        SparkPlug *m_plugs;
        Crankshaft *m_crankshaft;
        int m_cylinderCount;

        double m_lastCrankshaftAngle;
        double m_revLimit;
        double m_revLimitTimer;
        double m_limiterDuration;
};

#endif /* ATG_ENGINE_SIM_IGNITION_MODULE_H */
