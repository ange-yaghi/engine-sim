#ifndef ATG_ENGINE_SIM_EXHAUST_SYSTEM_NODE_H
#define ATG_ENGINE_SIM_EXHAUST_SYSTEM_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "impulse_response_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class ExhaustSystemNode : public ObjectReferenceNode<ExhaustSystemNode> {
    public:
        ExhaustSystemNode() { /* void */ }
        virtual ~ExhaustSystemNode() { /* void */ }

        ExhaustSystem *generate(EngineContext *context) {
            ExhaustSystem *exhaust = context->getExhaust(this);
            ExhaustSystem::Parameters parameters = m_parameters;
            parameters.ImpulseResponse = m_impulseResponse->generate(context);

            exhaust->initialize(parameters);

            return exhaust;
        }

    protected:
        virtual void registerInputs() {
            addInput("volume", &m_parameters.Volume);
            addInput("collector_cross_section_area", &m_parameters.CollectorCrossSectionArea);
            addInput("outlet_flow_rate", &m_parameters.OutletFlowRate);
            addInput("primary_tube_length", &m_parameters.PrimaryTubeLength);
            addInput("primary_flow_rate", &m_parameters.PrimaryFlowRate);
            addInput("audio_volume", &m_parameters.AudioVolume);
            addInput("velocity_decay", &m_parameters.VelocityDecay);
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
