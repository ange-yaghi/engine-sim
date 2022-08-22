#ifndef ATG_ENGINE_SIM_SET_VEHICLE_NODE_H
#define ATG_ENGINE_SIM_SET_VEHICLE_NODE_H

#include "node.h"

#include "piranha.h"
#include "compiler.h"
#include "engine_sim.h"
#include "vehicle_node.h"

namespace es_script {

    class SetVehicle : public piranha::Node {
    public:
        SetVehicle() { /* void */ }
        virtual ~SetVehicle() { /* void */ }

    protected:
        virtual void registerInputs() {
            registerInput(&m_vehicleInput, "vehicle");

            Node::registerInputs();
        }

        virtual void _evaluate() {
            VehicleNode* vehicleNode = getObject<VehicleNode>(m_vehicleInput);

            Vehicle* vehicle= new Vehicle;
            vehicleNode->generate(vehicle);
            Compiler::output()->vehicle = vehicle;
        }

    protected:
        piranha::pNodeInput m_vehicleInput = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_SET_VEHICLE_NODE_H */
