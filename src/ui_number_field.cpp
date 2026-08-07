#include "../include/ui_number_field.h"

#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

#include <cmath>
#include <sstream>
#include <iomanip>

UiNumberField::UiNumberField() {
    m_checkMouse = true;
    m_value = 0.0;
    m_min = 0.0;
    m_max = 1.0;
    m_precision = 2;
    m_dragValue0 = 0.0;
    m_changed = false;
}

UiNumberField::~UiNumberField() {
    /* void */
}

void UiNumberField::update(float dt) {
    m_mouseBounds = m_bounds;
}

void UiNumberField::onMouseDown(const Point &mouseLocal) {
    UiElement::onMouseDown(mouseLocal);
    m_dragValue0 = m_value;
}

void UiNumberField::onDrag(const Point &p0, const Point &mouse0, const Point &mouse) {
    // fmax/fmin rather than std::max/std::min -- this translation unit
    // pulls in Windows.h (via delta.h) without NOMINMAX, and its max/min
    // object-like macros shadow std::max/std::min by name.
    const float span = std::fmax(1.0f, m_bounds.width());
    const double delta = (mouse.x - mouse0.x) / span * (m_max - m_min);

    m_value = std::fmin(m_max, std::fmax(m_min, m_dragValue0 + delta));
    m_changed = true;
}

void UiNumberField::render() {
    const double range = (m_max > m_min) ? (m_max - m_min) : 1.0;
    const float fraction = (float)std::fmin(1.0, std::fmax(0.0, (m_value - m_min) / range));

    const ysVector fillColor = isMouseHeld()
        ? m_app->getHightlight1Color()
        : mix(m_app->getBackgroundColor(), m_app->getForegroundColor(), 0.05f);

    drawPanel(m_bounds);

    if (fraction > 0.0f) {
        const Bounds fillBounds(
            m_bounds.left(), m_bounds.left() + m_bounds.width() * fraction,
            m_bounds.bottom(), m_bounds.top());
        drawBox(fillBounds.inset(1.0f), fillColor);
    }

    std::stringstream label;
    label << m_label;
    drawAlignedText(label.str(), m_bounds.inset(4.0f), 12.0f, Bounds::lm, Bounds::lm);

    std::stringstream value;
    value << std::fixed << std::setprecision(m_precision) << m_value;
    if (!m_unit.empty()) value << " " << m_unit;
    drawAlignedText(value.str(), m_bounds.inset(4.0f), 12.0f, Bounds::rm, Bounds::rm);
}
