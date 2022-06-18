#include "../include/right_gauge_cluster.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"

#include <sstream>

RightGaugeCluster::RightGaugeCluster() {
    m_engine = nullptr;
    m_simulator = nullptr;

    m_afrCluster = nullptr;
    m_tachometer = nullptr;
    m_speedometer = nullptr;
    m_manifoldVacuumGauge = nullptr;
    m_volumetricEffGauge = nullptr;
    m_intakeCfmGauge = nullptr;
    m_combusionChamberStatus = nullptr;
    m_throttleDisplay = nullptr;
    m_fuelCluster = nullptr;
}

RightGaugeCluster::~RightGaugeCluster() {
    /* void */
}

void RightGaugeCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_tachometer = addElement<LabeledGauge>();
    m_speedometer = addElement<LabeledGauge>();
    m_manifoldVacuumGauge = addElement<LabeledGauge>();
    m_intakeCfmGauge = addElement<LabeledGauge>();
    m_volumetricEffGauge = addElement<LabeledGauge>();
    m_combusionChamberStatus = addElement<FiringOrderDisplay>();
    m_throttleDisplay = addElement<ThrottleDisplay>();
    m_afrCluster = addElement<AfrCluster>();
    m_fuelCluster = addElement<FuelCluster>();

    m_combusionChamberStatus->m_engine = m_engine;

    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);

    m_tachometer->m_title = "ENGINE SPEED";
    m_tachometer->m_unit = "rpm";
    m_tachometer->m_precision = 0;
    m_tachometer->setLocalPosition({ 0, 0 });
    m_tachometer->m_gauge->m_min = 0;
    m_tachometer->m_gauge->m_max = 7000;
    m_tachometer->m_gauge->m_minorStep = 100;
    m_tachometer->m_gauge->m_majorStep = 1000;
    m_tachometer->m_gauge->m_maxMinorTick = INT_MAX;
    m_tachometer->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_tachometer->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_tachometer->m_gauge->m_needleWidth = 4.0;
    m_tachometer->m_gauge->m_gamma = 1.0f;
    m_tachometer->m_gauge->m_needleKs = 1000.0f;
    m_tachometer->m_gauge->m_needleKd = 20.0f;
    m_tachometer->m_gauge->setBandCount(3);
    m_tachometer->m_gauge->setBand(
        { m_app->getWhite(), 400, 1000, 3.0f, 6.0f }, 0);
    m_tachometer->m_gauge->setBand(
        { m_app->getOrange(), 5000, 5500, 3.0f, 6.0f, -shortenAngle, shortenAngle }, 1);
    m_tachometer->m_gauge->setBand(
        { m_app->getRed(), 5500, 7000, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 2);

    m_speedometer->m_title = "VEHICLE SPEED";
    m_speedometer->m_unit = "MPH";
    m_speedometer->m_precision = 0;
    m_speedometer->setLocalPosition({ 0, 0 });
    m_speedometer->m_gauge->m_min = 0;
    m_speedometer->m_gauge->m_max = 200;
    m_speedometer->m_gauge->m_minorStep = 5;
    m_speedometer->m_gauge->m_majorStep = 10;
    m_speedometer->m_gauge->m_maxMinorTick = 200;
    m_speedometer->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_speedometer->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_speedometer->m_gauge->m_needleWidth = 4.0;
    m_speedometer->m_gauge->m_gamma = 1.0f;
    m_speedometer->m_gauge->m_needleKs = 1000.0f;
    m_speedometer->m_gauge->m_needleKd = 20.0f;
    m_speedometer->m_gauge->setBandCount(0);

    m_manifoldVacuumGauge->m_title = "MANIFOLD PRESSURE";
    m_manifoldVacuumGauge->m_unit = "inHg";
    m_manifoldVacuumGauge->m_precision = 0;
    m_manifoldVacuumGauge->setLocalPosition({ 0, 0 });
    m_manifoldVacuumGauge->m_gauge->m_min = -30;
    m_manifoldVacuumGauge->m_gauge->m_max = 5;
    m_manifoldVacuumGauge->m_gauge->m_minorStep = 1;
    m_manifoldVacuumGauge->m_gauge->m_majorStep = 5;
    m_manifoldVacuumGauge->m_gauge->m_maxMinorTick = 200;
    m_manifoldVacuumGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_manifoldVacuumGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_manifoldVacuumGauge->m_gauge->m_needleWidth = 4.0;
    m_manifoldVacuumGauge->m_gauge->m_gamma = 1.0f;
    m_manifoldVacuumGauge->m_gauge->m_needleKs = 1000.0f;
    m_manifoldVacuumGauge->m_gauge->m_needleKd = 50.0f;
    m_manifoldVacuumGauge->m_gauge->setBandCount(5);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getRed(), -5, -1, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getWhite(), -1.0f, 1.0f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 1);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getOrange(), -10, -5, 3.0f, 6.0f, shortenAngle, shortenAngle }, 2);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getBlue(), -22, -10, 3.0f, 6.0f, shortenAngle, shortenAngle }, 3);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getWhite(), -30, -22, 3.0f, 6.0f, shortenAngle, shortenAngle }, 4);

    m_volumetricEffGauge->m_title = "VOLUMETRIC EFF.";
    m_volumetricEffGauge->m_unit = "%";
    m_volumetricEffGauge->m_spaceBeforeUnit = false;
    m_volumetricEffGauge->m_precision = 1;
    m_volumetricEffGauge->setLocalPosition({ 0, 0 });
    m_volumetricEffGauge->m_gauge->m_min = 0;
    m_volumetricEffGauge->m_gauge->m_max = 120;
    m_volumetricEffGauge->m_gauge->m_minorStep = 5;
    m_volumetricEffGauge->m_gauge->m_majorStep = 10;
    m_volumetricEffGauge->m_gauge->m_maxMinorTick = 200;
    m_volumetricEffGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_volumetricEffGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_volumetricEffGauge->m_gauge->m_needleWidth = 4.0;
    m_volumetricEffGauge->m_gauge->m_gamma = 1.0f;
    m_volumetricEffGauge->m_gauge->m_needleKs = 1000.0f;
    m_volumetricEffGauge->m_gauge->m_needleKd = 50.0f;
    m_volumetricEffGauge->m_gauge->setBandCount(3);
    m_volumetricEffGauge->m_gauge->setBand(
        { m_app->getBlue(), 30, 80, 3.0f, 6.0f, 0.0f, shortenAngle }, 0);
    m_volumetricEffGauge->m_gauge->setBand(
        { m_app->getGreen(), 80, 100, 3.0f, 6.0f, shortenAngle, shortenAngle }, 1);
    m_volumetricEffGauge->m_gauge->setBand(
        { m_app->getRed(), 100, 120, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 2);

    m_intakeCfmGauge->m_title = "AIR SCFM";
    m_intakeCfmGauge->m_unit = "";
    m_intakeCfmGauge->m_precision = 1;
    m_intakeCfmGauge->setLocalPosition({ 0, 0 });
    m_intakeCfmGauge->m_gauge->m_min = 0;
    m_intakeCfmGauge->m_gauge->m_max = 1200;
    m_intakeCfmGauge->m_gauge->m_minorStep = 20;
    m_intakeCfmGauge->m_gauge->m_majorStep = 100;
    m_intakeCfmGauge->m_gauge->m_maxMinorTick = 1200;
    m_intakeCfmGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_intakeCfmGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_intakeCfmGauge->m_gauge->m_needleWidth = 4.0;
    m_intakeCfmGauge->m_gauge->m_gamma = 1.0f;
    m_intakeCfmGauge->m_gauge->m_needleKs = 1000.0f;
    m_intakeCfmGauge->m_gauge->m_needleKd = 50.0f;
    m_intakeCfmGauge->m_gauge->setBandCount(0);
}

void RightGaugeCluster::destroy() {
    UiElement::destroy();
}

void RightGaugeCluster::update(float dt) {
    m_combusionChamberStatus->m_engine = m_engine;
    m_throttleDisplay->m_engine = m_engine;
    m_afrCluster->m_engine = m_engine;
    m_fuelCluster->m_engine = m_engine;
    m_fuelCluster->m_simulator = m_simulator;

    UiElement::update(dt);
}

void RightGaugeCluster::render() {
    drawFrame(m_bounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    const Bounds tachSpeedCluster = m_bounds.verticalSplit(0.5f, 1.0f);
    renderTachSpeedCluster(tachSpeedCluster);

    const Bounds fuelAirCluster = m_bounds.verticalSplit(0.0f, 0.5f);
    renderFuelAirCluster(fuelAirCluster);

    UiElement::render();
}

void RightGaugeCluster::renderTachSpeedCluster(const Bounds &bounds) {
    const Bounds left = bounds.horizontalSplit(0.0f, 0.5f);
    const Bounds right = bounds.horizontalSplit(0.5f, 1.0f);

    const Bounds tach = left.verticalSplit(0.5f, 1.0f);
    m_tachometer->m_bounds = tach;
    m_tachometer->m_gauge->m_value = m_engine->getRpm();

    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);
    const float maxRpm = (float)std::ceil(units::toRpm(m_engine->getRedline() * 1.25) / 1000.0) * 1000.0f;
    const float redline = (float)std::ceil(units::toRpm(m_engine->getRedline()) / 500.0) * 500.0f;
    const float redlineWarning = (float)std::floor(units::toRpm(m_engine->getRedline() * 0.9) / 500.0) * 500.0f;
    m_tachometer->m_gauge->m_max = maxRpm;
    m_tachometer->m_gauge->setBandCount(3);
    m_tachometer->m_gauge->setBand(
        { m_app->getWhite(), 400, 1000, 3.0f, 6.0f }, 0);
    m_tachometer->m_gauge->setBand(
        { m_app->getOrange(), redlineWarning, redline, 3.0f, 6.0f, -shortenAngle, shortenAngle }, 1);
    m_tachometer->m_gauge->setBand(
        { m_app->getRed(), redline, maxRpm, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 2);

    const Bounds speed = left.verticalSplit(0.0f, 0.5f);
    m_speedometer->m_bounds = speed;
    m_speedometer->m_gauge->m_value =
        units::convert(m_simulator->getVehicle()->getSpeed(), units::mile / units::hour);

    m_combusionChamberStatus->m_bounds = right;
}

void RightGaugeCluster::renderFuelAirCluster(const Bounds &bounds) {
    const Bounds left = bounds.horizontalSplit(0.0f, 0.5f);
    const Bounds right = bounds.horizontalSplit(0.5f, 1.0f);

    const Bounds throttle = left.verticalSplit(0.5f, 1.0f);
    m_throttleDisplay->m_bounds = throttle;

    const Bounds fuelSection = left.verticalSplit(0.0f, 0.5f);
    const Bounds afr = fuelSection.horizontalSplit(0.0f, 0.5f);
    m_afrCluster->m_bounds = afr;

    const Bounds fuelConsumption = fuelSection.horizontalSplit(0.5f, 1.0f);
    m_fuelCluster->m_bounds = fuelConsumption;

    const double ambientPressure = units::pressure(1.0, units::atm);
    const double ambientTemperature = units::celcius(25.0);

    Grid grid = { 1, 3 };
    const Bounds manifoldVacuum = grid.get(right, 0, 0, 1, 1);
    m_manifoldVacuumGauge->m_bounds = manifoldVacuum;
    m_manifoldVacuumGauge->m_gauge->m_value =
        units::convert(m_engine->getManifoldPressure() - ambientPressure, units::inHg);

    const double rpm = m_engine->getRpm();
    const double theoreticalAirPerRevolution =
        (ambientPressure * m_engine->getDisplacement())
        / (constants::R * ambientTemperature);
    const double theoreticalAirPerSecond = 0.5 * theoreticalAirPerRevolution * rpm / 60.0;
    const double actualAirPerSecond = m_engine->getIntakeFlowRate();
    const double volumetricEfficiency = (std::abs(rpm) < 1E-3)
        ? 0
        : (actualAirPerSecond / theoreticalAirPerSecond);

    const Bounds cfmBounds = grid.get(right, 0, 1, 1, 1);
    m_intakeCfmGauge->m_bounds = cfmBounds;
    m_intakeCfmGauge->m_gauge->m_value = units::convert(actualAirPerSecond, units::scfm);

    const Bounds volumetricEfficiencyBounds = grid.get(right, 0, 2, 1, 1);
    m_volumetricEffGauge->m_bounds = volumetricEfficiencyBounds;
    m_volumetricEffGauge->m_gauge->m_value = 100 * volumetricEfficiency;
}
