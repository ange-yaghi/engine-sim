#ifndef ATG_ENGINE_SIM_FIRING_ORDER_DISPLAY_H
#define ATG_ENGINE_SIM_FIRING_ORDER_DISPLAY_H

#include "ui_element.h"

#include "engine.h"
#include "gauge.h"

class FiringOrderDisplay : public UiElement {
    public:
        FiringOrderDisplay();
        virtual ~FiringOrderDisplay();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        Engine *m_engine;

    protected:
        int m_cylinderCount;
        float *m_cylinderLit;
};

#endif /* ATG_ENGINE_SIM_FIRING_ORDER_DISPLAY_H */
