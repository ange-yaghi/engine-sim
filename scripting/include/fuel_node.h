#ifndef ATG_ENGINE_SIM_FUEL_NODE_H
#define ATG_ENGINE_SIM_FUEL_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "function_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class FuelNode : public ObjectReferenceNode<FuelNode> {
    public:
        FuelNode() { /* void */ }
        virtual ~FuelNode() { /* void */ }

        void generate(Fuel *fuel, EngineContext *context) const {
            Fuel::Parameters params = m_parameters;
            params.TurbulenceToFlameSpeedRatio =
                m_turbulenceToFlameSpeedRatio->generate(context);

            fuel->initialize(params);
        }

    protected:
        virtual void registerInputs() {
            addInput(
                "turbulence_to_flame_speed_ratio",
                &m_turbulenceToFlameSpeedRatio,
                InputTarget::Type::Object);
            addInput("name", &m_parameters.Name);
            addInput("molecular_mass", &m_parameters.MolecularMass);
            addInput("energy_density", &m_parameters.EnergyDensity);
            addInput("density", &m_parameters.Density);
            addInput("molecular_afr", &m_parameters.MolecularAfr);
            addInput("max_burning_efficiency", &m_parameters.MaxBurningEfficiency);
            addInput("burning_efficiency_randomness", &m_parameters.BurningEfficiencyRandomness);
            addInput("low_efficiency_attenuation", &m_parameters.LowEfficiencyAttenuation);
            addInput("max_turbulence_effect", &m_parameters.MaxTurbulenceEffect);
            addInput("max_dilution_effect", &m_parameters.MaxDilutionEffect);

            ObjectReferenceNode<FuelNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        FunctionNode *m_turbulenceToFlameSpeedRatio = nullptr;
        Fuel::Parameters m_parameters;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_FUEL_NODE_H */
