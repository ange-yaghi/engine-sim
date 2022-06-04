#ifndef ATG_ENGINE_SIM_INTAKE_H
#define ATG_ENGINE_SIM_INTAKE_H

#include "part.h"

#include "gas_system.h"

class Intake : public Part {
    public:
        struct Parameters {
            double volume;
            double inputFlowK;
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
        double m_inputFlowK;
        double m_flow;

        double m_flowRate;
};

#endif /* ATG_ENGINE_SIM_INTAKE_H */
