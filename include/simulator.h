#ifndef ATG_ENGINE_SIM_SIMULATOR_H
#define ATG_ENGINE_SIM_SIMULATOR_H

#include "engine.h"
#include "transmission.h"
#include "combustion_chamber.h"
#include "vehicle.h"
#include "synthesizer.h"
#include "dynamometer.h"
#include "starter_motor.h"
#include "derivative_filter.h"
#include "vehicle_drag_constraint.h"
#include "delay_filter.h"

#include "scs.h"

#include <chrono>

class Simulator {
    public:
        enum class SystemType {
            NsvOptimized,
            Generic
        };

        struct Parameters {
            SystemType SystemType = SystemType::NsvOptimized;
        };

        static constexpr int DynoTorqueSamples = 512;

    public:
        Simulator();
        ~Simulator();

        void initialize(const Parameters &params);
        void loadSimulation(Engine *engine, Vehicle *vehicle, Transmission *transmission);
        void releaseSimulation();

        void startFrame(double dt);
        bool simulateStep();
        double getTotalExhaustFlow() const;
        int readAudioOutput(int samples, int16_t *target);
        int getFrameIterationCount() const { return i_steps; }
        void endFrame();
        void destroy();

        void startAudioRenderingThread();
        void endAudioRenderingThread();

        double getSynthesizerInputLatency() const { return m_synthesizer.getLatency(); }
        double getSynthesizerInputLatencyTarget() const;

        int getCurrentIteration() const { return m_currentIteration; }

        int i_steps;

        double getAverageProcessingTime() const { return m_physicsProcessingTime; }

        Engine *getEngine() const { return m_engine; }
        Transmission *getTransmission() const { return m_transmission; }
        Vehicle *getVehicle() const { return m_vehicle; }
        atg_scs::RigidBodySystem *getSystem() { return m_system; }

        void setSimulationFrequency(int frequency) { m_simulationFrequency = frequency; }
        int getSimulationFrequency() const { return m_simulationFrequency; }

        void setFluidSimulationSteps(int steps) { m_fluidSimulationSteps = steps; }
        int getFluidSimulationSteps() const { return m_fluidSimulationSteps; }
        int getFluidSimulationFrequency() const { return m_fluidSimulationSteps * m_simulationFrequency; }

        double getTimestep() const { return 1.0 / m_simulationFrequency; }

        void setTargetSynthesizerLatency(double latency) { m_targetSynthesizerLatency = latency; }
        double getTargetSynthesizerLatency() const { return m_targetSynthesizerLatency; }

        void setSimulationSpeed(double simSpeed) { m_simulationSpeed = simSpeed; }
        double getSimulationSpeed() const { return m_simulationSpeed; }

        double getFilteredDynoTorque() const;
        double getDynoPower() const;
        double getAverageOutputSignal() const;

        Synthesizer *getSynthesizer() { return &m_synthesizer; }

        Dynamometer m_dyno;
        StarterMotor m_starterMotor;
        DerivativeFilter m_derivativeFilter;

    protected:
        void placeAndInitialize();
        void placeCylinder(int i);
        void initializeSynthesizer();
        
    protected:
        void updateFilteredEngineSpeed(double dt);
        void writeToSynthesizer();

    protected:
        atg_scs::RigidBodySystem *m_system;
        Synthesizer m_synthesizer;

        DelayFilter *m_delayFilters;

        atg_scs::FixedPositionConstraint *m_crankConstraints;
        atg_scs::ClutchConstraint *m_crankshaftLinks;
        atg_scs::RotationFrictionConstraint *m_crankshaftFrictionConstraints;
        atg_scs::LineConstraint *m_cylinderWallConstraints;
        atg_scs::LinkConstraint *m_linkConstraints;
        atg_scs::RigidBody m_vehicleMass;
        VehicleDragConstraint m_vehicleDrag;

        std::chrono::steady_clock::time_point m_simulationStart;
        std::chrono::steady_clock::time_point m_simulationEnd;
        int m_currentIteration;

        Engine *m_engine;
        Transmission *m_transmission;
        Vehicle *m_vehicle;

        double m_physicsProcessingTime;

        int m_simulationFrequency;
        int m_fluidSimulationSteps;

        double m_targetSynthesizerLatency;
        double m_simulationSpeed;
        double *m_exhaustFlowStagingBuffer;
        double m_filteredEngineSpeed;

        double *m_dynoTorqueSamples;
        int m_lastDynoTorqueSample;
};

#endif /* ATG_ENGINE_SIM_SIMULATOR_H */
