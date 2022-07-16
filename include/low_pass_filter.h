#ifndef ATG_ENGINE_SIM_LOW_PASS_FILTER_H
#define ATG_ENGINE_SIM_LOW_PASS_FILTER_H

#include "filter.h"

#include "constants.h"

class LowPassFilter : public Filter {
    public:
        LowPassFilter();
        virtual ~LowPassFilter();

        virtual double f(double sample);

        __forceinline double fast_f(double sample) {
            const double alpha = m_dt / (m_rc + m_dt);
            m_y = alpha * sample + (1 - alpha) * m_y;

            return m_y;
        }

        inline void setCutoffFrequency(double f) {
            m_rc = 1 / (f * 2 * constants::pi);
        }

        double m_dt;

    protected:
        double m_y;
        double m_rc;
};

#endif /* ATG_ENGINE_SIM_LOW_PASS_FILTER_H */

