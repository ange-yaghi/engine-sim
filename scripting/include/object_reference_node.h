#ifndef ATG_ENGINE_SIM_OBJECT_REFERENCE_H
#define ATG_ENGINE_SIM_OBJECT_REFERENCE_H

#include "node.h"

#include "object_reference_node_output.h"

namespace es_script {

    template <typename Type>
    class ObjectReferenceNode : public Node {
    public:
        ObjectReferenceNode() {
            /* void */
        }

        ~ObjectReferenceNode() {
            /* void */
        }

        template <typename Type>
        void overrideType() {
            m_output.overrideType(LookupChannelType<Type>());
        }

    protected:
        virtual void registerOutputs() {
            setPrimaryOutput("__out");
            registerOutput(&m_output, "__out");
        }

        virtual void _evaluate() {
            m_output.setReference(nullptr);
        }

        void setOutput(Type *output) { m_output.setReference(output); }

        ObjectReferenceNodeOutput<Type> m_output;
    };

    template <typename T_ReferenceType>
    using NullReferenceNode = ObjectReferenceNode<T_ReferenceType>;

} /* namespace manta */

#endif /* ATG_ENGINE_SIM_OBJECT_REFERENCE_H */
