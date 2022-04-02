#ifndef ATG_ENGINE_SIM_CONVOLUTION_FILTER_H
#define ATG_ENGINE_SIM_CONVOLUTION_FILTER_H

#include "filter.h"

class ConvolutionFilter : public Filter {
    public:
        ConvolutionFilter();
        virtual ~ConvolutionFilter();

        void initialize(int samples);
        virtual double f(double sample);
        virtual void destroy();

        int getSampleCount() const { return m_sampleCount; }
        double *getImpulseResponse() { return m_impulseResponse; }

    protected:
        double *m_shiftRegister;
        int m_shiftOffset;

        double *m_impulseResponse;
        int m_sampleCount;
};

#endif /* ATG_ENGINE_SIM_CONVOLUTION_FILTER_H */
