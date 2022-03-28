#ifndef ATG_ENGINE_SIM_GAUGE_CLUSTER_H
#define ATG_ENGINE_SIM_GAUGE_CLUSTER_H

#include "ui_element.h"

#include "engine.h"
#include "engine_simulator.h"
#include "gauge.h"

class GaugeCluster : public UiElement {
    public:
        GaugeCluster();
        virtual ~GaugeCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        EngineSimulator *m_simulator;

    protected:
        Gauge *m_tachometer;
        Gauge *m_speedometer;
};

#endif /* ATG_ENGINE_SIM_GAUGE_CLUSTER_H */
