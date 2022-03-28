#ifndef ATG_ENGINE_SIM_CYLINDER_TEMPERATURE_GAUGE_H
#define ATG_ENGINE_SIM_CYLINDER_TEMPERATURE_GAUGE_H

#include "ui_element.h"

#include "engine.h"
#include "engine_simulator.h"
#include "gauge.h"

class CylinderTemperatureGauge : public UiElement {
    public:
        CylinderTemperatureGauge();
        virtual ~CylinderTemperatureGauge();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        EngineSimulator *m_simulator;
        double m_maxTemperature;
        double m_minTemperature;
};

#endif /* ATG_ENGINE_SIM_CYLINDER_TEMPERATURE_GAUGE_H */
