#ifndef ATG_ENGINE_SIM_SIMULATOR_H
#define ATG_ENGINE_SIM_SIMULATOR_H

#include "engine.h"
#include "combustion_chamber.h"
#include "crankshaft_load.h"

#include "scs.h"

#include <chrono>

class Simulator {
    public:
        enum class SystemType {
            NsvOptimized,
            Generic
        };

    public:
        Simulator();
        ~Simulator();

        void synthesize(Engine *engine, SystemType systemType);
        void placeAndInitialize();

        void start();
        bool simulateStep(double dt);
        void destroy();

        int getCurrentIteration() const { return m_currentIteration; }

        int i_steps;

        double getAverageProcessingTime() const { return m_physicsProcessingTime; }

        Engine *getEngine() const { return m_engine; }
        CrankshaftLoad *getCrankshaftLoad(int i);
        atg_scs::RigidBodySystem *getSystem() { return m_system; }

        void setGear(int gear);
        void setClutch(double pressure);

    protected:
        atg_scs::RigidBodySystem *m_system;

        atg_scs::FixedPositionConstraint *m_crankConstraints;
        atg_scs::LineConstraint *m_cylinderWallConstraints;
        atg_scs::LinkConstraint *m_linkConstraints;
        atg_scs::ClutchConstraint m_clutchConstraint;
        atg_scs::RigidBody m_vehicleMass;

        CrankshaftLoad *m_crankshaftLoads;

        std::chrono::steady_clock::time_point m_simulationStart;
        std::chrono::steady_clock::time_point m_simulationEnd;
        int m_currentIteration;

        Engine *m_engine;

        double m_physicsProcessingTime;
};

#endif /* ATG_ENGINE_SIM_SIMULATOR_H */
