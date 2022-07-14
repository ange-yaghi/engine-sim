#include "../include/ui_element.h"

#include "../include/engine_sim_application.h"

#include <assert.h>

UiElement::UiElement() {
    m_app = nullptr;
    m_parent = nullptr;
    m_signalTarget = nullptr;
    m_checkMouse = false;
    m_disabled = false;
    m_index = -1;

    m_draggable = false;
    m_mouseOver = false;
    m_mouseHeld = false;
}

UiElement::~UiElement() {
    /* void */
}

void UiElement::initialize(EngineSimApplication *app) {
    m_app = app;
}

void UiElement::destroy() {
    /* void */
}

void UiElement::update(float dt) {
    for (UiElement *child : m_children) {
        child->update(dt);
    }
}

void UiElement::render() {
    for (UiElement *child : m_children) {
        child->render();
    }
}

void UiElement::signal(UiElement *element, Event event) {
    /* void */
}

void UiElement::onMouseDown(const Point &mouseLocal) {
    m_mouseHeld = true;
}

void UiElement::onMouseUp(const Point &mouseLocal) {
    m_mouseHeld = false;
}

void UiElement::onMouseClick(const Point &mouseLocal) {
    signal(Event::Clicked);
}

void UiElement::onDrag(const Point &p0, const Point &mouse0, const Point &mouse) {
    if (m_draggable) {
        m_localPosition = p0 + (mouse - mouse0);
    }
}

void UiElement::onMouseOver(const Point &mouseLocal) {
    m_mouseOver = true;
}

void UiElement::onMouseLeave() {
    m_mouseOver = false;
}

void UiElement::onMouseScroll(int mouseScroll) {
    /* void */
}

UiElement *UiElement::mouseOver(const Point &mouseLocal) {
    if (m_disabled) return nullptr;

    const int n = (int)getChildCount();
    for (int i = n - 1; i >= 0; --i) {
        UiElement *child = m_children[i];
        UiElement *clickedElement = child->mouseOver(mouseLocal - child->m_localPosition);
        if (clickedElement != nullptr) {
            return clickedElement;
        }
    }

    return (m_checkMouse && m_mouseBounds.overlaps(mouseLocal))
        ? this
        : nullptr;
}

Point UiElement::getWorldPosition() const {
    return (m_parent != nullptr)
        ? m_parent->getWorldPosition() + m_localPosition
        : m_localPosition;
}

void UiElement::setLocalPosition(const Point &p, const Point &ref) {
    const Point current = m_bounds.getPosition(ref) + m_localPosition;
    m_localPosition += (p - current);
}

void UiElement::bringToFront(UiElement *element) {
    assert(element->m_parent == this);

    m_children.erase(m_children.begin() + element->m_index);
    m_children.push_back(element);
}

void UiElement::activate() {
    if (m_parent != nullptr) {
        m_parent->bringToFront(this);
        m_parent->activate();
    }
}

void UiElement::signal(Event event) {
    if (m_signalTarget == nullptr) return;

    m_signalTarget->signal(this, event);
}

float UiElement::pixelsToUnits(float length) const {
    return m_app->pixelsToUnits(length);
}

Point UiElement::pixelsToUnits(const Point &p) const {
    return { pixelsToUnits(p.x), pixelsToUnits(p.y) };
}

float UiElement::unitsToPixels(float x) const {
    return m_app->unitsToPixels(x);
}

Point UiElement::unitsToPixels(const Point &p) const {
    return { unitsToPixels(p.x), unitsToPixels(p.y) };
}

Point UiElement::getRenderPoint(const Point &p) const {
    const Point offset(
            -(float)m_app->getScreenWidth() / 2,
            -(float)m_app->getScreenHeight() / 2);
    const Point posPixels = localToWorld(p) + offset;

    return pixelsToUnits(posPixels);
}

Bounds UiElement::getRenderBounds(const Bounds &b) const {
    return { getRenderPoint(b.m0), getRenderPoint(b.m1) };
}

Bounds UiElement::unitsToPixels(const Bounds &b) const {
    return { unitsToPixels(b.m0), unitsToPixels(b.m1) };
}

void UiElement::resetShader() {
    m_app->getShaders()->ResetBaseColor();
    m_app->getShaders()->SetObjectTransform(ysMath::LoadIdentity());
}

void UiElement::drawFrame(
        const Bounds &bounds,
        float thickness,
        const ysVector &frameColor,
        const ysVector &fillColor,
        bool fill)
{
    GeometryGenerator *generator = m_app->getGeometryGenerator();

    const Bounds worldBounds = getRenderBounds(bounds);
    const Point position = worldBounds.getPosition(Bounds::center);

    GeometryGenerator::FrameParameters params;
    params.frameWidth = worldBounds.width();
    params.frameHeight = worldBounds.height();
    params.lineWidth = pixelsToUnits(thickness);
    params.x = position.x;
    params.y = position.y;

    GeometryGenerator::Line2dParameters lineParams;
    lineParams.lineWidth = worldBounds.height();
    lineParams.y0 = lineParams.y1 = worldBounds.getPosition(Bounds::center).y;
    lineParams.x0 = worldBounds.left();
    lineParams.x1 = worldBounds.right();

    GeometryGenerator::GeometryIndices frame, body;

    resetShader();
    if (fill) {
        generator->startShape();
        generator->generateLine2d(lineParams);
        generator->endShape(&body);

        m_app->getShaders()->SetBaseColor(fillColor);
        m_app->drawGenerated(body, 0x11, m_app->getShaders()->GetUiFlags());
    }

    generator->startShape();
    generator->generateFrame(params);
    generator->endShape(&frame);

    m_app->getShaders()->SetBaseColor(frameColor);
    m_app->drawGenerated(frame, 0x11, m_app->getShaders()->GetUiFlags());
}

void UiElement::drawText(
        const std::string &s,
        const Bounds &bounds,
        float height,
        const Point &ref)
{
    const Bounds renderBounds = unitsToPixels(getRenderBounds(bounds));
    const Point origin = renderBounds.getPosition(ref);

    m_app->getTextRenderer()->RenderText(
            s, origin.x, origin.y - height / 4, height);
}

void UiElement::drawCenteredText(
        const std::string &s,
        const Bounds &bounds,
        float height,
        const Point &ref)
{
    const Bounds renderBounds = unitsToPixels(getRenderBounds(bounds));
    const Point origin = renderBounds.getPosition(ref);

    const float width = m_app->getTextRenderer()->CalculateWidth(s, height);
    m_app->getTextRenderer()->RenderText(
            s, origin.x - width / 2, origin.y - height / 4, height);
}
