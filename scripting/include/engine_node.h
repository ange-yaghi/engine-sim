#ifndef ATG_ENGINE_SIM_ENGINE_NODE_H
#define ATG_ENGINE_SIM_ENGINE_NODE_H

#include "object_reference_node.h"

#include "engine_sim.h"

#include <map>

namespace es_script {

    class EngineNode : public ObjectReferenceNode<EngineNode> {
    public:
        EngineNode() {
            m_released = false;
            m_engine = nullptr;
        }

        virtual ~EngineNode() {
            /* void */
        }

        Engine *getEngine() { return m_engine; }
        void release() { m_released = true; }

    protected:
        virtual void registerInputs() {
            addInput("name");
            addInput("cylinder_banks");
            addInput("crankshafts");
            addInput("cylinders");
            addInput("exhaust_systems");
            addInput("intakes");
            addInput("starter_torque");
            addInput("starter_speed");
            addInput("redline");

            addInput("fuel_molecular_mass");
            addInput("fuel_energy_density");
            addInput("fuel_density");

            ObjectReferenceNode<EngineNode>::registerInputs();
        }

        virtual void _evaluate() {
            m_engine = new Engine;
            setOutput(this);

            // Read inputs
            Engine::Parameters params;
            params.CrankshaftCount = readAtomicInput<int>("crankshafts");
            params.CylinderBanks = readAtomicInput<int>("cylinder_banks");
            params.CylinderCount = readAtomicInput<int>("cylinders");
            params.ExhaustSystemCount = readAtomicInput<int>("exhaust_systems");
            params.Fuel.Density = readAtomicInput<double>("fuel_density");
            params.Fuel.EnergyDensity = readAtomicInput<double>("fuel_energy_density");
            params.Fuel.MolecularMass = readAtomicInput<double>("fuel_molecular_mass");
            params.IntakeCount = readAtomicInput<int>("intakes");
            params.Name = readAtomicInput<std::string>("name");
            params.Redline = readAtomicInput<double>("redline");
            params.StarterSpeed = readAtomicInput<double>("starter_speed");
            params.StarterTorque = readAtomicInput<double>("starter_torque");

            m_engine->initialize(params);
        }

        virtual void _destroy() {
            if (m_released) {
                delete m_engine;
            }
        }

        Engine *m_engine = nullptr;
        bool m_released = false;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_ENGINE_NODE_H */
