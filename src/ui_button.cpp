#include "../include/ui_button.h"

#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

UiButton::UiButton() {
    m_text = "";
    m_fontSize = 12;
    m_checkMouse = true;
}

UiButton::~UiButton() {
    /* void */
}

void UiButton::update(float dt) {
    m_mouseBounds = m_bounds;
}

void UiButton::render() {
    ysVector color = mix(m_app->getBackgroundColor(), m_app->getForegroundColor(), 0.02f);
    if (isMouseHeld()) {
        color = mix(m_app->getBackgroundColor(), m_app->getForegroundColor(), 0.1f);
    }
    else if (isMouseOver()) {
        color = mix(m_app->getBackgroundColor(), m_app->getForegroundColor(), 0.05f);
    }

    const ysVector border = mix(m_app->getBackgroundColor(), m_app->getForegroundColor(), 0.16f);
    drawFrame(m_bounds, 1.0f, border, color);
    drawCenteredText(m_text, m_bounds, m_fontSize, Bounds::center);
}
