#ifndef ATG_ENGINE_SIM_DYNO_CLUSTER_H
#define ATG_ENGINE_SIM_DYNO_CLUSTER_H

#include "ui_element.h"
#include "dyno_chart.h"
#include "ui_button.h"
#include "simulator.h"

#include <string>

// The realistic-dyno-sheet screen: two stacked torque/power charts plus a
// big colored peak-stat row, modeled on a real dyno printout. Labeled as
// engine-level torque/power (not wheel/flywheel) since engine-sim has no
// drivetrain model to derive those from.
class DynoCluster : public UiElement {
    public:
        DynoCluster();
        virtual ~DynoCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();
        virtual void signal(UiElement *element, Event event);

        void setSimulator(Simulator *simulator) { m_simulator = simulator; }
        void setUnits();
        void newPull();

    protected:
        DynoChart *m_torqueChart;
        DynoChart *m_powerChart;
        UiButton *m_newPullButton;

        Simulator *m_simulator;
        std::string m_torqueUnits;
        std::string m_powerUnits;

        double m_filteredTorque;
        double m_filteredPower;

        float m_updatePeriod;
        float m_updateTimer;
};

#endif /* ATG_ENGINE_SIM_DYNO_CLUSTER_H */
