#include "../include/low_pass_filter.h"

LowPassFilter::LowPassFilter() {
    m_y = 0;
    m_rc = 0;
    m_dt = 1 / 44100.0;
}

LowPassFilter::~LowPassFilter() {
    /* void */
}

double LowPassFilter::f(double sample) {
    const double alpha = m_dt / (m_rc + m_dt);
    m_y = alpha * sample + (1 - alpha) * m_y;

    return m_y;
}
