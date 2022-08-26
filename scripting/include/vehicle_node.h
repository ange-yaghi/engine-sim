#ifndef ATG_ENGINE_SIM_VEHICLE_NODE_H
#define ATG_ENGINE_SIM_VEHICLE_NODE_H

#include "object_reference_node.h"

#include "engine_sim.h"

namespace es_script {

    class VehicleNode : public ObjectReferenceNode<VehicleNode> {
    public:
        VehicleNode() { /* void */ }
        virtual ~VehicleNode() { /* void */ }

        void generate(Vehicle *vehicle) const
        {
            vehicle->initialize(m_parameters);
        }

    protected:
        virtual void registerInputs() {
            addInput("mass", &m_parameters.mass);
            addInput("drag_coefficient", &m_parameters.dragCoefficient);
            addInput("cross_sectional_area", &m_parameters.crossSectionArea);
            addInput("diff_ratio", &m_parameters.diffRatio);
            addInput("tire_radius", &m_parameters.tireRadius);
            addInput("rolling_resistance", &m_parameters.rollingResistance);

            ObjectReferenceNode<VehicleNode>::registerInputs();
        }

        virtual void _evaluate() {           
            setOutput(this);
            // Read inputs
            readAllInputs();

        }

        Vehicle::Parameters m_parameters;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_VEHICLE_NODE_H */
