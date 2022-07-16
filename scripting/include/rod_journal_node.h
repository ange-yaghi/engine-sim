#ifndef ATG_ENGINE_SIM_ROD_JOURNAL_NODE_H
#define ATG_ENGINE_SIM_ROD_JOURNAL_NODE_H

#include "object_reference_node.h"

#include "engine_sim.h"

#include <map>

namespace es_script {

    class ConnectingRodNode;
    class RodJournalNode : public ObjectReferenceNode<RodJournalNode> {
    public:
        RodJournalNode() { /* void */ }
        virtual ~RodJournalNode() { /* void */ }

        double getAngle() const {
            return m_angle;
        }

        int getJournalIndex() const {
            return m_journalIndex;
        }

        void setJournalIndex(int index) {
            m_journalIndex = index;
        }

        void setCrankshaft(CrankshaftNode *crankshaft) {
            m_crankshaft = crankshaft;
        }

    protected:
        virtual void registerInputs() {
            addInput("angle", &m_angle);

            ObjectReferenceNode<RodJournalNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        int m_journalIndex = 0;
        double m_angle = 0.0;

        CrankshaftNode *m_crankshaft = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_ROD_JOURNAL_NODE_H */
