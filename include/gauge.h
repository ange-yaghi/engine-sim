#ifndef ATG_ENGINE_SIM_GAUGE_H
#define ATG_ENGINE_SIM_GAUGE_H

#include "ui_element.h"

#include <string>

class Gauge : public UiElement {
    public:
        struct Band {
            ysVector color = ysMath::Constants::One;
            float start = 0.0;
            float end = 0.0;
            float width = 0.0;
            float radial_offset = 0.0f;
            float shorten_start = 0.0f;
            float shorten_end = 0.0f;
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
        float m_gamma;

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

        float m_needleMaxVelocity;
        float m_needleKs;
        float m_needleKd;

        bool m_renderText;

        Point m_center;

    protected:
        float m_needlePosition;
        float m_needleVelocity;

    protected:
        std::vector<Band> m_bands;
};

#endif /* ATG_ENGINE_SIM_GAUGE_H */
