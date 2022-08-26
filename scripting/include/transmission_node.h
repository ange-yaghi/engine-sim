#ifndef ATG_ENGINE_SIM_TRANSMISSION_NODE_H
#define ATG_ENGINE_SIM_TRANSMISSION_NODE_H

#include "object_reference_node.h"

#include "engine_sim.h"

#include <vector>

namespace es_script {

    class TransmissionNode : public ObjectReferenceNode<TransmissionNode> {
    public:
        TransmissionNode() { /* void */ }
        virtual ~TransmissionNode() { /* void */ }

        void generate(Transmission *transmission) const {
            Transmission::Parameters parameters = m_parameters;
            parameters.GearCount = static_cast<int>(m_gears.size());
            parameters.GearRatios = m_gears.data();

            transmission->initialize(parameters);
        }

        void addGear(double ratio) {
            m_gears.push_back(ratio);
        }

    protected:
        virtual void registerInputs() {
            addInput("max_clutch_torque", &m_parameters.MaxClutchTorque);

            ObjectReferenceNode<TransmissionNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);

            // Read inputs
            readAllInputs();
        }

        Transmission::Parameters m_parameters;
        std::vector<double> m_gears;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_TRANSMISSION_NODE_H */
