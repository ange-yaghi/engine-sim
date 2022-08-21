#ifndef ATG_ENGINE_SIM_SET_APPLICATION_SETTINGS_NODE_H
#define ATG_ENGINE_SIM_SET_APPLICATION_SETTINGS_NODE_H

#include "node.h"

#include "piranha.h"
#include "compiler.h"
#include "engine_sim.h"

namespace es_script {

    class SetApplicationSettings : public Node {
    public:
        SetApplicationSettings() { /* void */ }
        virtual ~SetApplicationSettings() { /* void */ }

    protected:
        virtual void registerInputs() {
            addInput("start_fullscreen", &m_settings.startFullscreen);

            Node::registerInputs();
        }

        virtual void _evaluate() {
            readAllInputs();

            Compiler::output()->applicationSettings = m_settings;
        }

    protected:
        ApplicationSettings m_settings;
    };

} /* namespace es_script */

#endif /* ATG_ENGINE_SIM_SET_APPLICATION_SETTINGS_NODE_H */
