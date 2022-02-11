#ifndef ATG_ENGINE_SIM_CRANKSHAFT_FRICTION_H
#define ATG_ENGINE_SIM_CRANKSHAFT_FRICTION_H

#include "scs.h"

#include "crankshaft.h"

class CrankshaftFriction : public atg_scs::ForceGenerator {
    public:
        CrankshaftFriction();
        virtual ~CrankshaftFriction();

        virtual void apply(atg_scs::SystemState *system);

        Crankshaft *m_crankshaft;
        
        double m_friction;
        double m_damping;
};

#endif /* ATG_ENGINE_SIM_CRANKSHAFT_FRICTION_H */
