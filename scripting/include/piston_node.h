#ifndef ATG_ENGINE_SIM_PISTON_NODE_H
#define ATG_ENGINE_SIM_PISTON_NODE_H

#include "object_reference_node.h"

#include "rod_journal_node.h"
#include "piston_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class PistonNode : public ObjectReferenceNode<PistonNode> {
    public:
        PistonNode() { /* void */ }
        virtual ~PistonNode() { /* void */ }

        void generate(
            Piston *piston,
            ConnectingRod *rod,
            CylinderBank *cylinderBank,
            int cylinderIndex) const
        {
            Piston::Parameters params = m_parameters;
            params.Bank = cylinderBank;
            params.CylinderIndex = cylinderIndex;
            params.Rod = rod;

            piston->initialize(params);
        }

    protected:
        virtual void registerInputs() {
            addInput("mass", &m_parameters.Mass);
            addInput("blowby", &m_parameters.BlowbyFlowCoefficient);
            addInput("compression_height", &m_parameters.CompressionHeight);
            addInput("wrist_pin_position", &m_parameters.WristPinPosition);
            addInput("displacement", &m_parameters.Displacement);

            ObjectReferenceNode<PistonNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.Bank = nullptr;
            m_parameters.Rod = nullptr;
            m_parameters.CylinderIndex = 0;
        }

        Piston::Parameters m_parameters;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_PISTON_NODE_H */
