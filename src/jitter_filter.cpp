#include "../include/jitter_filter.h"

JitterFilter::JitterFilter() {
    m_history = nullptr;
    m_maxJitter = 0;
    m_offset = 0;
}

JitterFilter::~JitterFilter() {
    /* void */
}

void JitterFilter::initialize(
    int maxJitter,
    float cutoffFrequency,
    float audioFrequency)
{
    m_maxJitter = maxJitter;

    m_history = new float[maxJitter];
    m_offset = 0;
    memset(m_history, 0, sizeof(float) * maxJitter);

    m_noiseFilter.setCutoffFrequency(cutoffFrequency);
    m_noiseFilter.m_dt = 1 / audioFrequency;
}

float JitterFilter::f(float sample) {
    return fast_f(sample);
}
