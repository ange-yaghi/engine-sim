#ifndef ATG_ENGINE_SIM_CYLINDER_TEMPERATURE_GAUGE_H
#define ATG_ENGINE_SIM_CYLINDER_TEMPERATURE_GAUGE_H

#include "ui_element.h"

#include "engine.h"
#include "gauge.h"

class CylinderTemperatureGauge : public UiElement {
    public:
        CylinderTemperatureGauge();
        virtual ~CylinderTemperatureGauge();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        Engine *m_engine;
        double m_maxTemperature;
        double m_minTemperature;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_TEMPERATURE_GAUGE_H */
