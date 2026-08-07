#ifndef ATG_ENGINE_SIM_TURBOCHARGER_NODE_H
#define ATG_ENGINE_SIM_TURBOCHARGER_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "intake_node.h"
#include "exhaust_system_node.h"
#include "bov_sound_node.h"
#include "supercharger_node.h"

#include "engine_sim.h"

#include <vector>

namespace es_script {

    class TurbochargerNode : public ObjectReferenceNode<TurbochargerNode> {
    public:
        TurbochargerNode() { /* void */ }
        virtual ~TurbochargerNode() { /* void */ }

        Turbocharger *generate(EngineContext *context) {
            Turbocharger *turbo = context->getTurbocharger(this);
            Turbocharger::Parameters parameters = m_parameters;
            if (m_bovSound != nullptr) {
                parameters.bovSound = m_bovSound->generate(context);
            }
            turbo->initialize(parameters);

            if (m_intake != nullptr) {
                Intake *intake = context->getIntake(m_intake);
                turbo->setIntake(intake);
                intake->addTurbocharger(turbo);
            }

            if (m_upstreamSupercharger != nullptr) {
                turbo->setUpstreamSupercharger(context->getSupercharger(m_upstreamSupercharger));
            }

            for (ExhaustSystemNode *exhaustNode : m_exhaustSystems) {
                turbo->addExhaustSystem(context->getExhaust(exhaustNode));
            }

            return turbo;
        }

        void addExhaustSystem(ExhaustSystemNode *exhaust) {
            m_exhaustSystems.push_back(exhaust);
        }

    protected:
        virtual void registerInputs() {
            addInput("intake", &m_intake, InputTarget::Type::Object);
            addInput("upstream_supercharger", &m_upstreamSupercharger, InputTarget::Type::Object);
            addInput("turbine_flow_rate", &m_parameters.turbineRateK);
            addInput("turbine_inertia", &m_parameters.turbineInertia);
            addInput("turbine_efficiency", &m_parameters.turbineEfficiency);
            addInput("compressor_efficiency", &m_parameters.compressorEfficiency);
            addInput("max_boost", &m_parameters.maxBoost);
            addInput("spool_speed", &m_parameters.spoolSpeed);
            addInput("spin_down_time", &m_parameters.spinDownTime);
            addInput("turbine_inducer_diameter", &m_parameters.turbineInducerDiameter);
            addInput("turbine_exducer_diameter", &m_parameters.turbineExducerDiameter);
            addInput("bov_sound", &m_bovSound, InputTarget::Type::Object);

            ObjectReferenceNode<TurbochargerNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        IntakeNode *m_intake = nullptr;
        SuperchargerNode *m_upstreamSupercharger = nullptr;
        BovSoundNode *m_bovSound = nullptr;
        std::vector<ExhaustSystemNode *> m_exhaustSystems;
        Turbocharger::Parameters m_parameters;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_TURBOCHARGER_NODE_H */
