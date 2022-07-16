#ifndef ATG_ENGINE_SIM_CONNECTING_ROD_NODE_H
#define ATG_ENGINE_SIM_CONNECTING_ROD_NODE_H

#include "object_reference_node.h"

#include "rod_journal_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class ConnectingRodNode : public ObjectReferenceNode<ConnectingRodNode> {
    public:
        ConnectingRodNode() { /* void */ }
        virtual ~ConnectingRodNode() { /* void */ }

        void generate(
            ConnectingRod *connectingRod,
            Crankshaft *crankshaft) const
        {
            ConnectingRod::Parameters params = m_parameters;
            params.Crankshaft = crankshaft;
            params.Journal = m_rodJournal->getJournalIndex();
            params.Piston = nullptr;

            connectingRod->initialize(params);
        }

    protected:
        virtual void registerInputs() {
            addInput("mass", &m_parameters.Mass);
            addInput("moment_of_inertia", &m_parameters.MomentOfInertia);
            addInput("center_of_mass", &m_parameters.CenterOfMass);
            addInput("length", &m_parameters.Length);
            addInput("rod_journal", &m_rodJournal, InputTarget::Type::Object);

            ObjectReferenceNode<ConnectingRodNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.Crankshaft = nullptr;
            m_parameters.Piston = nullptr;

            m_rodJournal->addRod(this);
        }

        ConnectingRod::Parameters m_parameters;
        RodJournalNode *m_rodJournal = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CONNECTING_ROD_NODE_H */
