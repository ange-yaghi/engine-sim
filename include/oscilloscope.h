#ifndef ATG_ENGINE_SIM_OSCILLOSCOPE_H
#define ATG_ENGINE_SIM_OSCILLOSCOPE_H

#include "ui_element.h"

class Oscilloscope : public UiElement {
    public:
        struct DataPoint {
            double x, y;
        };

    public:
        Oscilloscope();
        virtual ~Oscilloscope();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        Point dataPointToRenderPosition(const DataPoint &p) const;

        void addDataPoint(double x, double y);

        void setBufferSize(int n);
        void reset();

        double m_xMin;
        double m_xMax;

        double m_yMin;
        double m_yMax;

        double m_lineWidth;

    protected:
        DataPoint *m_points;
        Point *m_renderBuffer;
        int m_writeIndex;
        int m_bufferSize;
        int m_pointCount;
};

#endif /* ATG_ENGINE_SIM_OSCILLOSCOPE_H */
