#include "../include/fuel_cluster.h"

#include "../include/engine_sim_application.h"

#include <sstream>
#include <iomanip>

FuelCluster::FuelCluster() {
    m_engine = nullptr;
    m_simulator = nullptr;
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
    grid.v_cells = 10;

    std::stringstream ss;
    ss << std::setprecision(3) << std::fixed;
    ss << units::convert(getTotalVolumeFuelConsumed(), units::L);
    ss << " L";

    const Bounds totalFuelLiters = grid.get(bodyBounds, 0, 1, 1, 2);
    drawText(ss.str(), totalFuelLiters, 32.0f, Bounds::lm);

    ss = std::stringstream();
    ss << std::setprecision(3) << std::fixed;
    ss << units::convert(getTotalVolumeFuelConsumed(), units::gal);
    ss << " gal";

    const Bounds totalFuelGallons = grid.get(bodyBounds, 0, 3, 1, 1);
    drawText(ss.str(), totalFuelGallons, 16.0f, Bounds::lm);

    const double fuelConsumed = getTotalVolumeFuelConsumed();
    const double fuelConsumed_gallons = units::convert(fuelConsumed, units::gal);

    ss = std::stringstream();
    ss << std::setprecision(2) << std::fixed;
    ss << "$" << 4.761 * fuelConsumed_gallons << " USD";

    const Bounds costUSD = grid.get(bodyBounds, 0, 4);
    drawText(ss.str(), costUSD, 16.0f, Bounds::lm);

    const double travelledDistance = (m_simulator->getVehicle() != nullptr)
        ? m_simulator->getVehicle()->getTravelledDistance()
        : 0.0;
    const double mpg = units::convert(travelledDistance, units::mile) / fuelConsumed_gallons;

    ss = std::stringstream();
    ss << std::setprecision(2) << std::fixed;
    ss << mpg << " MPG";

    const Bounds mpgBounds = grid.get(bodyBounds, 0, 6);
    drawText(ss.str(), mpgBounds, 16.0f, Bounds::lm);

    const double lp100km = (travelledDistance != 0)
        ? units::convert(fuelConsumed, units::L)
            / (units::convert(travelledDistance, units::km) / 100.0)
        : 0;

    ss = std::stringstream();
    ss << std::setprecision(2) << std::fixed;
    ss << ((lp100km > 100.0) ? 100.0 : lp100km) << " L/100 KM";

    const Bounds lp100kmBounds = grid.get(bodyBounds, 0, 7);
    drawText(ss.str(), lp100kmBounds, 12.0f, Bounds::lm);

    UiElement::render();
}

double FuelCluster::getTotalVolumeFuelConsumed() const {
    return (m_engine != nullptr)
        ? m_engine->getTotalVolumeFuelConsumed()
        : 0.0;
}
