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
            params.rodJournals = (int)m_rodJournals.size();

            crankshaft->initialize(params);
            for (int i = 0; i < params.rodJournals; ++i) {
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
            addInput("throw", &m_parameters.crankThrow);
            addInput("flywheel_mass", &m_parameters.flywheelMass);
            addInput("mass", &m_parameters.mass);
            addInput("friction_torque", &m_parameters.frictionTorque);
            addInput("moment_of_inertia", &m_parameters.momentOfInertia);
            addInput("position_x", &m_parameters.pos_x);
            addInput("position_y", &m_parameters.pos_y);
            addInput("tdc", &m_parameters.tdc);

            ObjectReferenceNode<CrankshaftNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.rodJournals = 0;
        }

        Crankshaft::Parameters m_parameters;
        std::vector<RodJournalNode *> m_rodJournals;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CRANKSHAFT_NODE_H */
