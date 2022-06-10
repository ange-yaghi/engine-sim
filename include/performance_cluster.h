#ifndef ATG_ENGINE_SIM_PERFORMANCE_CLUSTER_H
#define ATG_ENGINE_SIM_PERFORMANCE_CLUSTER_H

#include "ui_element.h"

#include "engine.h"
#include "gauge.h"
#include "cylinder_temperature_gauge.h"
#include "cylinder_pressure_gauge.h"
#include "labeled_gauge.h"
#include "throttle_display.h"

class PerformanceCluster : public UiElement {
    public:
        PerformanceCluster();
        virtual ~PerformanceCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        void addTimePerTimestepSample(double sample);

        LabeledGauge *m_timePerTimestepGauge;

    protected:
        double m_timePerTimestep;
};

#endif /* ATG_ENGINE_SIM_PERFORMANCE_CLUSTER_H */
