#ifndef ATG_ENGINE_SIM_SIMULATOR_H
#define ATG_ENGINE_SIM_SIMULATOR_H

#include "engine.h"
#include "transmission.h"
#include "vehicle.h"
#include "synthesizer.h"
#include "dynamometer.h"
#include "starter_motor.h"
#include "derivative_filter.h"
#include "vehicle_drag_constraint.h"
#include "delay_filter.h"
#include "engine.h"

#include <chrono>

class Simulator {
public:
    enum class SystemType {
        NsvOptimized,
        Generic
    };

    struct Parameters {
        SystemType systemType = SystemType::NsvOptimized;
    };

    static constexpr int DynoTorqueSamples = 512;

public:
    Simulator();
    virtual ~Simulator();

    virtual void initialize(const Parameters &params);
    void loadSimulation(Engine *engine, Vehicle *vehicle, Transmission *transmission);
    void releaseSimulation();

    virtual void startFrame(double dt);
    bool simulateStep();
    virtual double getTotalExhaustFlow() const;
    int readAudioOutput(int samples, int16_t *target);
    virtual void endFrame();
    virtual void destroy();

    void startAudioRenderingThread();
    void endAudioRenderingThread();

    int getFrameIterationCount() const { return m_steps; }

    Synthesizer &synthesizer() { return m_synthesizer; }

    Engine *getEngine() const { return m_engine; }
    Transmission *getTransmission() const { return m_transmission; }
    Vehicle *getVehicle() const { return m_vehicle; }
    atg_scs::RigidBodySystem *getSystem() { return m_system; }

    void setSimulationFrequency(int frequency) { m_simulationFrequency = frequency; }
    int getSimulationFrequency() const { return m_simulationFrequency; }

    double getTimestep() const { return 1.0 / m_simulationFrequency; }
    double getSinceLastUpdate() const { return m_sinceLastUpdate; }
    void resetSinceLastUpdate() { m_sinceLastUpdate = 0.0; }

    void setTargetSynthesizerLatency(double latency) { m_targetSynthesizerLatency = latency; }
    double getTargetSynthesizerLatency() const { return m_targetSynthesizerLatency; }
    double getSynthesizerInputLatency() const { return m_synthesizer.getLatency(); }
    double getSynthesizerInputLatencyTarget() const;

    void setSimulationSpeed(double simSpeed) { m_simulationSpeed = simSpeed; }
    double getSimulationSpeed() const { return m_simulationSpeed; }
    int getCurrentIteration() const { return m_currentIteration; }
    double getAverageProcessingTime() const { return m_physicsProcessingTime; }

    int simulationSteps() const { return m_steps; }

    virtual double getFilteredDynoTorque() const;
    virtual double getDynoPower() const;
    virtual double getAverageOutputSignal() const;

    double filteredEngineSpeed() const { return m_filteredEngineSpeed; }

    Dynamometer m_dyno;
    StarterMotor m_starterMotor;

protected:
    void initializeSynthesizer();
    virtual void simulateStep_();
    virtual void writeToSynthesizer() = 0;

    atg_scs::RigidBodySystem *m_system;

private:
    void updateFilteredEngineSpeed(double dt);

private:
    atg_scs::RigidBody m_vehicleMass;
    VehicleDragConstraint m_vehicleDrag;

    Synthesizer m_synthesizer;

    std::chrono::steady_clock::time_point m_simulationStart;
    std::chrono::steady_clock::time_point m_simulationEnd;
    int m_currentIteration;

    Engine *m_engine;
    Transmission *m_transmission;
    Vehicle *m_vehicle;

    double m_physicsProcessingTime;

    int m_simulationFrequency;
    double m_elapsedTime;
    double m_sinceLastUpdate;

    double m_targetSynthesizerLatency;
    double m_simulationSpeed;

    double *m_dynoTorqueSamples;
    int m_lastDynoTorqueSample;

    double m_filteredEngineSpeed;

    int m_steps;
};

#endif /* ATG_ENGINE_SIM_SIMULATOR_H */
