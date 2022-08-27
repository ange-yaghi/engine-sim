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

            // Flow rate from plenum to runner
            double RunnerFlowRate;

            // Molecular air fuel ratio (defaults to ideal for octane)
            double MolecularAfr = (25.0 / 2.0);

            // Throttle plate position at idle
            double IdleThrottlePlatePosition = 0.975;

            // Runner volume
            double RunnerLength = units::distance(4.0, units::inch);

            // Velocity decay factor
            double VelocityDecay = 0.5;
        };

    public:
        Intake();
        virtual ~Intake();

        void initialize(Parameters &params);
        virtual void destroy();

        void process(double dt);

        inline double getRunnerFlowRate() const { return m_runnerFlowRate; }
        inline double getThrottlePlatePosition() const { return m_idleThrottlePlatePosition * m_throttle; }
        inline double getRunnerLength() const { return m_runnerLength; }
        inline double getPlenumCrossSectionArea() const { return m_crossSectionArea; }
        inline double getVelocityDecay() const { return m_velocityDecay; }

        GasSystem m_system;
        double m_throttle;

        double m_flow;
        double m_flowRate;
        double m_totalFuelInjected;

    protected:
        double m_crossSectionArea;
        double m_inputFlowK;
        double m_idleFlowK;
        double m_runnerFlowRate;
        double m_molecularAfr;
        double m_idleThrottlePlatePosition;
        double m_runnerLength;
        double m_velocityDecay;

        GasSystem m_atmosphere;
};

#endif /* ATG_ENGINE_SIM_INTAKE_H */
