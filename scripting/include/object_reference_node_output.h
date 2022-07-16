#ifndef ATG_ENGINE_SIM_OBJECT_REFERENCE_NODE_OUTPUT_H
#define ATG_ENGINE_SIM_OBJECT_REFERENCE_NODE_OUTPUT_H

#include "piranha.h"

#include "channel_types.h"

namespace es_script {

    template <typename Type>
    class ObjectReferenceNodeOutput : public piranha::NodeOutput {
    public:
        ObjectReferenceNodeOutput() : NodeOutput(LookupChannelType<Type>()) {
            m_data = nullptr;
        }

        virtual ~ObjectReferenceNodeOutput() {
            /* void */
        }

        virtual void fullCompute(void *target) const {
            *reinterpret_cast<Type **>(target) = m_data;
        }

        Type *getReference() const { return m_data; }
        void setReference(Type *data) { m_data = data; }

    protected:
        Type *m_data;
    };

    template <typename Type>
    Type *getObject(piranha::pNodeInput input) {
        return static_cast<ObjectReferenceNodeOutput<Type> *>(input)->getReference();
    }

} /* namespace manta */

#endif /* ATG_ENGINE_SIM_OBJECT_REFERENCE_NODE_OUTPUT_H */
