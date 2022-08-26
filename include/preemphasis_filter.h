#ifndef ATG_ENGINE_SIM_PREEMPHASIS_FILTER_H
#define ATG_ENGINE_SIM_PREEMPHASIS_FILTER_H

#include "filter.h"

#include "low_pass_filter.h"

#include <random>

class PreemphasisFilter : public Filter {
public:
    PreemphasisFilter() { m_lastSample = 0; }
    virtual ~PreemphasisFilter() {}

    virtual float f(float sample) override { return fast_f(sample); }

    __forceinline float fast_f(float sample) {
        const float s = -0.95f * sample + m_lastSample;

        m_lastSample = sample;
        return s;
    }

protected:
    float m_lastSample;
};

#endif /* ATG_ENGINE_SIM_PREEMPHASIS_FILTER_H */
