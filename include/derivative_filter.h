#ifndef ATG_ENGINE_SIM_DERIVATIVE_FILTER_H
#define ATG_ENGINE_SIM_DERIVATIVE_FILTER_H

#include "filter.h"

class DerivativeFilter : public Filter {
    public:
        DerivativeFilter();
        virtual ~DerivativeFilter();

        virtual float f(float sample) override;

        float m_dt;

    protected:
        float m_previous;
};

#endif /* ATG_ENGINE_SIM_DERIVATIVE_FILTER_H */
