#ifndef ATG_ENGINE_SIM_DERIVATIVE_FILTER_H
#define ATG_ENGINE_SIM_DERIVATIVE_FILTER_H

#include "filter.h"

class DerivativeFilter : public Filter {
    public:
        DerivativeFilter();
        virtual ~DerivativeFilter();

        virtual double f(double sample);

        double m_dt;

    protected:
        double m_previous;
};

#endif /* ATG_ENGINE_SIM_DERIVATIVE_FILTER_H */
