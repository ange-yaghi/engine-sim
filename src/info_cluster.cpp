#include "../include/info_cluster.h"

#include "../include/engine_sim_application.h"

#include <sstream>
#include <iomanip>

InfoCluster::InfoCluster() {
    m_engine = nullptr;
    m_logMessage = "Started";
}

InfoCluster::~InfoCluster() {
    /* void */
}

void InfoCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);
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
    drawFrame(logoBounds, 1.0f, m_app->getWhite(), m_app->getBackgroundColor());

    drawModel(
        m_app->getAssetManager()->GetModelAsset("Logo"),
        m_app->getWhite(),
        logoBounds.getPosition(Bounds::center),
        Point(logoBounds.height(), logoBounds.height()) * 0.75f);

    const Bounds titleBounds = grid.get(m_bounds, 1, 0, 5, 2);
    drawFrame(titleBounds, 1.0f, m_app->getWhite(), m_app->getBackgroundColor());

    Grid titleSplit;
    titleSplit.h_cells = 1;
    titleSplit.v_cells = 2;
    drawAlignedText(
        "ENGINE SIMULATOR",
        titleSplit.get(titleBounds, 0, 0).inset(10.0f).move({ 0.0f, -10.0f }),
        42.0f,
        Bounds::bl,
        Bounds::bl);
    drawAlignedText(
        "YOUTUBE/ANGETHEGREAT",
        titleSplit.get(titleBounds, 0, 1).inset(10.0f).move({ 0.0f, 10.0f }),
        24.0f,
        Bounds::tl,
        Bounds::tl);

    const Bounds engineInfoBounds = grid.get(m_bounds, 0, 2, 6, 1);
    drawFrame(engineInfoBounds, 1.0f, m_app->getWhite(), m_app->getBackgroundColor());

    drawAlignedText(
        m_engine->getName(),
        engineInfoBounds.inset(10.0f),
        24.0f,
        Bounds::lm,
        Bounds::lm);

    std::stringstream ss;
    ss << std::fixed
        << std::setprecision(1) << units::convert(m_engine->getDisplacement(), units::L) << " L -- "
        << std::setprecision(0) << units::convert(m_engine->getDisplacement(), units::cubic_inches) << " CI";

    drawAlignedText(
        ss.str(),
        engineInfoBounds.inset(10.0f),
        24.0f,
        Bounds::rm,
        Bounds::rm);

    const Bounds infoMessagesBounds = grid.get(m_bounds, 0, 3, 6, 1);
    drawFrame(infoMessagesBounds, 1.0f, m_app->getWhite(), m_app->getBackgroundColor());

    drawAlignedText(
        m_logMessage,
        infoMessagesBounds.inset(10.0f),
        24.0f,
        Bounds::lm,
        Bounds::lm);
}
