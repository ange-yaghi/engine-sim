#include "../include/right_gauge_cluster.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"

#include <climits>
#include <cmath>
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
    m_isAbsolute = false;
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

    m_speedUnits = app->getAppSettings()->speedUnits;
    m_pressureUnits = app->getAppSettings()->pressureUnits;
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
    m_tachometer->m_gauge->m_thetaMin = (float)constants::pi * 1.2f;
    m_tachometer->m_gauge->m_thetaMax = -(float)constants::pi * 0.2f;
    m_tachometer->m_gauge->m_needleWidth = 4.0f;
    m_tachometer->m_gauge->m_gamma = 1.0f;
    m_tachometer->m_gauge->m_needleKs = 1000.0f;
    m_tachometer->m_gauge->m_needleKd = 20.0f;
    m_tachometer->m_gauge->setBandCount(3);
    m_tachometer->m_gauge->setBand(
        { m_app->getForegroundColor(), 400, 1000, 3.0f, 6.0f }, 0);
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
    m_speedometer->m_gauge->m_thetaMin = (float)constants::pi * 1.2f;
    m_speedometer->m_gauge->m_thetaMax = -(float)constants::pi * 0.2f;
    m_speedometer->m_gauge->m_needleWidth = 4.0f;
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
    m_manifoldVacuumGauge->m_gauge->m_thetaMin = (float)constants::pi * 1.2f;
    m_manifoldVacuumGauge->m_gauge->m_thetaMax = -(float)constants::pi * 0.2f;
    m_manifoldVacuumGauge->m_gauge->m_needleWidth = 4.0f;
    m_manifoldVacuumGauge->m_gauge->m_gamma = 1.0f;
    m_manifoldVacuumGauge->m_gauge->m_needleKs = 1000.0f;
    m_manifoldVacuumGauge->m_gauge->m_needleKd = 50.0f;
    m_manifoldVacuumGauge->m_gauge->setBandCount(5);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getRed(), -5, -1, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getForegroundColor(), -1.0f, 1.0f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 1);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getOrange(), -10, -5, 3.0f, 6.0f, shortenAngle, shortenAngle }, 2);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getBlue(), -22, -10, 3.0f, 6.0f, shortenAngle, shortenAngle }, 3);
    m_manifoldVacuumGauge->m_gauge->setBand(
        { m_app->getForegroundColor(), -30, -22, 3.0f, 6.0f, shortenAngle, shortenAngle }, 4);

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
    m_volumetricEffGauge->m_gauge->m_thetaMin = (float)constants::pi * 1.2f;
    m_volumetricEffGauge->m_gauge->m_thetaMax = -(float)constants::pi * 0.2f;
    m_volumetricEffGauge->m_gauge->m_needleWidth = 4.0f;
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
    m_intakeCfmGauge->m_gauge->m_thetaMin = (float)constants::pi * 1.2f;
    m_intakeCfmGauge->m_gauge->m_thetaMax = -(float)constants::pi * 0.2f;
    m_intakeCfmGauge->m_gauge->m_needleWidth = 4.0f;
    m_intakeCfmGauge->m_gauge->m_gamma = 1.0f;
    m_intakeCfmGauge->m_gauge->m_needleKs = 1000.0f;
    m_intakeCfmGauge->m_gauge->m_needleKd = 50.0f;
    m_intakeCfmGauge->m_gauge->setBandCount(0);
    //Set display units
    setUnits();
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
    drawFrame(m_bounds, 1.0, m_app->getForegroundColor(), m_app->getBackgroundColor());

    const Bounds tachSpeedCluster = m_bounds.verticalSplit(0.5f, 1.0f);
    renderTachSpeedCluster(tachSpeedCluster);

    const Bounds fuelAirCluster = m_bounds.verticalSplit(0.0f, 0.5f);
    renderFuelAirCluster(fuelAirCluster);

    UiElement::render();
}

void RightGaugeCluster::setEngine(Engine *engine) {
    m_engine = engine;
}

void RightGaugeCluster::renderTachSpeedCluster(const Bounds &bounds) {
    const Bounds left = bounds.horizontalSplit(0.0f, 0.5f);
    const Bounds right = bounds.horizontalSplit(0.5f, 1.0f);

    const Bounds tach = left.verticalSplit(0.5f, 1.0f);
    m_tachometer->m_bounds = tach;
    m_tachometer->m_gauge->m_value = (float)std::abs(getRpm());

    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);
    const float maxRpm =
        (float)std::ceil(units::toRpm(getRedline() * 1.25) / 1000.0) * 1000.0f;
    const float redline =
        (float)std::ceil(units::toRpm(getRedline()) / 500.0) * 500.0f;
    const float redlineWarning =
        (float)std::floor(units::toRpm(getRedline() * 0.9) / 500.0) * 500.0f;
    m_tachometer->m_gauge->m_max = (int)maxRpm;
    m_tachometer->m_gauge->setBandCount(3);
    m_tachometer->m_gauge->setBand(
        { m_app->getForegroundColor(), 400, 1000, 3.0f, 6.0f }, 0);
    m_tachometer->m_gauge->setBand(
        { m_app->getOrange(), redlineWarning, redline, 3.0f, 6.0f, -shortenAngle, shortenAngle }, 1);
    m_tachometer->m_gauge->setBand(
        { m_app->getRed(), redline, maxRpm, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 2);

    const Bounds speed = left.verticalSplit(0.0f, 0.5f);
    m_speedometer->m_bounds = speed;

    m_speedometer->m_gauge->m_value = (m_speedUnits == "mph") 
        ? (float)units::convert(std::abs(getSpeed()), units::mile / units::hour) 
        : (float)units::convert(std::abs(getSpeed()), units::km / units::hour);


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

    constexpr double ambientPressure = units::pressure(1.0, units::atm);
    constexpr double ambientTemperature = units::celcius(25.0);

    Grid grid = { 1, 3 };
    const Bounds manifoldVacuum = grid.get(right, 0, 0, 1, 1);
    m_manifoldVacuumGauge->m_bounds = manifoldVacuum;

    const double vacuumReading = getManifoldPressureWithUnits(ambientPressure);
    if (m_isAbsolute) {
        m_manifoldVacuumGauge->m_gauge->m_value = static_cast<float>(vacuumReading);
    }
    else {
        m_manifoldVacuumGauge->m_gauge->m_value = (vacuumReading > -0.5)
            ? 0.0f
            : static_cast<float>(vacuumReading);
    }

    const double rpm = std::fmax(getRpm(), 0.0);
    const double theoreticalAirPerRevolution = (m_engine == nullptr)
        ? 0.0
        : 0.5 * (ambientPressure * m_engine->getDisplacement())
            / (constants::R * ambientTemperature);
    const double theoreticalAirPerSecond = theoreticalAirPerRevolution * rpm / 60.0;
    const double actualAirPerSecond = (m_engine == nullptr)
        ? 0.0
        : m_engine->getIntakeFlowRate();
    const double volumetricEfficiency = (std::abs(theoreticalAirPerSecond) < 1E-3)
        ? 0.0
        : (actualAirPerSecond / theoreticalAirPerSecond);

    const Bounds cfmBounds = grid.get(right, 0, 1, 1, 1);
    m_intakeCfmGauge->m_bounds = cfmBounds;
    m_intakeCfmGauge->m_gauge->m_value =
        (float)units::convert(actualAirPerSecond, units::scfm);

    const Bounds volumetricEfficiencyBounds = grid.get(right, 0, 2, 1, 1);
    m_volumetricEffGauge->m_bounds = volumetricEfficiencyBounds;
    m_volumetricEffGauge->m_gauge->m_value = 100.0f * (float)volumetricEfficiency;
}

double RightGaugeCluster::getManifoldPressureWithUnits(double ambientPressure) {
    if (m_pressureUnits == "inHg") {
        return units::convert(std::fmin(getManifoldPressure() - ambientPressure, 0.0), units::inHg);
    }
    else if (m_pressureUnits == "kPa") {
        return units::convert(getManifoldPressure(), units::kPa);
    }
    else if (m_pressureUnits == "mbar") {
        return units::convert(getManifoldPressure(), units::mbar);
    }
    else if (m_pressureUnits == "bar") {
        return units::convert(getManifoldPressure(), units::bar);
    }
    else if (m_pressureUnits == "psi") {
        return units::convert(std::fmin(getManifoldPressure() - ambientPressure, 0.0), units::psi);
    }
    else {
        return units::convert(std::fmin(getManifoldPressure() - ambientPressure, 0.0), units::inHg);
    }
}

double RightGaugeCluster::getRpm() const {
    return (m_engine != nullptr)
        ? m_engine->getRpm()
        : 0;
}

double RightGaugeCluster::getRedline() const {
    return (m_engine != nullptr)
        ? m_engine->getRedline()
        : 0;
}

double RightGaugeCluster::getSpeed() const {
    return (m_simulator->getVehicle() != nullptr)
        ? m_simulator->getVehicle()->getSpeed()
        : 0;
}

double RightGaugeCluster::getManifoldPressure() const {
    return (m_engine != nullptr)
        ? m_engine->getManifoldPressure()
        : units::pressure(1.0, units::atm);
}

void RightGaugeCluster::setUnits() {
    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);

    m_speedometer->m_unit = (m_speedUnits == "mph")
        ? "mph"
        : "kph";

    if (m_pressureUnits == "kPa") {
        m_isAbsolute = true;

        m_manifoldVacuumGauge->m_unit = "kPa";
        m_manifoldVacuumGauge->m_gauge->m_min = 0;
        m_manifoldVacuumGauge->m_gauge->m_max = 110;
        m_manifoldVacuumGauge->m_gauge->m_minorStep = 5;
        m_manifoldVacuumGauge->m_gauge->m_majorStep = 10;
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getRed(), 110, 90, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), -1.0f, 1.0f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 1);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getOrange(), 30, 40, 3.0f, 6.0f, shortenAngle, shortenAngle }, 2);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getBlue(), 15, 29, 3.0f, 6.0f, shortenAngle, shortenAngle }, 3);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), 0, 14, 3.0f, 6.0f, shortenAngle, shortenAngle }, 4);
    }
    else if (m_pressureUnits == "mbar") {
        m_isAbsolute = true;

        m_manifoldVacuumGauge->m_unit = "mbar";
        m_manifoldVacuumGauge->m_gauge->m_min = 0;
        m_manifoldVacuumGauge->m_gauge->m_max = 1100;
        m_manifoldVacuumGauge->m_gauge->m_minorStep = 50;
        m_manifoldVacuumGauge->m_gauge->m_majorStep = 100;
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getRed(), 1100, 900, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), -1.0f, 1.0f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 1);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getOrange(), 300, 400, 3.0f, 6.0f, shortenAngle, shortenAngle }, 2);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getBlue(), 150, 290, 3.0f, 6.0f, shortenAngle, shortenAngle }, 3);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), 0, 140, 3.0f, 6.0f, shortenAngle, shortenAngle }, 4);
    }
    else if (m_pressureUnits == "bar") {
        m_isAbsolute = true;

        m_manifoldVacuumGauge->m_unit = "bar";
        m_manifoldVacuumGauge->m_gauge->m_min = 0;
        m_manifoldVacuumGauge->m_gauge->m_max = 1.1f;
        m_manifoldVacuumGauge->m_gauge->m_minorStep = 1;
        m_manifoldVacuumGauge->m_gauge->m_majorStep = 1;
        m_manifoldVacuumGauge->m_precision = 2;

        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getRed(), 0.8f, 1.1f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), -1.0f, 1.0f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 1);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getOrange(), 0.3f, 0.5f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 2);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getBlue(), 0.15f, 0.29f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 3);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), 0, 0.14f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 4);
    }
    else if (m_pressureUnits == "psi") {
        m_isAbsolute = false;

        m_manifoldVacuumGauge->m_unit = "psi";
        m_manifoldVacuumGauge->m_gauge->m_min = -15;
        m_manifoldVacuumGauge->m_gauge->m_max = 3;
        m_manifoldVacuumGauge->m_gauge->m_minorStep = 1;
        m_manifoldVacuumGauge->m_gauge->m_majorStep = 5;
        m_manifoldVacuumGauge->m_precision = 1;
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getRed(), -4, 1, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), -1.0f, 1.0f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 1);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getOrange(), -7, -4, 3.0f, 6.0f, shortenAngle, shortenAngle }, 2);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getBlue(), -12, -7, 3.0f, 6.0f, shortenAngle, shortenAngle }, 3);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), -15, -12, 3.0f, 6.0f, shortenAngle, shortenAngle }, 4);
    }
    else {
        m_isAbsolute = false;

        m_manifoldVacuumGauge->m_unit = "inHg";
        m_manifoldVacuumGauge->m_gauge->m_min = -30;
        m_manifoldVacuumGauge->m_gauge->m_max = 5;
        m_manifoldVacuumGauge->m_gauge->m_minorStep = 1;
        m_manifoldVacuumGauge->m_gauge->m_majorStep = 5;
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getRed(), -5, -1, 3.0f, 6.0f, shortenAngle, shortenAngle }, 0);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), -1.0f, 1.0f, 3.0f, 6.0f, shortenAngle, shortenAngle }, 1);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getOrange(), -10, -5, 3.0f, 6.0f, shortenAngle, shortenAngle }, 2);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getBlue(), -22, -10, 3.0f, 6.0f, shortenAngle, shortenAngle }, 3);
        m_manifoldVacuumGauge->m_gauge->setBand(
            { m_app->getForegroundColor(), -30, -22, 3.0f, 6.0f, shortenAngle, shortenAngle }, 4);
    }
}
