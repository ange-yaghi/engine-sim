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
    ysVector color = m_app->getBackgroundColor();
    if (isMouseHeld()) {
        color = mix(m_app->getBackgroundColor(), m_app->getWhite(), 0.02f);
    }
    else if (isMouseOver()) {
        color = mix(m_app->getBackgroundColor(), m_app->getWhite(), 0.01f);
    }

    drawFrame(m_bounds, 1.0, ysMath::Constants::One, color);
    drawCenteredText(m_text, m_bounds, m_fontSize);
}
