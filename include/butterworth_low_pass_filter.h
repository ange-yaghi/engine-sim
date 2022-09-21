#ifndef ATG_ENGINE_SIM_BUTTERWORTH_LOW_PASS_FILTER_H
#define ATG_ENGINE_SIM_BUTTERWORTH_LOW_PASS_FILTER_H

#include "filter.h"

#include "constants.h"
#include "ring_buffer.h"

#include <cmath>

class ButterworthLowPassFilter : public Filter {
public:
    ButterworthLowPassFilter() {
        m_y.initialize(4);
        m_x.initialize(4);

        for (int i = 0; i < 4; ++i) {
            m_y.write(0.0f);
            m_x.write(0.0f);
        }
    }

    virtual ~ButterworthLowPassFilter() {
        /* void */
    }

    virtual float f(float sample) override {
        return fast_f(sample);
    }

    __forceinline float fast_f(float sample) {
        const float y_prev[4] = {
            m_y.read(3),
            m_y.read(2),
            m_y.read(1),
            m_y.read(0)
        };

        const float x_prev[4] = {
            m_x.read(3),
            m_x.read(2),
            m_x.read(1),
            m_x.read(0)
        };

        float const n = m_f_4 / m_a[0] * (sample + 4.0f * x_prev[0] + 6.f * x_prev[1] + 4.f * x_prev[2] + x_prev[3]);
        float const d = -m_a[1] * y_prev[0] - m_a[2] * y_prev[1] - m_a[3] * y_prev[2] - m_a[4] * y_prev[3];
        float const y = n + d;

        m_x.removeBeginning(1);
        m_x.write(sample);

        m_y.removeBeginning(1);
        m_y.write(y);

        return y;
    }

    inline void setCutoffFrequency(float f_c, float sampleRate) {
        const float f = std::tan(static_cast<float>(constants::pi) * f_c / sampleRate);
        const float f_2 = f * f;
        const float f_3 = f_2 * f;
        const float f_4 = f_2 * f_2;
        const float m = -2.0f * std::cos(5.0f * constants::pi / 8.0f);
        const float n = -2.0f * std::cos(7.0f * constants::pi / 8.0f);

        m_a[0] = 1.0f + (m + n) * f + (2.0f + n * m) * f_2 + (m + n) * f_3 + f_4;
        m_a[1] = (-4.0f - 2.0f * (n + m) * f + 2.0f * (m + n) * f_3 + 4.0f * f_4) / m_a[0];
        m_a[2] = (6.0f - 2.0f * (2 + m * n) * f_2 + 6.0f * f_4) / m_a[0];
        m_a[3] = (-4.0f + 2.0f * (m + n) * f - 2.0f * (m + n) * f_3 + 4.0f * f_4) / m_a[0];
        m_a[4] = (1.0f - (n + m) * f + (2.0f + m * n) * f_2 - (m + n) * f_3 + f_4) / m_a[0];

        m_f_4 = f_4;
    }

protected:
    RingBuffer<float> m_y;
    RingBuffer<float> m_x;
    float m_a[5];
    float m_f_4;
};

#endif /* ATG_ENGINE_SIM_BUTTERWORTH_LOW_PASS_FILTER_H */
