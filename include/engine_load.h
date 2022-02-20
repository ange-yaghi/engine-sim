#ifndef ATG_ENGINE_SIM_ENGINE_LOAD_H
#define ATG_ENGINE_SIM_ENGINE_LOAD_H

#include "scs.h"

#include "crankshaft.h"

class EngineLoad : public atg_scs::ForceGenerator {
    public:
        EngineLoad();
        virtual ~EngineLoad();

        virtual void apply(atg_scs::SystemState *system);

        Crankshaft *m_crankshaft;

        double m_torque;
};

#endif /* ATG_ENGINE_SIM_ENGINE_LOAD_H */
