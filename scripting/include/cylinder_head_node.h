#ifndef ATG_ENGINE_SIM_CYLINDER_HEAD_NODE_H
#define ATG_ENGINE_SIM_CYLINDER_HEAD_NODE_H

#include "object_reference_node.h"

#include "camshaft_node.h"
#include "function_node.h"
#include "exhaust_system_node.h"
#include "intake_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class CylinderHeadNode : public ObjectReferenceNode<CylinderHeadNode> {
    public:
        CylinderHeadNode() { /* void */ }
        virtual ~CylinderHeadNode() { /* void */ }

        void generate(
            CylinderHead *head,
            CylinderBank *bank,
            Crankshaft *crankshaft,
            EngineContext *context) const
        {
            Camshaft
                *exhaustCam = context->getCamshaft(m_exhaustCam), 
                *intakeCam = context->getCamshaft(m_intakeCam);

            if (exhaustCam == nullptr) {
                exhaustCam = new Camshaft;
                m_exhaustCam->generate(exhaustCam, crankshaft, context);
                context->addCamshaft(m_exhaustCam, exhaustCam);
            }

            if (intakeCam == nullptr) {
                intakeCam = new Camshaft;
                m_intakeCam->generate(intakeCam, crankshaft, context);
                context->addCamshaft(m_intakeCam, intakeCam);
            }

            CylinderHead::Parameters params = m_parameters;
            params.Bank = bank;
            params.ExhaustCam = exhaustCam;
            params.IntakeCam = intakeCam;
            params.IntakePortFlow = m_intakePortFlow->generate(context);
            params.ExhaustPortFlow = m_exhaustPortFlow->generate(context);

            head->initialize(params);
        }

        void setBank(CylinderBankNode *bank) { m_bank = bank; }
        CylinderBankNode *getBank() const { return m_bank; }

    protected:
        virtual void registerInputs() {
            addInput(
                "intake_port_flow",
                &m_intakePortFlow,
                InputTarget::Type::Object);
            addInput(
                "exhaust_port_flow",
                &m_exhaustPortFlow,
                InputTarget::Type::Object);
            addInput(
                "intake_camshaft",
                &m_intakeCam,
                InputTarget::Type::Object);
            addInput(
                "exhaust_camshaft",
                &m_exhaustCam,
                InputTarget::Type::Object);
            addInput("chamber_volume", &m_parameters.CombustionChamberVolume);
            addInput("flip_display", &m_parameters.FlipDisplay);

            ObjectReferenceNode<CylinderHeadNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.Bank = nullptr;
            m_parameters.ExhaustCam = nullptr;
            m_parameters.IntakeCam = nullptr;
            m_parameters.IntakePortFlow = nullptr;
            m_parameters.ExhaustPortFlow = nullptr;
        }

        CylinderHead::Parameters m_parameters;

        CylinderBankNode *m_bank = nullptr;
        FunctionNode *m_intakePortFlow = nullptr;
        FunctionNode *m_exhaustPortFlow = nullptr;
        CamshaftNode *m_intakeCam = nullptr;
        CamshaftNode *m_exhaustCam = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CYLINDER_HEAD_NODE_H */
