#ifndef ATG_ENGINE_SIM_PERFORMANCE_CLUSTER_H
#define ATG_ENGINE_SIM_PERFORMANCE_CLUSTER_H

#include "ui_element.h"

#include "engine.h"
#include "gauge.h"
#include "cylinder_temperature_gauge.h"
#include "cylinder_pressure_gauge.h"
#include "labeled_gauge.h"
#include "throttle_display.h"
#include "simulator.h"

class PerformanceCluster : public UiElement {
    public:
        PerformanceCluster();
        virtual ~PerformanceCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        void setSimulator(Simulator *simulator) { m_simulator = simulator; }
        void addTimePerTimestepSample(double sample);
        void addAudioLatencySample(double sample);
        void addInputBufferUsageSample(double sample);

        LabeledGauge *m_timePerTimestepGauge;
        LabeledGauge *m_fpsGauge;
        LabeledGauge *m_simSpeedGauge;
        LabeledGauge *m_simulationFrequencyGauge;
        LabeledGauge *m_inputSamplesGauge;
        LabeledGauge *m_audioLagGauge;

    protected:
        double m_timePerTimestep;

        double m_filteredSimulationFrequency;
        double m_audioLatency;
        double m_inputBufferUsage;

        Simulator *m_simulator;
};

#endif /* ATG_ENGINE_SIM_PERFORMANCE_CLUSTER_H */
