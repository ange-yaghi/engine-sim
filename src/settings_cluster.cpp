#include "../include/settings_cluster.h"

#include "../include/engine_sim_application.h"

#include <cmath>

SettingsCluster::SettingsCluster() {
    m_saveButton = nullptr;
    m_resetButton = nullptr;
    m_closeButton = nullptr;
    m_capturingAction = InputAction::Count;
}

SettingsCluster::~SettingsCluster() {
    /* void */
}

void SettingsCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    std::string lastGroup;
    for (int i = 0; i < g_inputActionCount; ++i) {
        const InputActionInfo &info = g_inputActionTable[i];

        if (info.group != lastGroup) {
            LayoutItem header;
            header.isHeader = true;
            header.headerLabel = info.group;
            header.actionIndex = -1;
            m_layout.push_back(header);
            lastGroup = info.group;
        }

        LayoutItem row;
        row.isHeader = false;
        row.actionIndex = i;
        m_layout.push_back(row);

        KeybindRow keybindRow;
        keybindRow.actionIndex = i;
        keybindRow.rebindButton = addElement<UiButton>(this);
        keybindRow.rebindButton->m_text = "REBIND";
        keybindRow.rebindButton->m_fontSize = 12.0f;
        m_rows.push_back(keybindRow);
    }

    m_saveButton = addElement<UiButton>(this);
    m_saveButton->m_text = "SAVE";

    m_resetButton = addElement<UiButton>(this);
    m_resetButton->m_text = "RESET TO DEFAULTS";

    m_closeButton = addElement<UiButton>(this);
    m_closeButton->m_text = "CLOSE";

    setVisible(false);
}

void SettingsCluster::destroy() {
    UiElement::destroy();
}

void SettingsCluster::open() {
    m_snapshot = m_app->getKeyBindings()->snapshot();
    m_capturingAction = InputAction::Count;
    setVisible(true);
    activate();
}

void SettingsCluster::close() {
    m_app->getKeyBindings()->restore(m_snapshot);
    m_capturingAction = InputAction::Count;
    setVisible(false);
}

void SettingsCluster::signal(UiElement *element, Event event) {
    if (event != Event::Clicked) return;

    if (element == m_saveButton) {
        m_app->saveKeyBindings();
        m_snapshot = m_app->getKeyBindings()->snapshot();
    }
    else if (element == m_resetButton) {
        m_app->getKeyBindings()->resetAllToDefaults();
    }
    else if (element == m_closeButton) {
        close();
    }
    else {
        for (KeybindRow &row : m_rows) {
            if (element == row.rebindButton) {
                m_capturingAction = g_inputActionTable[row.actionIndex].action;
                break;
            }
        }
    }
}

void SettingsCluster::update(float dt) {
    UiElement::update(dt);

    if (!isVisible() || !isCapturing()) return;

    for (ysKey::Code code : KeyBindings::bindableKeys()) {
        if (m_app->getEngine()->ProcessKeyDown(code)) {
            m_app->getKeyBindings()->rebind(m_capturingAction, code);
            m_capturingAction = InputAction::Count;
            break;
        }
    }
}

void SettingsCluster::render() {
    if (!isVisible()) return;

    drawBox(m_bounds, m_app->getBackgroundColor());

    const Bounds panelBounds = m_bounds.inset(60.0f);
    drawPanel(panelBounds, 2.0f);

    const Bounds titleBounds = panelBounds.verticalSplit(0.93f, 1.0f);
    drawCenteredText("SETTINGS - KEYBINDS", titleBounds.inset(10.0f), 28.0f);

    const Bounds bottomBar = panelBounds.verticalSplit(0.0f, 0.07f);
    const Bounds contentArea = panelBounds.verticalSplit(0.07f, 0.93f).inset(20.0f);

    constexpr int columns = 3;
    const int rowsPerColumn = (int)std::ceil(m_layout.size() / (float)columns);

    Grid grid;
    grid.h_cells = columns;
    grid.v_cells = rowsPerColumn;

    for (size_t i = 0; i < m_layout.size(); ++i) {
        const int col = (int)(i / rowsPerColumn);
        const int row = (int)(i % rowsPerColumn);
        const Bounds cellBounds = grid.get(contentArea, col, row).inset(4.0f);

        const LayoutItem &item = m_layout[i];
        if (item.isHeader) {
            drawAlignedText(item.headerLabel, cellBounds, 15.0f, Bounds::bl, Bounds::bl);
        }
        else {
            const InputActionInfo &info = g_inputActionTable[item.actionIndex];

            const Bounds labelBounds = cellBounds.horizontalSplit(0.0f, 0.52f);
            const Bounds keyBounds = cellBounds.horizontalSplit(0.52f, 0.75f);
            const Bounds buttonBounds = cellBounds.horizontalSplit(0.75f, 1.0f);

            drawAlignedText(info.label, labelBounds.inset(2.0f), 13.0f, Bounds::lm, Bounds::lm);

            const bool capturingThis = (m_capturingAction == info.action);
            const std::string keyLabel = capturingThis
                ? "..."
                : KeyBindings::keyName(m_app->getKeyBindings()->get(info.action));
            drawCenteredText(keyLabel, keyBounds.inset(2.0f), 13.0f);

            m_rows[item.actionIndex].rebindButton->m_bounds = buttonBounds.inset(2.0f);
        }
    }

    Grid bottomGrid;
    bottomGrid.h_cells = 3;
    bottomGrid.v_cells = 1;
    m_saveButton->m_bounds = bottomGrid.get(bottomBar, 0, 0).inset(10.0f);
    m_resetButton->m_bounds = bottomGrid.get(bottomBar, 1, 0).inset(10.0f);
    m_closeButton->m_bounds = bottomGrid.get(bottomBar, 2, 0).inset(10.0f);

    UiElement::render();
}
