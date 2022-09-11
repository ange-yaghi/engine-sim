#ifndef ATG_ENGINE_SIM_EXHAUST_SYSTEM_NODE_H
#define ATG_ENGINE_SIM_EXHAUST_SYSTEM_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "impulse_response_node.h"

#include "engine_sim.h"

namespace es_script {

    class ExhaustSystemNode : public ObjectReferenceNode<ExhaustSystemNode> {
    public:
        ExhaustSystemNode() { /* void */ }
        virtual ~ExhaustSystemNode() { /* void */ }

        ExhaustSystem *generate(EngineContext *context) {
            ExhaustSystem *exhaust = context->getExhaust(this);
            ExhaustSystem::Parameters parameters = m_parameters;
            parameters.impulseResponse = m_impulseResponse->generate(context);

            exhaust->initialize(parameters);

            return exhaust;
        }

    protected:
        virtual void registerInputs() {
            addInput("length", &m_parameters.length);
            addInput("collector_cross_section_area", &m_parameters.collectorCrossSectionArea);
            addInput("outlet_flow_rate", &m_parameters.outletFlowRate);
            addInput("primary_tube_length", &m_parameters.primaryTubeLength);
            addInput("primary_flow_rate", &m_parameters.primaryFlowRate);
            addInput("audio_volume", &m_parameters.audioVolume);
            addInput("velocity_decay", &m_parameters.velocityDecay);
            addInput("impulse_response", &m_impulseResponse, InputTarget::Type::Object);

            ObjectReferenceNode<ExhaustSystemNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        ImpulseResponseNode *m_impulseResponse = nullptr;
        ExhaustSystem::Parameters m_parameters;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_EXHAUST_SYSTEM_NODE_H */
