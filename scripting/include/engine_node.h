#ifndef ATG_ENGINE_SIM_ENGINE_NODE_H
#define ATG_ENGINE_SIM_ENGINE_NODE_H

#include "object_reference_node.h"

#include "crankshaft_node.h"
#include "cylinder_bank_node.h"
#include "ignition_module_node.h"
#include "engine_context.h"

#include "engine_sim.h"

#include <map>
#include <set>

namespace es_script {

    class EngineNode : public ObjectReferenceNode<EngineNode> {
    public:
        EngineNode() { /* void */ }
        virtual ~EngineNode() { /* void */ }

        void buildEngine(Engine *engine) {
            int cylinderCount = 0;
            for (const CylinderBankNode *bank : m_cylinderBanks) {
                cylinderCount += bank->getCylinderCount();
            }

            std::set<ExhaustSystemNode *> exhaustSystems;
            std::set<IntakeNode *> intakes;
            for (const CylinderBankNode *bank : m_cylinderBanks) {
                const int n = bank->getCylinderCount();
                for (int i = 0; i < n; ++i) {
                    exhaustSystems.insert(bank->getCylinder(i).exhaust);
                    intakes.insert(bank->getCylinder(i).intake);
                }
            }

            EngineContext context;
            Engine::Parameters parameters = m_parameters;
            parameters.CrankshaftCount = (int)m_crankshafts.size();
            parameters.CylinderBanks = (int)m_cylinderBanks.size();
            parameters.CylinderCount = cylinderCount;
            parameters.ExhaustSystemCount = (int)exhaustSystems.size();
            parameters.IntakeCount = (int)intakes.size();
            engine->initialize(parameters);

            {
                int i = 0;
                for (ExhaustSystemNode *exhaust : exhaustSystems) {
                    context.addExhaust(
                        exhaust, engine->getExhaustSystem(i++));
                }
            }

            {
                int i = 0;
                for (IntakeNode *intake : intakes) {
                    context.addIntake(
                        intake, engine->getIntake(i++));
                }
            }

            {
                int i = 0;
                for (const CylinderBankNode *bank : m_cylinderBanks) {
                    context.addHead(bank->getCylinderHead(), engine->getHead(i++));
                }
            }

            for (const CylinderBankNode *bank : m_cylinderBanks) {
                const int n = bank->getCylinderCount();
                for (int i = 0; i < n; ++i) {
                    exhaustSystems.insert(bank->getCylinder(i).exhaust);
                    intakes.insert(bank->getCylinder(i).intake);
                }
            }

            for (int i = 0; i < parameters.CrankshaftCount; ++i) {
                m_crankshafts[i]->generate(engine->getCrankshaft(i), &context);
            }

            int cylinderIndex = 0;
            for (int i = 0; i < parameters.CylinderBanks; ++i) {
                m_cylinderBanks[i]->generate(
                    i,
                    cylinderIndex,
                    engine->getCylinderBank(i),
                    engine->getCrankshaft(0),
                    engine,
                    &context);
                cylinderIndex += m_cylinderBanks[i]->getCylinderCount();
            }

            m_ignitionModule->generate(engine, &context);
            
            // TEMP
            Function *turbulenceToFlameSpeedRatio = new Function;
            Function *equivalenceRatioToLaminarFlameSpeed = new Function;

            equivalenceRatioToLaminarFlameSpeed->initialize(12, 0.1);
            equivalenceRatioToLaminarFlameSpeed->addSample(0.8, units::distance(22, units::cm) / units::sec);
            equivalenceRatioToLaminarFlameSpeed->addSample(0.9, units::distance(27, units::cm) / units::sec);
            equivalenceRatioToLaminarFlameSpeed->addSample(1.0, units::distance(32, units::cm) / units::sec);
            equivalenceRatioToLaminarFlameSpeed->addSample(1.1, units::distance(35, units::cm) / units::sec);
            equivalenceRatioToLaminarFlameSpeed->addSample(1.2, units::distance(33, units::cm) / units::sec);
            equivalenceRatioToLaminarFlameSpeed->addSample(1.3, units::distance(30, units::cm) / units::sec);
            equivalenceRatioToLaminarFlameSpeed->addSample(1.4, units::distance(25, units::cm) / units::sec);

            turbulenceToFlameSpeedRatio->initialize(10, 10.0);
            turbulenceToFlameSpeedRatio->addSample(0.0, 1.0);
            turbulenceToFlameSpeedRatio->addSample(5.0, 1.5 * 5.0);
            turbulenceToFlameSpeedRatio->addSample(10.0, 1.5 * 10.0);
            turbulenceToFlameSpeedRatio->addSample(15.0, 1.5 * 15.0);
            turbulenceToFlameSpeedRatio->addSample(20.0, 1.5 * 20.0);
            turbulenceToFlameSpeedRatio->addSample(25.0, 1.5 * 25.0);
            turbulenceToFlameSpeedRatio->addSample(30.0, 1.5 * 30.0);
            turbulenceToFlameSpeedRatio->addSample(35.0, 1.5 * 35.0);
            turbulenceToFlameSpeedRatio->addSample(40.0, 1.5 * 40.0);
            turbulenceToFlameSpeedRatio->addSample(45.0, 1.5 * 45.0);

            Fuel::Parameters fParams;
            fParams.TurbulenceToFlameSpeedRatio = turbulenceToFlameSpeedRatio;
            Fuel *fuel = new Fuel;
            fuel->initialize(fParams);

            Function *meanPistonSpeedToTurbulence = new Function;
            meanPistonSpeedToTurbulence->initialize(30, 1);
            for (int i = 0; i < 30; ++i) {
                const double s = (double)i;
                meanPistonSpeedToTurbulence->addSample(s, s * 0.5);
            }

            CombustionChamber::Parameters ccParams;
            ccParams.CrankcasePressure = units::pressure(1.0, units::atm);
            ccParams.Fuel = fuel;
            ccParams.StartingPressure = units::pressure(1.0, units::atm);
            ccParams.StartingTemperature = units::celcius(25.0);
            ccParams.MeanPistonSpeedToTurbulence = meanPistonSpeedToTurbulence;

            for (int i = 0; i < engine->getCylinderCount(); ++i) {
                ccParams.Piston = engine->getPiston(i);
                ccParams.Head = engine->getHead(ccParams.Piston->getCylinderBank()->getIndex());
                engine->getChamber(i)->initialize(ccParams);
            }
        }

        void addCrankshaft(CrankshaftNode *crankshaft) {
            m_crankshafts.push_back(crankshaft);
        }

        void addCylinderBank(CylinderBankNode *bank) {
            m_cylinderBanks.push_back(bank);
        }

        int getIgnitionModuleCount() const {
            return m_ignitionModule == nullptr
                ? 0
                : 1;
        }

        void addIgnitionModule(IgnitionModuleNode *ignitionModule) {
            m_ignitionModule = ignitionModule;
        }

    protected:
        virtual void registerInputs() {
            addInput("name", &m_parameters.Name);
            addInput("starter_torque", &m_parameters.StarterTorque);
            addInput("starter_speed", &m_parameters.StarterSpeed);
            addInput("redline", &m_parameters.Redline);

            addInput("fuel_molecular_mass", &m_parameters.Fuel.MolecularMass);
            addInput("fuel_energy_density", &m_parameters.Fuel.EnergyDensity);
            addInput("fuel_density", &m_parameters.Fuel.Density);

            ObjectReferenceNode<EngineNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        IgnitionModuleNode *m_ignitionModule = nullptr;

        Engine::Parameters m_parameters;
        std::vector<CrankshaftNode *> m_crankshafts;
        std::vector<CylinderBankNode *> m_cylinderBanks;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_ENGINE_NODE_H */
