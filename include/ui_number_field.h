#ifndef ATG_ENGINE_SIM_UI_NUMBER_FIELD_H
#define ATG_ENGINE_SIM_UI_NUMBER_FIELD_H

#include "ui_element.h"

#include <string>

// Combined slider + numeric readout (design doc's Slider widget, doubling as
// the NumberField display -- keyboard text entry doesn't exist anywhere in
// this UI toolkit yet, so exact-value typing is out of scope here; drag is
// the only input method).
class UiNumberField : public UiElement {
    public:
        UiNumberField();
        virtual ~UiNumberField();

        virtual void update(float dt);
        virtual void render();

        virtual void onMouseDown(const Point &mouseLocal);
        virtual void onDrag(const Point &p0, const Point &mouse0, const Point &mouse);

        // True for exactly the frame(s) the value changed via drag; caller
        // is responsible for consuming/clearing it.
        bool consumeChanged() {
            const bool c = m_changed;
            m_changed = false;
            return c;
        }

        std::string m_label;
        std::string m_unit;
        double m_value;
        double m_min, m_max;
        int m_precision;

    protected:
        double m_dragValue0;
        bool m_changed;
};

#endif /* ATG_ENGINE_SIM_UI_NUMBER_FIELD_H */
