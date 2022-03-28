#include "../include/gauge_cluster.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"

#include <sstream>

GaugeCluster::GaugeCluster() {
    m_simulator = nullptr;
    m_tachometer = nullptr;
    m_speedometer = nullptr;
}

GaugeCluster::~GaugeCluster() {
    /* void */
}

void GaugeCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_tachometer = addElement<Gauge>();
    m_speedometer = addElement<Gauge>();
}

void GaugeCluster::destroy() {
    UiElement::destroy();
}

void GaugeCluster::update(float dt) {
    UiElement::update(dt);
}

void GaugeCluster::render() {
    drawFrame(m_bounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    const Bounds title = m_bounds.verticalSplit(1.0f, 0.9f);
    const Bounds tach = m_bounds.verticalSplit(0.45f, 0.9f);
    const Bounds speed = m_bounds.verticalSplit(0.0f, 0.45f);

    drawCenteredText("RPM // Speed", title.inset(10.0f), 24.0f);

    Engine *engine = m_simulator->getEngine();

    const double rpm = engine->getRpm();

    std::stringstream ss;
    ss << std::lroundf(rpm);
    drawCenteredText(ss.str(), tach.verticalSplit(0.0f, 2 / 6.0f), tach.height() / 6);

    m_tachometer->m_bounds = tach;
    m_tachometer->setLocalPosition({ 0, 0 });
    m_tachometer->m_min = 0;
    m_tachometer->m_max = 7000;
    m_tachometer->m_minorStep = 100;
    m_tachometer->m_majorStep = 1000;
    m_tachometer->m_maxMinorTick = 7000;
    m_tachometer->m_thetaMin = Constants::pi * 1.2;
    m_tachometer->m_thetaMax = -0.2 * Constants::pi;
    m_tachometer->m_outerRadius = std::fmin(tach.width(), tach.height()) / 2.0f;
    m_tachometer->m_value = rpm;
    m_tachometer->m_needleOuterRadius = m_tachometer->m_outerRadius * 0.7f;
    m_tachometer->m_needleInnerRadius = -m_tachometer->m_outerRadius * 0.1f;
    m_tachometer->m_needleWidth = 4.0;
    m_tachometer->m_gamma = 1.0f;
    m_tachometer->m_needleKs = 1000.0f;
    m_tachometer->m_needleKd = 20.0f;
    m_tachometer->setBandCount(3);
    m_tachometer->setBand({ ysMath::Constants::One, 400, 1000, 3.0f }, 0);
    m_tachometer->setBand({ ysColor::srgbiToLinear(0xFF8224), 5000, 5500, 3.0f }, 1);
    m_tachometer->setBand({ ysColor::srgbiToLinear(0xFF4040), 5500, 7000, 3.0f }, 2);

    UiElement::render();
}
