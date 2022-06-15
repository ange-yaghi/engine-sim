#include "../include/performance_cluster.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"

#include <sstream>

PerformanceCluster::PerformanceCluster() {
    m_timePerTimestepGauge = nullptr;
    m_timePerTimestepGauge = nullptr;
    m_fpsGauge = nullptr;
    m_simSpeedGauge = nullptr;
    m_simulationFrequencyGauge = nullptr;
    m_inputSamplesGauge = nullptr;
    m_audioLagGauge = nullptr;

    m_timePerTimestep = 0.0;
    m_filteredSimulationFrequency = 0.0;
    m_inputBufferUsage = 0.0;
    m_audioLatency = 0.0;
}

PerformanceCluster::~PerformanceCluster() {
    /* void */
}

void PerformanceCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);

    m_timePerTimestepGauge = addElement<LabeledGauge>();
    m_timePerTimestepGauge->m_title = "RT/dT";
    m_timePerTimestepGauge->m_unit = "";
    m_timePerTimestepGauge->m_precision = 1;
    m_timePerTimestepGauge->setLocalPosition({ 0, 0 });
    m_timePerTimestepGauge->m_gauge->m_min = 0;
    m_timePerTimestepGauge->m_gauge->m_max = 200;
    m_timePerTimestepGauge->m_gauge->m_minorStep = 5;
    m_timePerTimestepGauge->m_gauge->m_majorStep = 10;
    m_timePerTimestepGauge->m_gauge->m_maxMinorTick = 1000000;
    m_timePerTimestepGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_timePerTimestepGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_timePerTimestepGauge->m_gauge->m_needleWidth = 4.0;
    m_timePerTimestepGauge->m_gauge->m_gamma = 1.0f;
    m_timePerTimestepGauge->m_gauge->m_needleKs = 1000.0f;
    m_timePerTimestepGauge->m_gauge->m_needleKd = 20.0f;
    m_timePerTimestepGauge->m_gauge->setBandCount(3);
    m_timePerTimestepGauge->m_gauge->setBand(
        { m_app->getOrange(), 50.0f, 100.0f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
    m_timePerTimestepGauge->m_gauge->setBand(
        { m_app->getRed(), 100.0f, 200.0f, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 1);
    m_timePerTimestepGauge->m_gauge->setBand(
        { m_app->getBlue(), 0.0f, 50.0f, 3.0f, 6.0f, -shortenAngle, shortenAngle }, 2);

    m_fpsGauge = addElement<LabeledGauge>();
    m_fpsGauge->m_title = "FPS";
    m_fpsGauge->m_unit = "";
    m_fpsGauge->m_precision = 1;
    m_fpsGauge->setLocalPosition({ 0, 0 });
    m_fpsGauge->m_gauge->m_min = 0;
    m_fpsGauge->m_gauge->m_max = 120;
    m_fpsGauge->m_gauge->m_minorStep = 1;
    m_fpsGauge->m_gauge->m_majorStep = 15;
    m_fpsGauge->m_gauge->m_maxMinorTick = 60;
    m_fpsGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_fpsGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_fpsGauge->m_gauge->m_needleWidth = 4.0;
    m_fpsGauge->m_gauge->m_gamma = 0.6f;
    m_fpsGauge->m_gauge->m_needleKs = 1000.0f;
    m_fpsGauge->m_gauge->m_needleKd = 20.0f;
    m_fpsGauge->m_gauge->setBandCount(5);
    m_fpsGauge->m_gauge->setBand(
        { m_app->getGreen(), 58, 62, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
    m_fpsGauge->m_gauge->setBand(
        { m_app->getBlue(), 62, 120, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 1);
    m_fpsGauge->m_gauge->setBand(
        { m_app->getOrange(), 30, 58, 3.0f, 6.0f, shortenAngle, shortenAngle }, 2);
    m_fpsGauge->m_gauge->setBand(
        { m_app->getRed(), 0, 30, 3.0f, 6.0f, -shortenAngle, shortenAngle }, 3);
    m_fpsGauge->m_gauge->setBand(
        { m_app->getWhite(), 60, 120, 3.0f, 0.0f, -shortenAngle, shortenAngle }, 4);

    m_simSpeedGauge = addElement<LabeledGauge>();
    m_simSpeedGauge->m_title = "1 / SPEED";
    m_simSpeedGauge->m_unit = "";
    m_simSpeedGauge->m_spaceBeforeUnit = false;
    m_simSpeedGauge->m_precision = 1;
    m_simSpeedGauge->setLocalPosition({ 0, 0 });
    m_simSpeedGauge->m_gauge->m_min = 0;
    m_simSpeedGauge->m_gauge->m_max = 1000;
    m_simSpeedGauge->m_gauge->m_minorStep = 10;
    m_simSpeedGauge->m_gauge->m_majorStep = 50;
    m_simSpeedGauge->m_gauge->m_maxMinorTick = 1000;
    m_simSpeedGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_simSpeedGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_simSpeedGauge->m_gauge->m_needleWidth = 4.0;
    m_simSpeedGauge->m_gauge->m_gamma = 1.0f;
    m_simSpeedGauge->m_gauge->m_needleKs = 1000.0f;
    m_simSpeedGauge->m_gauge->m_needleKd = 20.0f;
    m_simSpeedGauge->m_gauge->setBandCount(0);

    m_audioLagGauge = addElement<LabeledGauge>();
    m_audioLagGauge->m_title = "LATENCY";
    m_audioLagGauge->m_unit = "";
    m_audioLagGauge->m_spaceBeforeUnit = false;
    m_audioLagGauge->m_precision = 1;
    m_audioLagGauge->setLocalPosition({ 0, 0 });
    m_audioLagGauge->m_gauge->m_min = 50;
    m_audioLagGauge->m_gauge->m_max = 150;
    m_audioLagGauge->m_gauge->m_minorStep = 5;
    m_audioLagGauge->m_gauge->m_majorStep = 10;
    m_audioLagGauge->m_gauge->m_maxMinorTick = 1000;
    m_audioLagGauge->m_gauge->m_thetaMin = constants::pi * 0.8;
    m_audioLagGauge->m_gauge->m_thetaMax = 0.2 * constants::pi;
    m_audioLagGauge->m_gauge->m_needleWidth = 4.0;
    m_audioLagGauge->m_gauge->m_gamma = 1.0f;
    m_audioLagGauge->m_gauge->m_needleKs = 1000.0f;
    m_audioLagGauge->m_gauge->m_needleKd = 20.0f;
    m_audioLagGauge->m_gauge->setBandCount(3);
    m_audioLagGauge->m_gauge->setBand(
        { m_app->getWhite(), 90, 110, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
    m_audioLagGauge->m_gauge->setBand(
        { m_app->getRed(), 50, 90, 3.0f, 6.0f, -shortenAngle, shortenAngle }, 1);
    m_audioLagGauge->m_gauge->setBand(
        { m_app->getOrange(), 110, 150, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 2);

    m_inputSamplesGauge = addElement<LabeledGauge>();
    m_inputSamplesGauge->m_title = "IN. BUFFER";
    m_inputSamplesGauge->m_unit = "";
    m_inputSamplesGauge->m_spaceBeforeUnit = false;
    m_inputSamplesGauge->m_precision = 1;
    m_inputSamplesGauge->setLocalPosition({ 0, 0 });
    m_inputSamplesGauge->m_gauge->m_min = 0;
    m_inputSamplesGauge->m_gauge->m_max = 200;
    m_inputSamplesGauge->m_gauge->m_minorStep = 5;
    m_inputSamplesGauge->m_gauge->m_majorStep = 10;
    m_inputSamplesGauge->m_gauge->m_maxMinorTick = 1000;
    m_inputSamplesGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_inputSamplesGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_inputSamplesGauge->m_gauge->m_needleWidth = 4.0;
    m_inputSamplesGauge->m_gauge->m_gamma = 1.0f;
    m_inputSamplesGauge->m_gauge->m_needleKs = 1000.0f;
    m_inputSamplesGauge->m_gauge->m_needleKd = 20.0f;
    m_inputSamplesGauge->m_gauge->setBandCount(3);
    m_inputSamplesGauge->m_gauge->setBand(
        { m_app->getWhite(), 90, 110, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
    m_inputSamplesGauge->m_gauge->setBand(
        { m_app->getRed(), 0, 10, 3.0f, 6.0f, -shortenAngle, shortenAngle }, 1);
    m_inputSamplesGauge->m_gauge->setBand(
        { m_app->getOrange(), 150, 200, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 2);

    m_simulationFrequencyGauge = addElement<LabeledGauge>();
    m_simulationFrequencyGauge->m_title = "FREQUENCY";
    m_simulationFrequencyGauge->m_unit = "hz";
    m_simulationFrequencyGauge->m_precision = 0;
    m_simulationFrequencyGauge->setLocalPosition({ 0, 0 });
    m_simulationFrequencyGauge->m_gauge->m_min = 1000;
    m_simulationFrequencyGauge->m_gauge->m_max = 50000;
    m_simulationFrequencyGauge->m_gauge->m_minorStep = 1000;
    m_simulationFrequencyGauge->m_gauge->m_majorStep = 10000;
    m_simulationFrequencyGauge->m_gauge->m_maxMinorTick = 50000;
    m_simulationFrequencyGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_simulationFrequencyGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_simulationFrequencyGauge->m_gauge->m_needleWidth = 4.0;
    m_simulationFrequencyGauge->m_gauge->m_gamma = 0.9f;
    m_simulationFrequencyGauge->m_gauge->m_needleKs = 1000.0f;
    m_simulationFrequencyGauge->m_gauge->m_needleKd = 20.0f;
    m_simulationFrequencyGauge->m_gauge->setBandCount(1);
    m_simulationFrequencyGauge->m_gauge->setBand(
        { m_app->getWhite(), 11025, 44100, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
}

void PerformanceCluster::destroy() {
    UiElement::destroy();
}

void PerformanceCluster::update(float dt) {
    UiElement::update(dt);

    m_filteredSimulationFrequency =
        0.9 * m_filteredSimulationFrequency + 0.1 * m_simulator->getSimulationFrequency() * m_simulator->getSimulationSpeed();
}

void PerformanceCluster::render() {
    Grid grid;
    grid.h_cells = 3;
    grid.v_cells = 2;

    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);
    const double idealTimePerTimestep = (1.0 / m_filteredSimulationFrequency);
    m_timePerTimestepGauge->m_bounds = grid.get(m_bounds, 1, 0);
    m_timePerTimestepGauge->m_gauge->m_value = (m_timePerTimestep / idealTimePerTimestep) * 100;

    m_fpsGauge->m_bounds = grid.get(m_bounds, 0, 0);
    m_fpsGauge->m_gauge->m_value = m_app->getEngine()->GetAverageFramerate();

    m_simSpeedGauge->m_bounds = grid.get(m_bounds, 2, 0);
    m_simSpeedGauge->m_gauge->m_value = 1 / m_simulator->getSimulationSpeed();

    m_audioLagGauge->m_bounds = grid.get(m_bounds, 0, 1);
    m_audioLagGauge->m_gauge->m_value = m_audioLatency * 100;

    m_inputSamplesGauge->m_bounds = grid.get(m_bounds, 1, 1);
    m_inputSamplesGauge->m_gauge->m_value = m_inputBufferUsage * 100;

    m_simulationFrequencyGauge->m_bounds = grid.get(m_bounds, 2, 1);
    m_simulationFrequencyGauge->m_gauge->m_value = m_simulator->getSimulationFrequency();

    UiElement::render();
}

void PerformanceCluster::addTimePerTimestepSample(double sample) {
    const double r = 0.95;
    m_timePerTimestep = r * m_timePerTimestep + (1 - r) * sample;
}

void PerformanceCluster::addAudioLatencySample(double sample) {
    const double r = 0.95;
    m_audioLatency = r * m_audioLatency + (1 - r) * sample;
}

void PerformanceCluster::addInputBufferUsageSample(double sample) {
    const double r = 0.95;
    m_inputBufferUsage = r * m_inputBufferUsage + (1 - r) * sample;
}
