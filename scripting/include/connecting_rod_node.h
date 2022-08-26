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

        void addRodJournal(RodJournalNode *rodJournal) {
            m_rodJournals.push_back(rodJournal);
            rodJournal->m_rod = this;
        }

        bool isMaster() const {
            return !m_rodJournals.empty();
        }

        void indexSlaveJournals(EngineContext *context) const {
            int index = 0;
            for (RodJournalNode *journal : m_rodJournals) {
                context->addRodJournal(journal, index++);
            }
        }

        void generate(
            ConnectingRod *connectingRod,
            Crankshaft *crankshaft,
            Piston *piston,
            int rodJournal) const
        {
            ConnectingRod::Parameters params = m_parameters;
            params.Crankshaft = crankshaft;
            params.Journal = rodJournal;
            params.Piston = piston;
            params.RodJournals = static_cast<int>(m_rodJournals.size());
            params.Master = nullptr;

            connectingRod->initialize(params);

            for (int i = 0; i < params.RodJournals; ++i) {
                connectingRod->setRodJournalAngle(i, m_rodJournals[i]->getAngle() + constants::pi / 2);
            }
        }

    protected:
        virtual void registerInputs() {
            addInput("mass", &m_parameters.Mass);
            addInput("moment_of_inertia", &m_parameters.MomentOfInertia);
            addInput("center_of_mass", &m_parameters.CenterOfMass);
            addInput("length", &m_parameters.Length);
            addInput("slave_throw", &m_parameters.SlaveThrow);

            ObjectReferenceNode<ConnectingRodNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.Crankshaft = nullptr;
            m_parameters.Piston = nullptr;
        }

        ConnectingRod::Parameters m_parameters;
        std::vector<RodJournalNode *> m_rodJournals;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CONNECTING_ROD_NODE_H */
