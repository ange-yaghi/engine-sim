#include "../include/derivative_filter.h"

DerivativeFilter::DerivativeFilter() {
    m_previous = 0;
    m_dt = 0;
}

DerivativeFilter::~DerivativeFilter() {
    /* void */
}

double DerivativeFilter::f(double sample) {
    const double temp = m_previous;
    m_previous = sample;

    return (sample - temp) / m_dt;
}
