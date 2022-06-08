#ifndef ATG_ENGINE_SIM_AFR_CLUSTER_H
#define ATG_ENGINE_SIM_AFR_CLUSTER_H

#include "ui_element.h"

#include "engine.h"
#include "gauge.h"
#include "cylinder_temperature_gauge.h"
#include "cylinder_pressure_gauge.h"
#include "labeled_gauge.h"
#include "throttle_display.h"

class AfrCluster : public UiElement {
    public:
        AfrCluster();
        virtual ~AfrCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        Engine *m_engine;

    protected:
        LabeledGauge *m_intakeAfrGauge;
        LabeledGauge *m_exhaustAfrGauge;
};

#endif /* ATG_ENGINE_SIM_AFR_CLUSTER_H */
