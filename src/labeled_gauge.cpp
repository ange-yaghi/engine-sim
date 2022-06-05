#include "../include/labeled_gauge.h"

#include "../include/engine_sim_application.h"

#include <sstream>
#include <iomanip>

LabeledGauge::LabeledGauge() {
    m_gauge = nullptr;
    m_title = "";
    m_precision = 2;
    m_unit = "";
    m_spaceBeforeUnit = true;
}

LabeledGauge::~LabeledGauge() {
    /* void */
}

void LabeledGauge::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_gauge = addElement<Gauge>();
    m_gauge->m_center = { 0.0f, -20.0f };
}

void LabeledGauge::destroy() {
    UiElement::destroy();
}

void LabeledGauge::update(float dt) {
    UiElement::update(dt);
}

void LabeledGauge::render() {
    drawFrame(m_bounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    const Bounds bounds = m_bounds.inset(m_margin);
    const Bounds title = bounds.verticalSplit(1.0f, 0.9f);
    const Bounds gaugeBounds = bounds.verticalSplit(0.0f, 0.9f);

    drawCenteredText(m_title, title.inset(10.0f), 24.0f);

    const double value = m_gauge->m_value;

    std::stringstream ss;
    ss << std::fixed << std::setprecision(m_precision);
    ss << value << (m_spaceBeforeUnit ? " " : "") << m_unit;
    drawCenteredText(ss.str(), gaugeBounds.verticalSplit(0.0f, 2 / 8.0f), gaugeBounds.height() / 8);

    m_gauge->m_bounds = gaugeBounds;
    m_gauge->setLocalPosition({ 0, 0 });
    m_gauge->m_outerRadius = std::fmin(gaugeBounds.width(), gaugeBounds.height()) / 2.0f;
    m_gauge->m_needleOuterRadius = m_gauge->m_outerRadius * m_needleOuterRadius;
    m_gauge->m_needleInnerRadius = -m_gauge->m_outerRadius * m_needleInnerRadius;

    UiElement::render();
}
