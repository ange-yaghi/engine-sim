#ifndef ATG_ENGINE_SIM_CRANKSHAFT_NODE_H
#define ATG_ENGINE_SIM_CRANKSHAFT_NODE_H

#include "object_reference_node.h"

#include "rod_journal_node.h"
#include "engine_context.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class CrankshaftNode : public ObjectReferenceNode<CrankshaftNode> {
    public:
        CrankshaftNode() { /* void */ }
        virtual ~CrankshaftNode() { /* void */ }

        void addRodJournal(RodJournalNode *rodJournal) {
            m_rodJournals.push_back(rodJournal);
            rodJournal->m_crankshaft = this;
        }

        void generate(Crankshaft *crankshaft, EngineContext *context) {
            Crankshaft::Parameters params = m_parameters;
            params.RodJournals = (int)m_rodJournals.size();

            crankshaft->initialize(params);
            for (int i = 0; i < params.RodJournals; ++i) {
                RodJournalNode *rodJournal = m_rodJournals[i];
                crankshaft->setRodJournalAngle(
                    i,
                    rodJournal->getAngle()
                );
                context->addRodJournal(rodJournal, i);
            }

            context->addCrankshaft(this, crankshaft);
        }

    protected:
        virtual void registerInputs() {
            addInput("throw", &m_parameters.CrankThrow);
            addInput("flywheel_mass", &m_parameters.FlywheelMass);
            addInput("mass", &m_parameters.Mass);
            addInput("friction_torque", &m_parameters.FrictionTorque);
            addInput("moment_of_inertia", &m_parameters.MomentOfInertia);
            addInput("position_x", &m_parameters.Pos_x);
            addInput("position_y", &m_parameters.Pos_y);
            addInput("tdc", &m_parameters.TDC);

            ObjectReferenceNode<CrankshaftNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.RodJournals = 0;
        }

        Crankshaft::Parameters m_parameters;
        std::vector<RodJournalNode *> m_rodJournals;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CRANKSHAFT_NODE_H */
