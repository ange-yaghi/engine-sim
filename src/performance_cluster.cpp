#include "../include/performance_cluster.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"

#include <sstream>

PerformanceCluster::PerformanceCluster() {
    m_timePerTimestep = 0.0;
    m_timePerTimestepGauge = nullptr;
}

PerformanceCluster::~PerformanceCluster() {
    /* void */
}

void PerformanceCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_timePerTimestepGauge = addElement<LabeledGauge>();
    m_timePerTimestepGauge->m_title = "RT/dT";
    m_timePerTimestepGauge->m_unit = " us";
    m_timePerTimestepGauge->m_precision = 1;
    m_timePerTimestepGauge->setLocalPosition({ 0, 0 });
    m_timePerTimestepGauge->m_gauge->m_min = 0;
    m_timePerTimestepGauge->m_gauge->m_max = 100;
    m_timePerTimestepGauge->m_gauge->m_minorStep = 5;
    m_timePerTimestepGauge->m_gauge->m_majorStep = 10;
    m_timePerTimestepGauge->m_gauge->m_maxMinorTick = 7000;
    m_timePerTimestepGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_timePerTimestepGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_timePerTimestepGauge->m_gauge->m_needleWidth = 4.0;
    m_timePerTimestepGauge->m_gauge->m_gamma = 1.0f;
    m_timePerTimestepGauge->m_gauge->m_needleKs = 1000.0f;
    m_timePerTimestepGauge->m_gauge->m_needleKd = 20.0f;
    m_timePerTimestepGauge->m_gauge->setBandCount(0);
}

void PerformanceCluster::destroy() {
    UiElement::destroy();
}

void PerformanceCluster::update(float dt) {
    UiElement::update(dt);
}

void PerformanceCluster::render() {
    Grid grid;
    grid.h_cells = 3;
    grid.v_cells = 2;

    m_timePerTimestepGauge->m_bounds = grid.get(m_bounds, 1, 0);
    m_timePerTimestepGauge->m_gauge->m_value = m_timePerTimestep;

    UiElement::render();
}

void PerformanceCluster::addTimePerTimestepSample(double sample) {
    const double r = 0.95;
    m_timePerTimestep = r * m_timePerTimestep + (1 - r) * sample;
}
