#ifndef ATG_ENGINE_SIM_CYLINDER_BANK_NODE_H
#define ATG_ENGINE_SIM_CYLINDER_BANK_NODE_H

#include "object_reference_node.h"

#include "rod_journal_node.h"
#include "piston_node.h"
#include "connecting_rod_node.h"
#include "cylinder_head_node.h"
#include "ignition_wire_node.h"

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
            IntakeNode *intake;
            ExhaustSystemNode *exhaust;
            double soundAttenuation;
            double primaryLength;
        };

    public:
        CylinderBankNode() { /* void */ }
        virtual ~CylinderBankNode() { /* void */ }

        void connectRodAssemblies(EngineContext *context) const {
            const int n = getCylinderCount();
            for (int i = 0; i < n; ++i) {
                ConnectingRodNode *rodNode = m_cylinders[i].rod;
                ConnectingRod *rod = context->getConnectingRod(rodNode);

                RodJournalNode *journal = m_cylinders[i].rodJournal;
                if (journal->getRod() != nullptr) {
                    rod->setMaster(context->getConnectingRod(journal->getRod()));
                    rod->setCrankshaft(rod->getMasterRod()->getCrankshaft());
                }
            }
        }

        void indexSlaveJournals(EngineContext *context) const {
            const int n = getCylinderCount();
            for (int i = 0; i < n; ++i) {
                m_cylinders[i].rod->indexSlaveJournals(context);
            }
        }

        void generate(
            int index,
            int cylinderBaseIndex,
            CylinderBank *cylinderBank,
            Crankshaft *crankshaft,
            Engine *engine,
            EngineContext *context) const 
        {
            CylinderBank::Parameters params = m_parameters;
            params.cylinderCount = static_cast<int>(m_cylinders.size());
            params.index = index;
            params.crankshaft = crankshaft;

            cylinderBank->initialize(params);

            const int n = getCylinderCount();
            for (int i = 0; i < n; ++i) {
                Piston *piston = engine->getPiston(cylinderBaseIndex + i);
                ConnectingRod *rod = engine->getConnectingRod(cylinderBaseIndex + i);
                Crankshaft *crankshaft = context->getCrankshaft(m_cylinders[i].rodJournal->getCrankshaft());

                context->addConnectingRod(m_cylinders[i].rod, rod);

                context->setCylinderIndex(
                    this,
                    i,
                    cylinderBaseIndex + i);

                m_cylinders[i].piston->generate(
                    piston,
                    rod,
                    cylinderBank,
                    i);
                m_cylinders[i].rod->generate(
                    rod,
                    crankshaft,
                    piston,
                    context->getRodJournalIndex(m_cylinders[i].rodJournal));
            }

            CylinderHead *head = context->getHead(m_head);
            m_head->generate(head, cylinderBank, crankshaft, context);

            const int n_cylinders = getCylinderCount();
            for (int i = 0; i < n_cylinders; ++i) {
                ExhaustSystemNode *exhaustNode = getCylinder(i).exhaust;
                IntakeNode *intakeNode = getCylinder(i).intake;

                ExhaustSystem *exhaust = exhaustNode->generate(context);
                Intake *intake = intakeNode->generate(context);

                head->setIntake(i, intake);
                head->setExhaustSystem(i, exhaust);
                head->setSoundAttenuation(i, getCylinder(i).soundAttenuation);
                head->setHeaderPrimaryLength(i, getCylinder(i).primaryLength);
            }
        }

        void addCylinder(
            PistonNode *piston,
            ConnectingRodNode *rod,
            RodJournalNode *rodJournal,
            IntakeNode *intake,
            ExhaustSystemNode *exhaust,
            IgnitionWireNode *wire,
            double soundAttenuation,
            double primaryLength)
        {
            m_cylinders.push_back({
                piston,
                rod,
                rodJournal,
                intake,
                exhaust,
                soundAttenuation,
                primaryLength
            });

            wire->connect(this, getCylinderCount() - 1);
        }

        const Cylinder &getCylinder(int i) const {
            return m_cylinders[i];
        }

        int getCylinderCount() const {
            return (int)m_cylinders.size();
        }

        void setCylinderHead(CylinderHeadNode *head) {
            m_head = head;
        }

        CylinderHeadNode *getCylinderHead() const {
            return m_head;
        }

    protected:
        virtual void registerInputs() {
            addInput("angle", &m_parameters.angle);
            addInput("bore", &m_parameters.bore);
            addInput("deck_height", &m_parameters.deckHeight);
            addInput("position_x", &m_parameters.positionX);
            addInput("position_y", &m_parameters.positionY);
            addInput("display_depth", &m_parameters.displayDepth);

            ObjectReferenceNode<CylinderBankNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();

            m_parameters.cylinderCount = 0;
            m_parameters.index = 0;
        }

        CylinderBank::Parameters m_parameters;
        std::vector<Cylinder> m_cylinders;
        CylinderHeadNode *m_head = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_CYLINDER_BANK_NODE_H */
