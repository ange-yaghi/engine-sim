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

            ObjectReferenceNode<CylinderBankNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.Crankshaft = nullptr;
            m_parameters.Piston = nullptr;

            m_rodJournal->addRod(this);
        }

        CylinderBank::Parameters m_parameters;
        std::vector<Cylinder> m_cylinders;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CYLINDER_BANK_NODE_H */
