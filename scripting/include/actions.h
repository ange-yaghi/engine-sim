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
#include "cylinder_head_node.h"
#include "cylinder_bank_node.h"
#include "ignition_module_node.h"

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

            m_crankshaft->addRodJournal(m_rodJournal);
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
            addInput("exhaust_system", &m_exhaustSystem, InputTarget::Type::Object);
            addInput("intake", &m_intake, InputTarget::Type::Object);
            addInput("cylinder_bank", &m_cylinderBank, InputTarget::Type::Object);
            addInput("ignition_wire", &m_ignitionWire, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            m_cylinderBank->addCylinder(
                m_pistonNode,
                m_connectingRod,
                m_rodJournal,
                m_intake,
                m_exhaustSystem,
                m_ignitionWire
            );
        }

    protected:
        PistonNode *m_pistonNode = nullptr;
        ConnectingRodNode *m_connectingRod = nullptr;
        RodJournalNode *m_rodJournal = nullptr;
        CylinderBankNode *m_cylinderBank = nullptr;
        ExhaustSystemNode *m_exhaustSystem = nullptr;
        IntakeNode *m_intake = nullptr;
        IgnitionWireNode *m_ignitionWire = nullptr;
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

    class SetCylinderHeadNode : public Node {
    public:
        SetCylinderHeadNode() { /* void */ }
        virtual ~SetCylinderHeadNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("head", &m_head, InputTarget::Type::Object);
            addInput("bank", &m_bank, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            if (m_bank->getCylinderHead() != nullptr) {
                throwError("Cylinder bank already has a head");
                return;
            }
            else if (m_head->getBank() != nullptr) {
                throwError("Cylinder head already attached to a bank");
                return;
            }

            m_bank->setCylinderHead(m_head);
            m_head->setBank(m_bank);
        }

    protected:
        CylinderHeadNode *m_head = nullptr;
        CylinderBankNode *m_bank = nullptr;
    };

    class k_28inH2ONode : public Node {
        class k_28inH2ONodeOutput : public piranha::NodeOutput {
        public:
            k_28inH2ONodeOutput() : NodeOutput(&piranha::FundamentalType::FloatType) {
                m_input = 0.0;
            }

            virtual ~k_28inH2ONodeOutput() {
                /* void */
            }

            virtual void fullCompute(void *target) const {
                *reinterpret_cast<double *>(target) = GasSystem::k_28inH2O(m_input);
            }

            double getInput() const { return m_input; }
            void setInput(double data) { m_input = data; }

        protected:
            double m_input;
        };

    public:
        k_28inH2ONode() { /* void */ }
        virtual ~k_28inH2ONode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("flow", &m_flowInput);

            Node::registerInputs();
        }

        virtual void registerOutputs() {
            registerOutput(&m_output, "__out");

            setPrimaryOutput("__out");
        }

        virtual void _evaluate() {
            readAllInputs();

            m_output.setInput(m_flowInput);
        }

    protected:
        k_28inH2ONodeOutput m_output;
        double m_flowInput = 0.0;
    };

    class k_CarbNode : public Node {
        class k_CarbNodeOutput : public piranha::NodeOutput {
        public:
            k_CarbNodeOutput() : NodeOutput(&piranha::FundamentalType::FloatType) {
                m_input = 0.0;
            }

            virtual ~k_CarbNodeOutput() {
                /* void */
            }

            virtual void fullCompute(void *target) const {
                *reinterpret_cast<double *>(target) = GasSystem::k_carb(m_input);
            }

            double getInput() const { return m_input; }
            void setInput(double data) { m_input = data; }

        protected:
            double m_input;
        };

    public:
        k_CarbNode() { /* void */ }
        virtual ~k_CarbNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("flow", &m_flowInput);

            Node::registerInputs();
        }

        virtual void registerOutputs() {
            registerOutput(&m_output, "__out");

            setPrimaryOutput("__out");
        }

        virtual void _evaluate() {
            readAllInputs();

            m_output.setInput(m_flowInput);
        }

    protected:
        k_CarbNodeOutput m_output;
        double m_flowInput = 0.0;
    };

    class ConnectIgnitionWireNode : public Node {
    public:
        ConnectIgnitionWireNode() { /* void */ }
        virtual ~ConnectIgnitionWireNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("wire", &m_wire, InputTarget::Type::Object);
            addInput("ignition_module", &m_module, InputTarget::Type::Object);
            addInput("angle", &m_angle);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            m_module->connect(m_wire, m_angle);
        }

    protected:
        IgnitionWireNode *m_wire = nullptr;
        IgnitionModuleNode *m_module = nullptr;
        double m_angle = 0.0;
    };

    class AddIgnitionModuleNode : public Node {
    public:
        AddIgnitionModuleNode() { /* void */ }
        virtual ~AddIgnitionModuleNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("engine", &m_engine, InputTarget::Type::Object);
            addInput("ignition_module", &m_ignitionModule, InputTarget::Type::Object);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            m_engine->addIgnitionModule(m_ignitionModule);
        }

    protected:
        IgnitionModuleNode *m_ignitionModule = nullptr;
        EngineNode *m_engine = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_ACTIONS_H */
