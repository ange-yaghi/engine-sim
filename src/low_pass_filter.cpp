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
    return fast_f(sample);
}
