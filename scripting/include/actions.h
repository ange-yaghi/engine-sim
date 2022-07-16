#ifndef ATG_ENGINE_SIM_ACTIONS_H
#define ATG_ENGINE_SIM_ACTIONS_H

#include "piranha.h"

#include "compiler.h"
#include "object_reference_node.h"
#include "engine_node.h"

namespace es_script {

    class SetEngineNode : public piranha::Node {
    public:
        SetEngineNode() { /* void */ }
        virtual ~SetEngineNode() { /* void */ }

    protected:
        virtual void registerInputs() {
            registerInput(&m_engineInput, "engine");
        }

        virtual void _evaluate() {
            EngineNode *engineNode = getObject<EngineNode>(m_engineInput);

            Compiler::output()->engine = engineNode->getEngine();
            engineNode->release();
        }

    protected:
        piranha::pNodeInput m_engineInput = nullptr;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_ACTIONS_H */
