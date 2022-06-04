#ifndef ATG_ENGINE_SIM_RIGHT_GAUGE_CLUSTER_H
#define ATG_ENGINE_SIM_RIGHT_GAUGE_CLUSTER_H

#include "ui_element.h"

#include "engine.h"
#include "engine_simulator.h"
#include "gauge.h"
#include "cylinder_temperature_gauge.h"
#include "labeled_gauge.h"
#include "throttle_display.h"

class RightGaugeCluster : public UiElement {
    public:
        RightGaugeCluster();
        virtual ~RightGaugeCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        EngineSimulator *m_simulator;

    protected:
        void renderTachSpeedCluster(const Bounds &bounds);
        void renderFuelAirCluster(const Bounds &bounds);

        LabeledGauge *m_tachometer;
        LabeledGauge *m_speedometer;
        LabeledGauge *m_manifoldVacuumGauge;
        LabeledGauge *m_intakeCfmGauge;
        LabeledGauge *m_volumetricEffGauge;
        ThrottleDisplay *m_throttleDisplay;
        CylinderTemperatureGauge *m_cylinderTemperatureGauge;
};

#endif /* ATG_ENGINE_SIM_GAUGE_CLUSTER_H */
