#ifndef ATG_ENGINE_SIM_DELAY_FILTER_H
#define ATG_ENGINE_SIM_DELAY_FILTER_H

#include "filter.h"

#include "ring_buffer.h"

#include <cmath>

class DelayFilter : public Filter {
public:
    DelayFilter() {
        m_latencySamples = 0;
    }

    virtual ~DelayFilter() {
        /* void */
    }

    void initialize(double delay, double audioFrequency) {
        const int samples = static_cast<int>(std::round(delay * audioFrequency));
        const int capacity = samples + 32;

        m_history.initialize(capacity);
        m_latencySamples = samples;
    }

    virtual float f(float sample) override {
        return fast_f(static_cast<double>(sample));
    }

    inline double fast_f(double sample) {
        m_history.write(sample);

        if (m_history.size() <= static_cast<size_t>(m_latencySamples)) {
            return 0;
        }
        else {
            double v;
            m_history.readAndRemove(1, &v);

            return v;
        }
    }

protected:
    int m_latencySamples;
    RingBuffer<double> m_history;
};

#endif /* ATG_ENGINE_SIM_DELAY_FILTER_H */
