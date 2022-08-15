#ifndef ATG_ENGINE_SIM_FEEDBACK_COMB_FILTER_H
#define ATG_ENGINE_SIM_FEEDBACK_COMB_FILTER_H

#include "filter.h"

class FeedbackCombFilter : public Filter {
    public:
        FeedbackCombFilter();
        virtual ~FeedbackCombFilter();

        void initialize(int M);
        virtual float f(float sample) override;
        virtual void destroy();

        float a_M;

    protected:
        float *m_y;
        int m_offset;

    protected:
        int M;
};

#endif /* ATG_ENGINE_SIM_FEEDBACK_COMB_FILTER_H */
