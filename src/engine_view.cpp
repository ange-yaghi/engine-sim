#include "../include/engine_view.h"

#include "../include/engine_sim_application.h"

EngineView::EngineView() {
    m_pan = { 0, 0 };
    m_checkMouse = true;
    m_lastScroll = 0;
    m_zoom = 1.0f;
}

EngineView::~EngineView() {
    /* void */
}

void EngineView::update(float dt) {
    m_mouseBounds = m_bounds;
}

void EngineView::render() {
    drawFrame(m_bounds, 1.0f, ysMath::Constants::One, m_app->getBackgroundColor(), false);
}

void EngineView::onMouseDown(const Point &mouseLocal) {
    UiElement::onMouseDown(mouseLocal);
    m_dragStart = m_pan;
}

void EngineView::onDrag(const Point &p0, const Point &mouse0, const Point &mouse) {
    const Point delta = mouse - mouse0;
    const Point deltaUnits = {
        m_app->pixelsToUnits(delta.x),
        m_app->pixelsToUnits(delta.y)
    };

    m_pan = m_dragStart + deltaUnits;
}

void EngineView::onMouseScroll(int scroll) {
    const float f = std::powf(2.0, (float)scroll / 500.0f);

    const Point prevCenter = getCenter();

    m_zoom *= f;
    const Point newCenter = getCenter();

    Point diff = newCenter - prevCenter;
    m_pan += diff * m_zoom;
    m_dragStart += diff * m_zoom;
}

void EngineView::setBounds(const Bounds &bounds) {
    m_bounds = bounds;
    return;

    if (m_bounds.width() == 0 || m_bounds.height() == 0) {
        m_bounds = bounds;
        return;
    }

    if (m_bounds.width() == bounds.width() && m_bounds.height() == bounds.height()) return;

    const Point prevCenter = getCameraPosition();
    m_bounds = bounds;
    const Point newCenter = getCameraPosition();

    Point diff = newCenter - prevCenter;
    m_pan += unitsToPixels(diff) * m_zoom;
}

Point EngineView::getCenter() const {
    return getCameraPosition();
}

Point EngineView::getCameraPosition() const {
    return Point(-m_pan / m_zoom);
}
