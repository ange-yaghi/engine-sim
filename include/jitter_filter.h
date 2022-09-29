#ifndef ATG_ENGINE_SIM_JITTER_FILTER_H
#define ATG_ENGINE_SIM_JITTER_FILTER_H

#include "filter.h"

#include "butterworth_low_pass_filter.h"
#include "utilities.h"

#include <random>

class JitterFilter : public Filter {
public:
    JitterFilter();
    virtual ~JitterFilter();

    void initialize(
        int maxJitter,
        float noiseCutoffFrequency,
        float audioFrequency);
    virtual float f(float sample) override;

    __forceinline float fast_f(float sample, float jitterScale = 1.0f) {
        m_history[m_offset] = sample;
        ++m_offset;

        if (m_offset >= m_maxJitter) {
            m_offset = 0;
        }

        std::uniform_real_distribution<float> dist(
            0.0f,
            static_cast<float>(m_maxJitter - 1));
        
        const float s = m_noiseFilter.fast_f(dist(m_generator) * m_jitterScale * jitterScale);
        const float s_i_0 = clamp(std::floor(s), 0.0f, static_cast<float>(m_maxJitter - 1));
        const float s_i_1 = clamp(std::ceil(s), 0.0f, static_cast<float>(m_maxJitter - 1));

        const float s_frac = (s - s_i_0);

        const int i_0 = static_cast<int>(s_i_0) + m_offset;
        const int i_1 = static_cast<int>(s_i_1) + m_offset;

        const float v0 = m_history[i_0 >= m_maxJitter ? i_0 - m_maxJitter : i_0];
        const float v1 = m_history[i_1 >= m_maxJitter ? i_1 - m_maxJitter : i_1];

        return v1 * s_frac + v0 * (1 - s_frac);
    }

    inline void setJitterScale(float jitterScale) { m_jitterScale = jitterScale; }
    inline float getJitterScale() const { return m_jitterScale; }

protected:
    ButterworthLowPassFilter<float> m_noiseFilter;

    float m_jitterScale;
    int m_maxJitter;
    int m_offset;
    float *m_history;

    std::default_random_engine m_generator;
};

#endif /* ATG_ENGINE_SIM_JITTER_FILTER_H */
