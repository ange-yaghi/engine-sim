#ifndef ATG_ENGINE_SIM_ENGINE_SIMULATOR_H
#define ATG_ENGINE_SIM_ENGINE_SIMULATOR_H

#include "engine.h"
#include "combustion_chamber.h"
#include "crankshaft_load.h"

#include "scs.h"

class EngineSimulator {
    public:
        enum class SystemType {
            NsvOptimized,
            Generic
        };

    public:
        EngineSimulator();
        ~EngineSimulator();

        void synthesize(Engine *engine, SystemType systemType);
        void placeAndInitialize();
        void update(float dt);
        void destroy();

        int m_steps;

        double getAverageProcessingTime() const { return m_physicsProcessingTime; }

        Engine *getEngine() const { return m_engine; }
        CombustionChamber *getCombustionChamber(int i);
        CrankshaftLoad *getCrankshaftLoad(int i);
        atg_scs::RigidBodySystem *getSystem() { return m_system; }

    protected:
        atg_scs::RigidBodySystem *m_system;

        atg_scs::FixedPositionConstraint *m_crankConstraints;
        atg_scs::LineConstraint *m_cylinderWallConstraints;
        atg_scs::LinkConstraint *m_linkConstraints;

        CombustionChamber *m_combustionChambers;
        CrankshaftLoad *m_crankshaftLoads;

        Engine *m_engine;

        double m_physicsProcessingTime;
};

#endif /* ATG_ENGINE_SIM_ENGINE_SIMULATOR_H */
