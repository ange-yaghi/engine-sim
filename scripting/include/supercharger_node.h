#ifndef ATG_ENGINE_SIM_SUPERCHARGER_NODE_H
#define ATG_ENGINE_SIM_SUPERCHARGER_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "intake_node.h"

#include "engine_sim.h"

#include <string>

namespace es_script {

    class SuperchargerNode : public ObjectReferenceNode<SuperchargerNode> {
    public:
        SuperchargerNode() { /* void */ }
        virtual ~SuperchargerNode() { /* void */ }

        Supercharger *generate(EngineContext *context) {
            Supercharger *supercharger = context->getSupercharger(this);
            Supercharger::Parameters parameters = m_parameters;
            parameters.type = parseType(m_type);

            supercharger->initialize(parameters);
            supercharger->setCrankshaft(context->getEngine()->getOutputCrankshaft());

            if (m_intake != nullptr) {
                // Optional (see objects.mr's `intake: intake()` default on
                // the public supercharger node): a twincharger's blower
                // stage feeds a turbo's inlet instead of a plenum, so it
                // deliberately leaves this unconnected to any real
                // cylinder -- context->getIntake() correctly returns
                // nullptr for an orphaned default IntakeNode that no
                // cylinder ever referenced, same as any other unattached
                // object in this graph. Guard the lookup result, not just
                // the raw node pointer (which is never null -- default
                // expressions still produce a real, if orphaned, node).
                Intake *intake = context->getIntake(m_intake);
                if (intake != nullptr) {
                    intake->addSupercharger(supercharger);
                }
            }

            return supercharger;
        }

    protected:
        static Supercharger::Type parseType(const std::string &type) {
            if (type == "twin_screw") return Supercharger::Type::TwinScrew;
            else if (type == "centrifugal") return Supercharger::Type::Centrifugal;
            else return Supercharger::Type::Roots;
        }

        virtual void registerInputs() {
            addInput("intake", &m_intake, InputTarget::Type::Object);
            addInput("type", &m_type);
            addInput("drive_ratio", &m_parameters.driveRatio);
            addInput("mechanical_efficiency", &m_parameters.mechanicalEfficiency);
            addInput("displacement", &m_parameters.displacement);
            addInput("volumetric_efficiency", &m_parameters.volumetricEfficiency);
            addInput("leakage_coefficient", &m_parameters.leakageCoefficient);
            addInput("max_impeller_speed", &m_parameters.maxImpellerSpeed);
            addInput("compressor_flow_rate", &m_parameters.compressorFlowRateK);
            addInput("compressor_efficiency", &m_parameters.compressorEfficiency);
            addInput("max_boost", &m_parameters.maxBoost);
            addInput("compressor_discharge_volume", &m_parameters.compressorDischargeVolume);
            addInput("clutched", &m_parameters.clutched);
            addInput("clutch_engage_rpm", &m_parameters.clutchEngageRpm);
            addInput("clutch_disengage_rpm", &m_parameters.clutchDisengageRpm);

            ObjectReferenceNode<SuperchargerNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        IntakeNode *m_intake = nullptr;
        std::string m_type = "roots";
        Supercharger::Parameters m_parameters;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_SUPERCHARGER_NODE_H */
