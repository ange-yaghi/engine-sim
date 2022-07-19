#ifndef ATG_ENGINE_SIM_ACTIONS_H
#define ATG_ENGINE_SIM_ACTIONS_H

#include "piranha.h"

#include "compiler.h"
#include "object_reference_node.h"
#include "engine_node.h"
#include "crankshaft_node.h"
#include "rod_journal_node.h"
#include "cylinder_bank_node.h"
#include "function_node.h"
#include "camshaft_node.h"

namespace es_script {

    class SetEngineNode : public piranha::Node {
    public:
        SetEngineNode() { /* void */ }
        virtual ~SetEngineNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            registerInput(&m_engineInput, "engine");
        }

        virtual void _evaluate() {
            EngineNode *engineNode = getObject<EngineNode>(m_engineInput);

            Engine *engine = new Engine;
            engineNode->buildEngine(engine);
            Compiler::output()->engine = engine;
        }

    protected:
        piranha::pNodeInput m_engineInput = nullptr;
    };

    class AddRodJournalNode : public Node {
    public:
        AddRodJournalNode() { /* void */ }
        virtual ~AddRodJournalNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("crankshaft", &m_crankshaft, InputTarget::Type::Object);
            addInput("rod_journal", &m_rodJournal, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();
        }

    protected:
        CrankshaftNode *m_crankshaft = nullptr;
        RodJournalNode *m_rodJournal = nullptr;
    };

    class AddCrankshaftNode : public Node {
    public:
        AddCrankshaftNode() { /* void */ }
        virtual ~AddCrankshaftNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("engine", &m_engine, InputTarget::Type::Object);
            addInput("crankshaft", &m_crankshaft, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            m_engine->addCrankshaft(m_crankshaft);
        }

    protected:
        CrankshaftNode *m_crankshaft = nullptr;
        EngineNode *m_engine = nullptr;
    };

    class AddCylinderBankNode : public Node {
    public:
        AddCylinderBankNode() { /* void */ }
        virtual ~AddCylinderBankNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("engine", &m_engine, InputTarget::Type::Object);
            addInput("cylinder_bank", &m_cylinderBankNode, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            m_engine->addCylinderBank(m_cylinderBankNode);
        }

    protected:
        CylinderBankNode *m_cylinderBankNode = nullptr;
        EngineNode *m_engine = nullptr;
    };

    class AddCylinderNode : public Node {
    public:
        AddCylinderNode() { /* void */ }
        virtual ~AddCylinderNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("piston", &m_pistonNode, InputTarget::Type::Object);
            addInput("connecting_rod", &m_connectingRod, InputTarget::Type::Object);
            addInput("rod_journal", &m_rodJournal, InputTarget::Type::Object);
            addInput("cylinder_bank", &m_cylinderBank, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            m_cylinderBank->addCylinder(
                m_pistonNode,
                m_connectingRod,
                m_rodJournal
            );
        }

    protected:
        PistonNode *m_pistonNode = nullptr;
        ConnectingRodNode *m_connectingRod = nullptr;
        RodJournalNode *m_rodJournal = nullptr;
        CylinderBankNode *m_cylinderBank = nullptr;
    };

    class AddSampleNode : public Node {
    public:
        AddSampleNode() { /* void */ }
        virtual ~AddSampleNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("x", &m_x);
            addInput("y", &m_y);
            addInput("function", &m_function, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            m_function->addSample(m_x, m_y);
        }

    protected:
        double m_x = 0;
        double m_y = 0;
        FunctionNode *m_function = nullptr;
    };

    class AddLobeNode : public Node {
    public:
        AddLobeNode() { /* void */ }
        virtual ~AddLobeNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("centerline", &m_centerline);
            addInput("camshaft", &m_camshaft, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            m_camshaft->addLobe(m_centerline);
        }

    protected:
        double m_centerline = 0;
        CamshaftNode *m_camshaft = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_ACTIONS_H */
