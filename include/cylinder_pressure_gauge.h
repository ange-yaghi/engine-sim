#ifndef ATG_ENGINE_SIM_UI_CYLINDER_PRESSURE_GAUGE_H
#define ATG_ENGINE_SIM_UI_CYLINDER_PRESSURE_GAUGE_H

#include "ui_element.h"

#include "engine.h"
#include "engine_simulator.h"

class CylinderPressureGauge : public UiElement {
    public:
        CylinderPressureGauge();
        virtual ~CylinderPressureGauge();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        EngineSimulator *m_simulator;
};

#endif /* ATG_ENGINE_SIM_UI_CYLINDER_PRESSURE_GAUGE_H */
