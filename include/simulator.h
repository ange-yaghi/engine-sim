#ifndef ATG_ENGINE_SIM_SIMULATOR_H
#define ATG_ENGINE_SIM_SIMULATOR_H

#include "engine.h"
#include "combustion_chamber.h"
#include "crankshaft_load.h"
#include "synthesizer.h"

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

        void initialize(Engine *engine, SystemType systemType);
        void startFrame(double dt);
        bool simulateStep();
        double getTotalExhaustFlow() const;
        int readAudioOutput(int samples, int16_t *target);
        int getFrameIterationCount() const { return i_steps; }
        void endFrame();
        void destroy();

        void setDynoEnable(bool enable) { m_crankshaftLoads[0].m_enableDyno = enable; }
        bool getDynoEnable() const { return m_crankshaftLoads[0].m_enableDyno; }

        void startAudioRenderingThread();
        void endAudioRenderingThread();

        int getSynthesizerInputLatency() const { return m_synthesizer.getInputWriteOffset(); }
        int getSynthesizerInputLatencyTarget() const;

        int getCurrentIteration() const { return m_currentIteration; }

        int i_steps;

        double getAverageProcessingTime() const { return m_physicsProcessingTime; }

        Engine *getEngine() const { return m_engine; }
        CrankshaftLoad *getCrankshaftLoad(int i);
        atg_scs::RigidBodySystem *getSystem() { return m_system; }

        void setGear(int gear);
        void setClutch(double pressure);

        void setSimulationFrequency(int frequency) { m_simulationFrequency = frequency; }
        int getSimulationFrequency() const { return m_simulationFrequency; }

        double getTimestep() const { return 1.0 / m_simulationFrequency; }

        void setTargetSynthesizerLatency(double latency) { m_targetSynthesizerLatency = latency; }
        double getTargetSynthesizerLatency() const { return m_targetSynthesizerLatency; }

        void setSimulationSpeed(double simSpeed) { m_simulationSpeed = simSpeed; }
        double getSimulationSpeed() const { return m_simulationSpeed; }

        double getTravelledDistance() const { return m_travelledDistance; }

        double getVehicleSpeed() const;

    protected:
        void placeAndInitialize();
        void initializeSynthesizer();
        
    protected:
        void writeToSynthesizer();

    protected:
        atg_scs::RigidBodySystem *m_system;
        Synthesizer m_synthesizer;

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

        int m_simulationFrequency;
        double m_targetSynthesizerLatency;
        double m_simulationSpeed;
        double *m_exhaustFlowStagingBuffer;

        double m_travelledDistance;
};

#endif /* ATG_ENGINE_SIM_SIMULATOR_H */
