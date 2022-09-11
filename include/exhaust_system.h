#ifndef ATG_ENGINE_SIM_EXHAUST_SYSTEM_H
#define ATG_ENGINE_SIM_EXHAUST_SYSTEM_H

#include "part.h"

#include "gas_system.h"
#include "impulse_response.h"

class ExhaustSystem : public Part {
    friend class Engine;

    public:
        struct Parameters {
            double length;
            double collectorCrossSectionArea;
            double outletFlowRate;
            double primaryTubeLength;
            double primaryFlowRate;
            double velocityDecay;
            double audioVolume;
            ImpulseResponse *impulseResponse;
        };

    public:
        ExhaustSystem();
        virtual ~ExhaustSystem();

        void initialize(const Parameters &params);
        virtual void destroy();

        void process(double dt);

        inline int getIndex() const { return m_index; }
        inline double getLength() const { return m_length; }
        inline double getFlow() const { return m_flow; }
        inline double getAudioVolume() const { return m_audioVolume; }
        inline double getPrimaryFlowRate() const { return m_primaryFlowRate; }
        inline double getCollectorCrossSectionArea() const { return m_collectorCrossSectionArea; }
        inline double getPrimaryTubeLength() const { return m_primaryTubeLength; }
        inline double getVelocityDecay() const { return m_velocityDecay; }
        inline ImpulseResponse *getImpulseResponse() const { return m_impulseResponse; }

        inline GasSystem *getSystem() { return &m_system; }

    protected:
        GasSystem m_atmosphere;
        GasSystem m_system;

        ImpulseResponse *m_impulseResponse;

        double m_length;
        double m_primaryTubeLength;
        double m_collectorCrossSectionArea;
        double m_primaryFlowRate;
        double m_outletFlowRate;
        double m_audioVolume;
        double m_velocityDecay;
        int m_index;

        double m_flow;
};

#endif /* ATG_ENGINE_SIM_EXHAUST_SYSTEM_H */
