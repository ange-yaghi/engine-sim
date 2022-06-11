#ifndef ATG_ENGINE_SIM_GUASSIAN_FILTER_H
#define ATG_ENGINE_SIM_GUASSIAN_FILTER_H

#include "scs.h"

#include "crankshaft.h"

class GaussianFilter {
    public:
        GaussianFilter();
        ~GaussianFilter();

        void initialize(double alpha, double radius, int cacheSteps=1024);
        double evaluate(double s) const;

        double getRadius() const { return m_radius; }
        double getAlpha() const { return m_alpha; }

    protected:
        double calculate(double s) const;
        void generateCache();

    protected:
        double *m_cache;

        int m_cacheSteps;
        double m_radius;
        double m_alpha;

        double m_exp_s;
        double m_inv_r;
};

#endif /* ATG_ENGINE_SIM_GAUSSIAN_FILTER_H */
