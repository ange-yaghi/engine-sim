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
    m_pan = m_dragStart + (mouse - mouse0);
}

void EngineView::onMouseScroll(int scroll) {
    const float f = std::powf(2.0, (float)scroll / 500.0f);

    const Point prevCenter = getCenter();

    m_zoom *= f;
    const Point newCenter = getCenter();

    Point diff = newCenter - prevCenter;
    m_pan += unitsToPixels(diff) * m_zoom;
    m_dragStart += unitsToPixels(diff) * m_zoom;

    const Point test = getRenderPoint(m_bounds.getPosition(Bounds::center));
    const Point finalCenter = getCenter();
}

Point EngineView::getCenter() const {
    return getCameraPosition() +
        getRenderPoint(m_bounds.getPosition(Bounds::center)) / m_zoom;
}

Point EngineView::getCameraPosition() const {
    return pixelsToUnits(-m_pan / m_zoom);
}
