#ifndef ATG_ENGINE_SIM_BUTTERWORTH_LOW_PASS_FILTER_H
#define ATG_ENGINE_SIM_BUTTERWORTH_LOW_PASS_FILTER_H

#include "filter.h"

#include "constants.h"
#include "ring_buffer.h"

#include <cmath>

template <typename T_Real>
class ButterworthLowPassFilter : public Filter {
public:
    ButterworthLowPassFilter() {
        m_y.initialize(4);
        m_x.initialize(4);

        for (int i = 0; i < 4; ++i) {
            m_y.write(0);
            m_x.write(0);
        }
    }

    virtual ~ButterworthLowPassFilter() {
        /* void */
    }

    __forceinline T_Real fast_f(T_Real sample) {
        const T_Real y_prev[4] = {
            m_y.read(3),
            m_y.read(2),
            m_y.read(1),
            m_y.read(0)
        };

        const T_Real x_prev[4] = {
            m_x.read(3),
            m_x.read(2),
            m_x.read(1),
            m_x.read(0)
        };

        T_Real const n = m_f_4 / m_a[0] * (sample + 4 * x_prev[0] + 6 * x_prev[1] + 4 * x_prev[2] + x_prev[3]);
        T_Real const d = -m_a[1] * y_prev[0] - m_a[2] * y_prev[1] - m_a[3] * y_prev[2] - m_a[4] * y_prev[3];
        T_Real const y = n + d;

        m_x.removeBeginning(1);
        m_x.write(sample);

        m_y.removeBeginning(1);
        m_y.write(y);

        return y;
    }

    inline void setCutoffFrequency(T_Real f_c, T_Real sampleRate) {
        const T_Real f = std::tan(static_cast<T_Real>(constants::pi) * f_c / sampleRate);
        const T_Real f_2 = f * f;
        const T_Real f_3 = f_2 * f;
        const T_Real f_4 = f_2 * f_2;
        const T_Real m = -2.0 * std::cos(5.0 * constants::pi / 8.0);
        const T_Real n = -2.0 * std::cos(7.0 * constants::pi / 8.0);

        m_a[0] = 1.0 + (m + n) * f + (2.0 + n * m) * f_2 + (m + n) * f_3 + f_4;
        m_a[1] = (-4.0 - 2.0 * (n + m) * f + 2.0 * (m + n) * f_3 + 4.0 * f_4) / m_a[0];
        m_a[2] = (6.0 - 2.0 * (2.0 + m * n) * f_2 + 6.0 * f_4) / m_a[0];
        m_a[3] = (-4.0 + 2.0 * (m + n) * f - 2.0 * (m + n) * f_3 + 4.0 * f_4) / m_a[0];
        m_a[4] = (1.0 - (n + m) * f + (2.0 + m * n) * f_2 - (m + n) * f_3 + f_4) / m_a[0];

        m_f_4 = f_4;
    }

protected:
    RingBuffer<T_Real> m_y;
    RingBuffer<T_Real> m_x;
    T_Real m_a[5];
    T_Real m_f_4;
};

#endif /* ATG_ENGINE_SIM_BUTTERWORTH_LOW_PASS_FILTER_H */
