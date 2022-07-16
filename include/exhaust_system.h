#ifndef ATG_ENGINE_SIM_EXHAUST_SYSTEM_H
#define ATG_ENGINE_SIM_EXHAUST_SYSTEM_H

#include "part.h"

#include "gas_system.h"

class ExhaustSystem : public Part {
    public:
        struct Parameters {
            double Volume;
            double FlowK;
            double AudioVolume;
        };

    public:
        ExhaustSystem();
        virtual ~ExhaustSystem();

        void initialize(Parameters &params);
        virtual void destroy();

        void process(double dt);

        GasSystem m_system;
        double m_flowK;
        double m_flow;
        double m_audioVolume;
        int m_index;

        GasSystem m_atmosphere;
};

#endif /* ATG_ENGINE_SIM_EXHAUST_SYSTEM_H */
