#ifndef ATG_ENGINE_SIM_CRANKSHAFT_LOAD_H
#define ATG_ENGINE_SIM_CRANKSHAFT_LOAD_H

#include "scs.h"

#include "crankshaft.h"

class CrankshaftLoad : public atg_scs::Constraint {
    public:
        CrankshaftLoad();
        virtual ~CrankshaftLoad();

        void setCrankshaft(Crankshaft *crankshaft);
        virtual void calculate(Output *output, atg_scs::SystemState *state);
        virtual void limit(atg_scs::Matrix *lambda, atg_scs::SystemState *state);

        atg_scs::FixedPositionConstraint *m_bearingConstraint;

        double m_loadTorque;

        bool m_enableDyno;
        double m_dynoSpeed;
        double m_minDynoTorque;
        double m_maxDynoTorque;

    protected:
        Crankshaft *m_crankshaft;
};

#endif /* ATG_ENGINE_SIM_CRANKSHAFT_LOAD_H */
