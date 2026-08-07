#include "../include/info_cluster.h"

#include "../include/engine_sim_application.h"

#include <sstream>
#include <iomanip>

InfoCluster::InfoCluster() {
    m_engine = nullptr;
    m_logMessage = "Started";
    m_loadEngineButton = nullptr;
    m_reloadButton = nullptr;
    m_loadThemeButton = nullptr;
    m_dynoButton = nullptr;
    m_settingsButton = nullptr;
}

InfoCluster::~InfoCluster() {
    /* void */
}

void InfoCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_loadEngineButton = addElement<UiButton>(this);
    m_loadEngineButton->m_text = "LOAD ENGINE";

    m_reloadButton = addElement<UiButton>(this);
    m_reloadButton->m_text = "RELOAD";

    m_loadThemeButton = addElement<UiButton>(this);
    m_loadThemeButton->m_text = "LOAD THEME";

    m_dynoButton = addElement<UiButton>(this);
    m_dynoButton->m_text = "DYNO";

    m_settingsButton = addElement<UiButton>(this);
    m_settingsButton->m_text = "SETTINGS";
}

void InfoCluster::signal(UiElement *element, Event event) {
    if (event != Event::Clicked) return;

    if (element == m_loadEngineButton) {
        m_app->requestLoadSimulationFromDialog();
    }
    else if (element == m_reloadButton) {
        m_app->requestReload();
    }
    else if (element == m_loadThemeButton) {
        m_app->loadThemeFromDialog();
    }
    else if (element == m_dynoButton) {
        m_app->toggleDynoScreen();
    }
    else if (element == m_settingsButton) {
        m_app->toggleSettingsScreen();
    }
}

void InfoCluster::destroy() {
    UiElement::destroy();
}

void InfoCluster::update(float dt) {
    UiElement::update(dt);
}

void InfoCluster::render() {
    Grid grid;
    grid.h_cells = 6;
    grid.v_cells = 4;

    const Bounds logoBounds = grid.get(m_bounds, 0, 0, 1, 2);
    drawPanel(logoBounds);

    drawModel(
        m_app->getAssetManager()->GetModelAsset("Logo"),
        m_app->getForegroundColor(),
        logoBounds.getPosition(Bounds::center),
        Point(logoBounds.height(), logoBounds.height()) * 0.75f);

    const Bounds titleBounds = grid.get(m_bounds, 1, 0, 5, 2);
    drawPanel(titleBounds);

    Grid titleSplit;
    titleSplit.h_cells = 1;
    titleSplit.v_cells = 3;
    drawAlignedText(
        "ENGINE SIMULATOR",
        titleSplit.get(titleBounds, 0, 0).inset(10.0f).move({ 0.0f, -21.0f }),
        42.0f,
        Bounds::bl,
        Bounds::bl);
    drawAlignedText(
        "YOUTUBE/ANGETHEGREAT -- UI BY SOLI-DSTATE",
        titleSplit.get(titleBounds, 0, 1).inset(10.0f).move({ 0.0f, 5.0f }),
        24.0f,
        Bounds::tl,
        Bounds::tl);
    drawAlignedText(
        "BUILD: v" + EngineSimApplication::getBuildVersion() + " // " __DATE__,
        titleSplit.get(titleBounds, 0, 2).inset(10.0f).move({ 0.0f, 10.0f }),
        16.0f,
        Bounds::tl,
        Bounds::tl);

    // Lower half (originally engine-info + log-message rows) subdivided into
    // three bands so the button row doesn't eat into the logo/title area above.
    const Bounds lowerBounds = grid.get(m_bounds, 0, 2, 6, 2);
    Grid lowerGrid;
    lowerGrid.h_cells = 6;
    lowerGrid.v_cells = 3;

    const Bounds buttonRowBounds = lowerGrid.get(lowerBounds, 0, 0, 6, 1);
    drawPanel(buttonRowBounds);

    Grid buttonSplit;
    buttonSplit.h_cells = 10;
    buttonSplit.v_cells = 1;
    m_loadEngineButton->m_bounds = buttonSplit.get(buttonRowBounds, 0, 0, 2, 1).inset(2.0f);
    m_reloadButton->m_bounds = buttonSplit.get(buttonRowBounds, 2, 0, 2, 1).inset(2.0f);
    m_loadThemeButton->m_bounds = buttonSplit.get(buttonRowBounds, 4, 0, 2, 1).inset(2.0f);
    m_dynoButton->m_bounds = buttonSplit.get(buttonRowBounds, 6, 0, 2, 1).inset(2.0f);
    m_settingsButton->m_bounds = buttonSplit.get(buttonRowBounds, 8, 0, 2, 1).inset(2.0f);

    const Bounds engineInfoBounds = lowerGrid.get(lowerBounds, 0, 1, 6, 1);
    drawPanel(engineInfoBounds);

    drawAlignedText(
        (m_engine != nullptr) ? m_engine->getName() : "<NO ENGINE>",
        engineInfoBounds.inset(10.0f),
        24.0f,
        Bounds::lm,
        Bounds::lm);

    std::stringstream ss;
    if (m_engine != nullptr) {
        ss << std::fixed;

        if (m_engine->getDisplacement() < units::volume(1.0, units::L)) {
            ss << std::setprecision(0) << units::convert(m_engine->getDisplacement(), units::cc) << " cc -- ";
        }
        else {
            ss << std::setprecision(1) << units::convert(m_engine->getDisplacement(), units::L) << " L -- ";
        }

        ss << std::setprecision(0) << units::convert(m_engine->getDisplacement(), units::cubic_inches) << " CI";
    }
    else {
        ss << "N/A";
    }

    drawAlignedText(
        ss.str(),
        engineInfoBounds.inset(10.0f),
        24.0f,
        Bounds::rm,
        Bounds::rm);

    const Bounds infoMessagesBounds = lowerGrid.get(lowerBounds, 0, 2, 6, 1);
    drawPanel(infoMessagesBounds);

    drawAlignedText(
        m_logMessage,
        infoMessagesBounds.inset(10.0f),
        24.0f,
        Bounds::lm,
        Bounds::lm);

    UiElement::render();
}
