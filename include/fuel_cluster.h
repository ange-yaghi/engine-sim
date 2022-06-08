#ifndef ATG_ENGINE_SIM_FUEL_CLUSTER_H
#define ATG_ENGINE_SIM_FUEL_CLUSTER_H

#include "ui_element.h"

#include "engine.h"

class FuelCluster : public UiElement {
    public:
        FuelCluster();
        virtual ~FuelCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        Engine *m_engine;
};

#endif /* ATG_ENGINE_SIM_FUEL_CLUSTER_H */
