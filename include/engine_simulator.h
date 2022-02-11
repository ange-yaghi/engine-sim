#ifndef ATG_ENGINE_SIM_ENGINE_SIMULATOR_H
#define ATG_ENGINE_SIM_ENGINE_SIMULATOR_H

#include "engine.h"
#include "combustion_chamber.h"

#include "scs.h"

class EngineSimulator {
    public:
        EngineSimulator();
        ~EngineSimulator();

        void synthesize(Engine *engine);
        void placeAndInitialize();
        void update(float dt);
        void destroy();

        int m_steps;

    protected:
        atg_scs::RigidBodySystem m_system;

        atg_scs::FixedPositionConstraint *m_crankConstraints;
        atg_scs::LineConstraint *m_cylinderWallConstraints;
        atg_scs::LinkConstraint *m_linkConstraints;

        CombustionChamber *m_combustionChambers;

        Engine *m_engine;
};

#endif /* ATG_ENGINE_SIM_ENGINE_SIMULATOR_H */
