#ifndef ATG_ENGINE_SIM_FUNCTION_H
#define ATG_ENGINE_SIM_FUNCTION_H

#include "gaussian_filter.h"

class Function {
    protected:
        static GaussianFilter *DefaultGaussianFilter;

    public:
        Function();
        virtual ~Function();

        void initialize(int size, double filterRadius, GaussianFilter *filter = nullptr);
        void resize(int newCapacity);
        void destroy();

        void setInputScale(double s) { m_inputScale = s; }
        void setOutputScale(double s) { m_outputScale = s; }
        void addSample(double x, double y);

        double sampleTriangle(double x) const;
        double sampleGaussian(double x) const;
        double triangle(double x) const;
        int closestSample(double x) const;

        bool isOrdered() const;

        void getDomain(double *x0, double *x1);
        void getRange(double *y0, double *y1);

    protected:
        double *m_x;
        double *m_y;

        double m_yMin;
        double m_yMax;
        double m_inputScale;
        double m_outputScale;

        double m_filterRadius;

        int m_capacity;
        int m_size;

        GaussianFilter *m_gaussianFilter;
};

#endif /* ATG_ENGINE_SIM_FUNCTION_H */
