#ifndef ATG_ENGINE_SIM_ENGINE_NODE_H
#define ATG_ENGINE_SIM_ENGINE_NODE_H

#include "object_reference_node.h"

#include "crankshaft_node.h"
#include "cylinder_bank_node.h"

#include "engine_sim.h"

#include <map>

namespace es_script {

    class EngineNode : public ObjectReferenceNode<EngineNode> {
    public:
        EngineNode() { /* void */ }
        virtual ~EngineNode() { /* void */ }

        void buildEngine(Engine *engine) {
            Engine::Parameters parameters = m_parameters;
            parameters.CrankshaftCount = (int)m_crankshafts.size();
            parameters.CylinderBanks = 0;
            parameters.CylinderCount = 0;
            parameters.ExhaustSystemCount = 0;
            parameters.IntakeCount = 0;

            engine->initialize(parameters);

            for (int i = 0; i < parameters.CrankshaftCount; ++i) {
                m_crankshafts[i]->generate(engine->getCrankshaft(i));
            }
        }

        void addCrankshaft(CrankshaftNode *crankshaft) {
            m_crankshafts.push_back(crankshaft);
        }

        void addCylinderBank(CylinderBankNode *bank) {
            m_cylinderBanks.push_back(bank);
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
            Engine::Parameters params;
            params.Fuel.Density = readAtomicInput<double>("fuel_density");
            params.Fuel.EnergyDensity = readAtomicInput<double>("fuel_energy_density");
            params.Fuel.MolecularMass = readAtomicInput<double>("fuel_molecular_mass");
            params.Name = readAtomicInput<std::string>("name");
            params.Redline = readAtomicInput<double>("redline");
            params.StarterSpeed = readAtomicInput<double>("starter_speed");
            params.StarterTorque = readAtomicInput<double>("starter_torque");
        }

        Engine::Parameters m_parameters;
        std::vector<CrankshaftNode *> m_crankshafts;
        std::vector<CylinderBankNode *> m_cylinderBanks;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_ENGINE_NODE_H */
