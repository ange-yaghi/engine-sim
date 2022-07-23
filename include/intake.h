#ifndef ATG_ENGINE_SIM_INTAKE_H
#define ATG_ENGINE_SIM_INTAKE_H

#include "part.h"

#include "gas_system.h"

class Intake : public Part {
    public:
        struct Parameters {
            // Plenum volume
            double Volume;

            // Plenum dimensions
            double CrossSectionArea;

            // Input flow constant
            double InputFlowK;

            // Idle-circuit flow constant
            double IdleFlowK;

            // Molecular air fuel ratio (defaults to ideal for octane)
            double MolecularAfr = (25.0 / 2.0);

            // Throttle plate position at idle
            double IdleThrottlePlatePosition = 0.975;

            // Gamma value to skew throttle positions
            double ThrottleGamma = 2.0;

            // Runner volume
            double RunnerVolume = units::volume(300, units::cc);
        };

    public:
        Intake();
        virtual ~Intake();

        void initialize(Parameters &params);
        virtual void destroy();

        void process(double dt);

        inline double getThrottlePlatePosition() const { return m_idleThrottlePlatePosition * m_throttle; }

        GasSystem m_system;
        double m_throttle;

        double m_flow;
        double m_flowRate;
        double m_totalFuelInjected;

    protected:
        double m_crossSectionArea;
        double m_inputFlowK;
        double m_idleFlowK;
        double m_molecularAfr;
        double m_idleThrottlePlatePosition;
        double m_throttleGamma;
        double m_runnerVolume;

        GasSystem m_atmosphere;
};

#endif /* ATG_ENGINE_SIM_INTAKE_H */
