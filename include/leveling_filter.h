#ifndef ATG_ENGINE_SIM_LEVELING_FILTER_H
#define ATG_ENGINE_SIM_LEVELING_FILTER_H

#include "filter.h"

#include "function.h"

class LevelingFilter : public Filter {
    public:
        LevelingFilter();
        virtual ~LevelingFilter();

        virtual float f(float sample);
        float getAttenuation() const { return m_attenuation; }

    protected:
        float m_peak;
        float m_attenuation;

    public:
        float p_maxLevel;
        float p_minLevel;
        float p_target;
};

#endif /* ATG_ENGINE_SIM_LEVELING_FILTER_H */
