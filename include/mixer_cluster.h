#ifndef ATG_ENGINE_SIM_MIXER_CLUSTER_H
#define ATG_ENGINE_SIM_MIXER_CLUSTER_H

#include "ui_element.h"

#include "engine.h"
#include "gauge.h"
#include "cylinder_temperature_gauge.h"
#include "cylinder_pressure_gauge.h"
#include "labeled_gauge.h"
#include "throttle_display.h"
#include "simulator.h"

class MixerCluster : public UiElement {
    public:
        MixerCluster();
        virtual ~MixerCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        void setSimulator(Simulator *simulator) { m_simulator = simulator; }

    protected:
        Simulator *m_simulator;

    protected:
        LabeledGauge
            *m_volumeGauge,
            *m_convolutionGauge,
            *m_highFreqFilterGauge,
            *m_levelerGauge,
            *m_noise0Gauge,
            *m_noise1Gauge;
};

#endif /* ATG_ENGINE_SIM_MIXER_CLUSTER_H */
