#ifndef ATG_ENGINE_SIM_VEHICLE_NODE_H
#define ATG_ENGINE_SIM_VEHICLE_NODE_H

#include "object_reference_node.h"

#include "rod_journal_node.h"
#include "vehicle_node.h"

#include "engine_sim.h"

#include <map>
#include <vector>

namespace es_script {

    class VehicleNode : public ObjectReferenceNode<VehicleNode> {
    public:
        VehicleNode() { /* void */ }
        virtual ~VehicleNode() { /* void */ }

        void generate(Vehicle* vehicle) const
        {
            Vehicle::Parameters params = m_parameters;

            vehicle->initialize(params);
        }

    protected:
        virtual void registerInputs() {
            addInput("mass", &m_parameters.Mass);
            addInput("drag_coefficient", &m_parameters.DragCoefficient);
            addInput("cross_sectional_area", &m_parameters.CrossSectionArea);
            addInput("diff_ratio", &m_parameters.DiffRatio);
            addInput("tire_radius", &m_parameters.TireRadius);
            addInput("rolling_resistance", &m_parameters.RollingResistance);


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
