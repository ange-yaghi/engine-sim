#ifndef ATG_ENGINE_SIM_ENGINE_SIMULATOR_H
#define ATG_ENGINE_SIM_ENGINE_SIMULATOR_H

#include "engine.h"
#include "combustion_chamber.h"
#include "crankshaft_friction.h"

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

        double getAverageProcessingTime() const { return m_physicsProcessingTime; }

        Engine *getEngine() const { return m_engine; }
        CombustionChamber *getCombustionChamber(int i);

    protected:
        atg_scs::RigidBodySystem m_system;

        atg_scs::FixedPositionConstraint *m_crankConstraints;
        atg_scs::LineConstraint *m_cylinderWallConstraints;
        atg_scs::LinkConstraint *m_linkConstraints;

        CombustionChamber *m_combustionChambers;
        CrankshaftFriction *m_crankshaftFrictionGenerators;

        Engine *m_engine;

        double m_physicsProcessingTime;
};

#endif /* ATG_ENGINE_SIM_ENGINE_SIMULATOR_H */
