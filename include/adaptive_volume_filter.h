#ifndef ATG_ENGINE_SIM_ADAPTIVE_VOLUME_FILTER_H
#define ATG_ENGINE_SIM_ADAPTIVE_VOLUME_FILTER_H

#include "filter.h"

class AdaptiveVolumeFilter : public Filter {
    public:
        AdaptiveVolumeFilter();
        virtual ~AdaptiveVolumeFilter();

        void initialize(int lookahead);
        virtual double f(double sample);
        virtual void destroy();

    protected:
        double *m_samples;
        int m_sampleCount;
        int m_lookahead;
};

#endif /* ATG_ENGINE_SIM_CONVOLUTION_ADAPTIVE_VOLUME */
