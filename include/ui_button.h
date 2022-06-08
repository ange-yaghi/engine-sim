#ifndef ATG_ENGINE_SIM_UI_BUTTON_H
#define ATG_ENGINE_SIM_UI_BUTTON_H

#include "ui_element.h"

class UiButton : public UiElement {
    public:
        UiButton();
        virtual ~UiButton();

        virtual void update(float dt);
        virtual void render();

        std::string m_text;
        float m_fontSize;
};

#endif /* ATG_ENGINE_SIM_UI_BUTTON_H */
