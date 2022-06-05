#ifndef ATG_ENGINE_SIM_INTAKE_H
#define ATG_ENGINE_SIM_INTAKE_H

#include "part.h"

#include "gas_system.h"

class Intake : public Part {
    public:
        struct Parameters {
            // Plenum volume
            double volume;

            // Input flow constant
            double inputFlowK;

            // Molecular air fuel ratio (defaults to ideal for octane)
            double molecularAfr = (25.0 / 2.0) * 2.5;
        };

    public:
        Intake();
        virtual ~Intake();

        void initialize(Parameters &params);
        virtual void destroy();

        void start();
        void process(double dt);
        void end();

        GasSystem m_system;
        double m_throttle;

        double m_flow;
        double m_flowRate;
        double m_totalFuelInjected;

    protected:
        double m_inputFlowK;
        double m_molecularAfr;
};

#endif /* ATG_ENGINE_SIM_INTAKE_H */
