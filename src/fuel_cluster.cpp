#include "../include/fuel_cluster.h"

#include <sstream>
#include <iomanip>

FuelCluster::FuelCluster() {
    m_engine = nullptr;
}

FuelCluster::~FuelCluster() {
    /* void */
}

void FuelCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app); 
}

void FuelCluster::destroy() {
    UiElement::destroy();
}

void FuelCluster::update(float dt) {
    UiElement::update(dt);
}

void FuelCluster::render() {
    const Bounds bounds = m_bounds.inset(10.0f);
    const Bounds title = bounds.verticalSplit(1.0f, 0.9f);
    const Bounds bodyBounds = bounds.verticalSplit(0.0f, 0.9f);

    drawCenteredText("FUEL", title.inset(10.0f), 24.0f);

    Grid grid;
    grid.h_cells = 1;
    grid.v_cells = 3;

    std::stringstream ss;
    ss << std::setprecision(3) << std::fixed;
    ss << units::convert(m_engine->getTotalVolumeFuelConsumed(), units::L);
    ss << " L";

    const Bounds totalFuelLiters = grid.get(bodyBounds, 0, 0);
    drawCenteredText(ss.str(), totalFuelLiters.inset(10.0f), 16.0f);

    ss = std::stringstream();
    ss << std::setprecision(3) << std::fixed;
    ss << units::convert(m_engine->getTotalVolumeFuelConsumed(), units::gal);
    ss << " gal";

    const Bounds totalFuelGallons = grid.get(bodyBounds, 0, 1);
    drawCenteredText(ss.str(), totalFuelGallons.inset(10.0f), 16.0f);

    ss = std::stringstream();
    ss << std::setprecision(2) << std::fixed;
    ss << "$" << 4.761 * units::convert(m_engine->getTotalVolumeFuelConsumed(), units::gal);
    ss << " USD";

    const Bounds costUSD = grid.get(bodyBounds, 0, 2);
    drawCenteredText(ss.str(), costUSD.inset(10.0f), 16.0f);

    UiElement::render();
}
