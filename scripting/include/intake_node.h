#ifndef ATG_ENGINE_SIM_INTAKE_NODE_H
#define ATG_ENGINE_SIM_INTAKE_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "function_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class IntakeNode : public ObjectReferenceNode<IntakeNode> {
    public:
        IntakeNode() { /* void */ }
        virtual ~IntakeNode() { /* void */ }

        Intake *generate(EngineContext *context) {
            Intake *intake = context->getIntake(this);
            Intake::Parameters parameters = m_parameters;
            intake->initialize(parameters);

            return intake;
        }

    protected:
        virtual void registerInputs() {
            addInput("plenum_volume", &m_parameters.Volume);
            addInput("plenum_cross_section_area", &m_parameters.CrossSectionArea);
            addInput("intake_flow_rate", &m_parameters.InputFlowK);
            addInput("idle_flow_rate", &m_parameters.IdleFlowK);
            addInput("runner_flow_rate", &m_parameters.RunnerFlowRate);
            addInput("molecular_afr", &m_parameters.MolecularAfr);
            addInput("idle_throttle_plate_position", &m_parameters.IdleThrottlePlatePosition);
            addInput("throttle_gamma", &m_throttleGammaUnused);
            addInput("runner_length", &m_parameters.RunnerLength);
            addInput("velocity_decay", &m_parameters.VelocityDecay);

            ObjectReferenceNode<IntakeNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        double m_throttleGammaUnused = 0.0; // Deprecated; to be removed in a future release
        Intake::Parameters m_parameters;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_INTAKE_NODE_H */
