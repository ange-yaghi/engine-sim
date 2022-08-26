#ifndef ATG_ENGINE_SIM_ROD_JOURNAL_NODE_H
#define ATG_ENGINE_SIM_ROD_JOURNAL_NODE_H

#include "object_reference_node.h"

#include "engine_sim.h"

#include <map>

namespace es_script {

    class CrankshaftNode;
    class ConnectingRodNode;
    class RodJournalNode : public ObjectReferenceNode<RodJournalNode> {
        friend CrankshaftNode;
        friend ConnectingRodNode;

    public:
        RodJournalNode() { /* void */ }
        virtual ~RodJournalNode() { /* void */ }

        double getAngle() const {
            return m_angle;
        }

        CrankshaftNode *getCrankshaft() const {
            return m_crankshaft;
        }

        ConnectingRodNode *getRod() const {
            return m_rod;
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

        CrankshaftNode *m_crankshaft = nullptr;
        ConnectingRodNode *m_rod = nullptr;
        double m_angle = 0.0;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_ROD_JOURNAL_NODE_H */
