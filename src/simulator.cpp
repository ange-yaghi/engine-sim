#include "../include/simulator.h"

Simulator::Simulator() {
    m_engine = nullptr;
    m_vehicle = nullptr;
    m_transmission = nullptr;
    m_system = nullptr;

    m_physicsProcessingTime = 0;

    m_simulationSpeed = 1.0;
    m_targetSynthesizerLatency = 0.1;
    m_simulationFrequency = 10000;
    m_steps = 0;

    m_currentIteration = 0;

    m_filteredEngineSpeed = 0.0;
    m_dynoTorqueSamples = nullptr;
    m_lastDynoTorqueSample = 0;
}

Simulator::~Simulator() {
    assert(m_system == nullptr);
    assert(m_dynoTorqueSamples == nullptr);
}

void Simulator::initialize(const Parameters &params) {
    if (params.systemType == SystemType::NsvOptimized) {
        atg_scs::OptimizedNsvRigidBodySystem *system =
            new atg_scs::OptimizedNsvRigidBodySystem;
        system->initialize(
            new atg_scs::GaussSeidelSleSolver);
        m_system = system;
    }
    else {
        atg_scs::GenericRigidBodySystem *system =
            new atg_scs::GenericRigidBodySystem;
        system->initialize(
            new atg_scs::GaussianEliminationSleSolver,
            new atg_scs::NsvOdeSolver);
        m_system = system;
    }

    m_dynoTorqueSamples = new double[DynoTorqueSamples];
    for (int i = 0; i < DynoTorqueSamples; ++i) {
        m_dynoTorqueSamples[i] = 0.0;
    }
}

void Simulator::loadSimulation(Engine *engine, Vehicle *vehicle, Transmission *transmission) {
    m_engine = engine;
    m_vehicle = vehicle;
    m_transmission = transmission;
}

void Simulator::releaseSimulation() {
    m_synthesizer.endAudioRenderingThread();
    if (m_system != nullptr) m_system->reset();

    destroy();
}

void Simulator::startFrame(double dt) {
    if (m_engine == nullptr) {
        m_steps = 0;
        return;
    }

    m_simulationStart = std::chrono::steady_clock::now();
    m_currentIteration = 0;
    m_synthesizer.setInputSampleRate(m_simulationFrequency * m_simulationSpeed);

    const double timestep = getTimestep();
    m_steps = (int)std::round((dt * m_simulationSpeed) / timestep);

    const double targetLatency = getSynthesizerInputLatencyTarget();
    if (m_synthesizer.getLatency() < targetLatency) {
        m_steps = static_cast<int>((m_steps + 1) * 1.1);
    }
    else if (m_synthesizer.getLatency() > targetLatency) {
        m_steps = static_cast<int>((m_steps - 1) * 0.9);
        if (m_steps < 0) {
            m_steps = 0;
        }
    }

    if (m_steps > 0) {
        for (int i = 0; i < m_engine->getIntakeCount(); ++i) {
            m_engine->getIntake(i)->m_flowRate = 0;
        }
    }
}

bool Simulator::simulateStep() {
    if (getCurrentIteration() >= simulationSteps()) {
        auto s1 = std::chrono::steady_clock::now();

        const long long lastFrame =
            std::chrono::duration_cast<std::chrono::microseconds>(s1 - m_simulationStart).count();
        m_physicsProcessingTime = m_physicsProcessingTime * 0.98 + 0.02 * lastFrame;

        return false;
    }

    const double timestep = getTimestep();
    m_system->process(timestep, 1);

    m_engine->update(timestep);
    m_vehicle->update(timestep);
    m_transmission->update(timestep);

    updateFilteredEngineSpeed(timestep);

    Crankshaft *outputShaft = m_engine->getOutputCrankshaft();
    outputShaft->resetAngle();

    for (int i = 0; i < m_engine->getCrankshaftCount(); ++i) {
        Crankshaft *shaft = m_engine->getCrankshaft(i);

        // Correct drift (temporary hack)
        shaft->m_body.theta = outputShaft->m_body.theta;
    }

    const int index =
        static_cast<int>(std::floor((DynoTorqueSamples - 1) * outputShaft->getCycleAngle() / (4 * constants::pi)));
    const int step = m_engine->isSpinningCw() ? 1 : -1;
    m_dynoTorqueSamples[index] = m_dyno.getTorque();

    if (m_lastDynoTorqueSample != index) {
        for (int i = m_lastDynoTorqueSample + step; i != index; i += step) {
            if (i >= DynoTorqueSamples) {
                i = -1;
                continue;
            }
            else if (i < 0) {
                i = DynoTorqueSamples;
                continue;
            }

            m_dynoTorqueSamples[i] = m_dyno.getTorque();
        }

        m_lastDynoTorqueSample = index;
    }

    simulateStep_();

    writeToSynthesizer();

    ++m_currentIteration;
    return true;
}

double Simulator::getTotalExhaustFlow() const {
    return 0.0;
}

int Simulator::readAudioOutput(int samples, int16_t *target) {
    return m_synthesizer.readAudioOutput(samples, target);
}

void Simulator::endFrame() {
    m_synthesizer.endInputBlock();
}

void Simulator::destroy() {
    if (m_dynoTorqueSamples != nullptr) delete[] m_dynoTorqueSamples;
    m_dynoTorqueSamples = nullptr;
    m_synthesizer.destroy();
}

void Simulator::startAudioRenderingThread() {
    m_synthesizer.startAudioRenderingThread();
}

void Simulator::endAudioRenderingThread() {
    m_synthesizer.endAudioRenderingThread();
}

double Simulator::getSynthesizerInputLatencyTarget() const {
    return m_targetSynthesizerLatency;
}

double Simulator::getFilteredDynoTorque() const {
    if (m_dynoTorqueSamples == nullptr) return 0;

    double averageTorque = 0;
    for (int i = 0; i < DynoTorqueSamples; ++i) {
        averageTorque += m_dynoTorqueSamples[i];
    }

    return averageTorque / DynoTorqueSamples;
}

double Simulator::getDynoPower() const {
    return (m_engine != nullptr)
        ? getFilteredDynoTorque() * m_engine->getSpeed()
        : 0;
}

double Simulator::getAverageOutputSignal() const {
    return 0.0;
}

void Simulator::initializeSynthesizer() {
    Synthesizer::Parameters synthParams;
    synthParams.audioBufferSize = 44100;
    synthParams.audioSampleRate = 44100;
    synthParams.inputBufferSize = 44100;
    synthParams.inputChannelCount = m_engine->getExhaustSystemCount();
    synthParams.inputSampleRate = static_cast<float>(getSimulationFrequency());
    m_synthesizer.initialize(synthParams);
}

void Simulator::simulateStep_() {
}

void Simulator::updateFilteredEngineSpeed(double dt) {
    const double alpha = dt / (100 + dt);
    m_filteredEngineSpeed = alpha * m_filteredEngineSpeed + (1 - alpha) * m_engine->getRpm();
}
