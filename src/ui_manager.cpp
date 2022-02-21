#include "../include/ui_manager.h"

#include "../include/engine_sim_application.h"

UiManager::UiManager() {
    m_app = nullptr;
    m_dragStart = nullptr;
    m_hover = nullptr;
}

UiManager::~UiManager() {
    /* void */
}

void UiManager::initialize(EngineSimApplication *app) {
    m_app = app;
    m_root.initialize(app);
}

void UiManager::destroy() {
    m_root.destroy();
}

void UiManager::update(float dt) {
    m_root.update(dt);

    int mouse_x, mouse_y;
    m_app->getEngine()->GetOsMousePos(&mouse_x, &mouse_y);

    Point mousePos = { (float)mouse_x, (float)mouse_y };
    UiElement *newHover = m_root.mouseOver(mousePos);
    if (newHover != m_hover) {
        if (m_hover != nullptr) m_hover->onMouseLeave();
        if (newHover != nullptr) newHover->onMouseOver(mousePos);
        m_hover = newHover;
    }

    if (m_app->getEngine()->ProcessMouseButtonDown(ysMouse::Button::Left)) {
        m_dragStart = m_hover;
        m_mouse_p0 = mousePos;
        if (m_dragStart != nullptr) {
            m_drag_p0 = m_dragStart->getLocalPosition();
            m_dragStart->onMouseDown(m_dragStart->worldToLocal(mousePos));
        }
    }
    else if (m_app->getEngine()->ProcessMouseButtonUp(ysMouse::Button::Left)) {
        UiElement *dragRelease = m_hover;

        if (m_dragStart != nullptr) m_dragStart->onMouseUp(mousePos);

        if (dragRelease != nullptr && m_dragStart == dragRelease) {
            m_dragStart->onMouseClick(m_dragStart->worldToLocal(mousePos));
        }

        m_dragStart = nullptr;
    }

    if (m_dragStart != nullptr) {
        m_dragStart->onDrag(m_drag_p0, mousePos - m_mouse_p0);
    }
}

void UiManager::render() {
    m_root.render();
}
