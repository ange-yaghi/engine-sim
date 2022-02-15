#ifndef ATG_ENGINE_SIM_GAUGE_H
#define ATG_ENGINE_SIM_GAUGE_H

#include "ui_element.h"

#include "engine.h"
#include "engine_simulator.h"

class Gauge : public UiElement {
    public:
        struct Band {
            ysVector color;
            float start;
            float end;
            float width;
        };

    public:
        Gauge();
        virtual ~Gauge();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        void setBandCount(int bands) { m_bands.resize(bands); }
        void setBand(const Band &band, int index) { m_bands[index] = band; }

        float m_value;
        float m_thetaMin;
        float m_thetaMax;

        int m_min;
        int m_max;
        int m_maxMinorTick;

        int m_minorStep;
        int m_majorStep;

        float m_minorTickWidth;
        float m_majorTickWidth;

        float m_minorTickLength;
        float m_majorTickLength;

        float m_outerRadius;

        float m_needleInnerRadius;
        float m_needleOuterRadius;
        float m_needleWidth;

        bool m_renderText;

        Point m_center;

    protected:
        std::vector<Band> m_bands;
};

#endif /* ATG_ENGINE_SIM_GAUGE_H */
