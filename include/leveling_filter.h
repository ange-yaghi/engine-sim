#ifndef ATG_ENGINE_SIM_LEVELING_FILTER_H
#define ATG_ENGINE_SIM_LEVELING_FILTER_H

#include "filter.h"

class LevelingFilter : public Filter {
    public:
        LevelingFilter();
        virtual ~LevelingFilter();

        virtual double f(double sample);
        double getAttenuation() const { return m_attenuation; }

    protected:
        double m_peak;
        double m_attenuation;

    public:
        double p_maxLevel;
        double p_minLevel;
        double p_target;
};

#endif /* ATG_ENGINE_SIM_LEVELING_FILTER_H */
