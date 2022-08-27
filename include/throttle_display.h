#ifndef ATG_ENGINE_SIM_THROTTLE_DISPLAY_H
#define ATG_ENGINE_SIM_THROTTLE_DISPLAY_H

#include "ui_element.h"

#include "engine.h"
#include "geometry_generator.h"

class ThrottleDisplay : public UiElement {
    public:
        ThrottleDisplay();
        virtual ~ThrottleDisplay();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        Engine *m_engine;

    protected:
        void renderThrottle(const Bounds &bounds);
        void renderSpeedControl(const Bounds &bounds);
};

#endif /* ATG_ENGINE_SIM_THROTTLE_DISPLAY_H */
