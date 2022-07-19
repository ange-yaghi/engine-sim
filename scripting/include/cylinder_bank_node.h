#ifndef ATG_ENGINE_SIM_CYLINDER_BANK_NODE_H
#define ATG_ENGINE_SIM_CYLINDER_BANK_NODE_H

#include "object_reference_node.h"

#include "rod_journal_node.h"
#include "piston_node.h"
#include "connecting_rod_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class CylinderBankNode : public ObjectReferenceNode<CylinderBankNode> {
    public:
        struct Cylinder {
            PistonNode *piston;
            ConnectingRodNode *rod;
            RodJournalNode *rodJournal;
        };

    public:
        CylinderBankNode() { /* void */ }
        virtual ~CylinderBankNode() { /* void */ }

        void generate(
            int index,
            int cylinderBaseIndex,
            CylinderBank *cylinderBank,
            Crankshaft *crankshaft,
            Engine *engine,
            EngineContext *context) const 
       {
            CylinderBank::Parameters params = m_parameters;
            params.CylinderCount = (int)m_cylinders.size();
            params.Index = index;

            cylinderBank->initialize(params);

            const int n = getCylinderCount();
            for (int i = 0; i < n; ++i) {
                Piston *piston = engine->getPiston(cylinderBaseIndex + i);
                ConnectingRod *rod = engine->getConnectingRod(cylinderBaseIndex + i);

                m_cylinders[i].piston->generate(
                    piston,
                    rod,
                    cylinderBank,
                    cylinderBaseIndex + 1);
                m_cylinders[i].rod->generate(
                    rod,
                    crankshaft,
                    context->getRodJournalIndex(m_cylinders[i].rodJournal));
            }
        }

        void addCylinder(
            PistonNode *piston,
            ConnectingRodNode *rod,
            RodJournalNode *rodJournal)
        {
            m_cylinders.push_back({ piston, rod, rodJournal });
        }

        int getCylinderCount() const {
            return (int)m_cylinders.size();
        }

    protected:
        virtual void registerInputs() {
            addInput("angle", &m_parameters.Angle);
            addInput("bore", &m_parameters.Bore);
            addInput("deck_height", &m_parameters.DeckHeight);

            ObjectReferenceNode<CylinderBankNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.CylinderCount = 0;
            m_parameters.Index = 0;
        }

        CylinderBank::Parameters m_parameters;
        std::vector<Cylinder> m_cylinders;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CYLINDER_BANK_NODE_H */
