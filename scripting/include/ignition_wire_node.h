#ifndef ATG_ENGINE_SIM_IGNITION_WIRE_NODE_H
#define ATG_ENGINE_SIM_IGNITION_WIRE_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"
#include "function_node.h"

#include "engine_sim.h"

#include <set>

namespace es_script {

    class IgnitionWireNode : public ObjectReferenceNode<IgnitionWireNode> {
    public:
        using Connection = std::pair<CylinderBankNode *, int>;

    public:
        IgnitionWireNode() { /* void */ }
        virtual ~IgnitionWireNode() { /* void */ }

        void connect(CylinderBankNode *bank, int i) {
            m_connections.insert({ bank, i });
        }

        std::set<Connection> getConnections() const {
            return m_connections;
        }

    protected:
        virtual void registerInputs() {
            ObjectReferenceNode<IgnitionWireNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);
        }

        std::set<Connection> m_connections;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_IGNITION_WIRE_NODE_H */
