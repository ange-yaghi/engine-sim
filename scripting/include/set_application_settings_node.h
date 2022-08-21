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
            addInput("power_units", &m_settings.powerUnits);
            addInput("torque_units", &m_settings.torqueUnits);
            addInput("speed_units", &m_settings.speedUnits);
            addInput("pressure_units", &m_settings.pressureUnits);
            addInput("boost_units", &m_settings.boostUnits);
            
            addInput("color_background", &m_settings.colorBackground);
            addInput("color_foreground", &m_settings.colorForeground);
            addInput("color_shadow", &m_settings.colorShadow);
            addInput("color_highlight1", &m_settings.colorHighlight1);
            addInput("color_highlight2", &m_settings.colorHighlight2);
            addInput("color_pink", &m_settings.colorPink);
            addInput("color_red", &m_settings.colorRed);
            addInput("color_orange", &m_settings.colorOrange);
            addInput("color_yellow", &m_settings.colorYellow);
            addInput("color_blue", &m_settings.colorBlue);
            addInput("color_green", &m_settings.colorGreen);


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
