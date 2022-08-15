#ifndef ATG_ENGINE_SIM_LOW_PASS_FILTER_H
#define ATG_ENGINE_SIM_LOW_PASS_FILTER_H

#include "filter.h"

#include "constants.h"

class LowPassFilter : public Filter {
    public:
        LowPassFilter();
        virtual ~LowPassFilter();

        virtual float f(float sample) override;

        __forceinline float fast_f(float sample) {
            const float alpha = m_dt / (m_rc + m_dt);
            m_y = alpha * sample + (1 - alpha) * m_y;

            return m_y;
        }

        inline void setCutoffFrequency(float f) {
            m_rc = 1.0f / (f * 2.0f * static_cast<float>(constants::pi));
        }

        float m_dt;

    protected:
        float m_y;
        float m_rc;
};

#endif /* ATG_ENGINE_SIM_LOW_PASS_FILTER_H */
