#ifndef ATG_ENGINE_SIM_EXHAUST_SYSTEM_H
#define ATG_ENGINE_SIM_EXHAUST_SYSTEM_H

#include "part.h"

#include "gas_system.h"

class ExhaustSystem : public Part {
    public:
        struct Parameters {
            double volume;
            double flowK;
        };

    public:
        ExhaustSystem();
        virtual ~ExhaustSystem();

        void initialize(Parameters &params);
        virtual void destroy();

        void start();
        void process(double dt);
        void end();

        GasSystem m_system;
        double m_flowK;
        double m_flow;
        int m_index;
};

#endif /* ATG_ENGINE_SIM_EXHAUST_SYSTEM_H */
