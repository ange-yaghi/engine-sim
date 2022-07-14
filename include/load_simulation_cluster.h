#ifndef ATG_ENGINE_SIM_LOAD_SIMULATION_CLUSTER_H
#define ATG_ENGINE_SIM_LOAD_SIMULATION_CLUSTER_H

#include "ui_element.h"

#include "simulator.h"
#include "labeled_gauge.h"

class LoadSimulationCluster : public UiElement {
    public:
        LoadSimulationCluster();
        virtual ~LoadSimulationCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        void setSimulator(Simulator *simulator) { m_simulator = simulator; }

    protected:
        void drawCurrentGear(const Bounds &bounds);
        void drawClutchPressureGauge(const Bounds &bounds);
        void drawSystemStatus(const Bounds &bounds);
        void updateHpAndTorque(float dt);

        float m_systemStatusLights[3];
        LabeledGauge *m_dynoSpeedGauge;
        LabeledGauge *m_torqueGauge;
        LabeledGauge *m_hpGauge;
        LabeledGauge *m_clutchPressureGauge;

        double m_filteredHorsepower;
        double m_filteredTorque;
        double m_peakHorsepower;
        double m_peakTorque;

        Simulator *m_simulator;
};

#endif /* ATG_ENGINE_SIM_LOAD_SIMULATION_CLUSTER_H */
