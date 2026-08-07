#ifndef ATG_ENGINE_SIM_DYNO_CHART_H
#define ATG_ENGINE_SIM_DYNO_CHART_H

#include "ui_element.h"
#include "oscilloscope.h"

#include <string>

// A styled torque/power-vs-rpm chart: wraps the existing Oscilloscope line
// plot and adds the chrome a plain scope lacks -- gridlines, numeric axis
// labels, a peak-value marker, and a redline marker.
class DynoChart : public UiElement {
    public:
        DynoChart();
        virtual ~DynoChart();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        void addDataPoint(double rpm, double value);
        void reset();

        std::string m_title;
        std::string m_unit;
        int m_precision;
        ysVector m_lineColor;

        double m_xMax;          // rpm axis max
        double m_redlineRpm;    // 0 = no redline marker

        double m_peakValue;
        double m_peakRpm;

    protected:
        Point dataToLocal(double rpm, double value, const Bounds &plotArea) const;

        double m_yMax;
        Oscilloscope *m_scope;
};

#endif /* ATG_ENGINE_SIM_DYNO_CHART_H */
