#include "../include/leveling_filter.h"

#include <cmath>

LevelingFilter::LevelingFilter() {
    m_peak = 30000.0;
    m_attenuation = 1.0;
    p_target = 30000.0;
    p_minLevel = 0.0;
    p_maxLevel = 1.0;
}

LevelingFilter::~LevelingFilter() {
    /* void */
}

float LevelingFilter::f(float sample) {
    m_peak = 0.999f * m_peak;
    if (std::abs(sample) > m_peak) {
        m_peak = std::abs(sample);
    }

    const float raw_attenuation = p_target / m_peak;

    float attenuation = raw_attenuation;
    if (attenuation < p_minLevel) attenuation = p_minLevel;
    else if (attenuation > p_maxLevel) attenuation = p_maxLevel;

    m_attenuation = 0.9 * m_attenuation + 0.1 * attenuation;

    return sample * m_attenuation;
}
