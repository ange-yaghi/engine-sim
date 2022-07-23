#ifndef ATG_ENGINE_SIM_EXHAUST_SYSTEM_H
#define ATG_ENGINE_SIM_EXHAUST_SYSTEM_H

#include "part.h"

#include "gas_system.h"

class ExhaustSystem : public Part {
    friend class Engine;

    public:
        struct Parameters {
            double Volume;
            double CollectorCrossSectionArea;
            double OutletFlowRate;
            double PrimaryTubeLength;
            double PrimaryFlowRate;
            double AudioVolume;
        };

    public:
        ExhaustSystem();
        virtual ~ExhaustSystem();

        void initialize(const Parameters &params);
        virtual void destroy();

        void process(double dt);

        inline int getIndex() const { return m_index; }
        inline double getFlow() const { return m_flow; }
        inline double getAudioVolume() const { return m_audioVolume; }

        inline GasSystem *getSystem() { return &m_system; }

    protected:
        GasSystem m_atmosphere;
        GasSystem m_system;

        double m_collectorCrossSectionArea;
        double m_primaryFlowRate;
        double m_outletFlowRate;
        double m_audioVolume;
        int m_index;

        double m_flow;
};

#endif /* ATG_ENGINE_SIM_EXHAUST_SYSTEM_H */
