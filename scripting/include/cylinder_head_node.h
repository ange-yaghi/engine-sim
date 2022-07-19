#ifndef ATG_ENGINE_SIM_CYLINDER_HEAD_NODE_H
#define ATG_ENGINE_SIM_CYLINDER_HEAD_NODE_H

#include "object_reference_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class CylinderHeadNode : public ObjectReferenceNode<CylinderHeadNode> {
    public:
        CylinderHeadNode() { /* void */ }
        virtual ~CylinderHeadNode() { /* void */ }

        void generate(CylinderHead *head, CylinderBank *bank) const {
            CylinderHead::Parameters params = m_parameters;
            params.Bank = bank;

            head->initialize(params);
        }

    protected:
        virtual void registerInputs() {
            addInput(
                "intake_port_flow",
                &m_parameters.IntakePortFlow,
                InputTarget::Type::Object);
            addInput(
                "exhaust_port_flow",
                &m_parameters.ExhaustPortFlow,
                InputTarget::Type::Object);
            addInput(
                "intake_camshaft",
                &m_parameters.IntakeCam,
                InputTarget::Type::Object);
            addInput(
                "exhaust_camshaft",
                &m_parameters.ExhaustCam,
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
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CYLINDER_HEAD_NODE_H */
