#ifndef ATG_ENGINE_SIM_CONVOLUTION_FILTER_H
#define ATG_ENGINE_SIM_CONVOLUTION_FILTER_H

#include "filter.h"

class ConvolutionFilter : public Filter {
    public:
        ConvolutionFilter();
        virtual ~ConvolutionFilter();

        void initialize(int samples);
        virtual float f(float sample) override;
        virtual void destroy();

        int getSampleCount() const { return m_sampleCount; }
        float *getImpulseResponse() { return m_impulseResponse; }

    protected:
        float *m_shiftRegister;
        int m_shiftOffset;

        float *m_impulseResponse;
        int m_sampleCount;
};

#endif /* ATG_ENGINE_SIM_CONVOLUTION_FILTER_H */
