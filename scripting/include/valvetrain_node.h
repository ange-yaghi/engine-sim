#ifndef ATG_ENGINE_SIM_VALVETRAIN_NODE_H
#define ATG_ENGINE_SIM_VALVETRAIN_NODE_H

#include "object_reference_node.h"

#include "engine_context.h"

namespace es_script {

    class ValvetrainNode : public ObjectReferenceNode<ValvetrainNode> {
    public:
        ValvetrainNode() { /* void */ }
        virtual ~ValvetrainNode() { /* void */ }

        virtual Valvetrain *generate(EngineContext *context, Crankshaft *crank) = 0;

    protected:
        virtual void registerInputs() {
            ObjectReferenceNode<ValvetrainNode>::registerInputs();
        }

        virtual void _evaluate() {
            setOutput(this);
            readAllInputs();
        }
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_VALVETRAIN_NODE_H */
