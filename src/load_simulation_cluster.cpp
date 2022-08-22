#include "../include/load_simulation_cluster.h"

#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

#include <sstream>
#include <iomanip>

LoadSimulationCluster::LoadSimulationCluster() {
    m_torqueGauge = nullptr;
    m_hpGauge = nullptr;
    m_simulator = nullptr;
    m_clutchPressureGauge = nullptr;
    m_filteredHorsepower = 0.0;
    m_filteredTorque = 0.0;
    m_peakHorsepower = 0.0;
    m_peakTorque = 0.0;
    m_peakHorsepowerRpm = 0.0;
    m_peakTorqueRpm = 0.0;
    memset(m_systemStatusLights, 0, sizeof(double) * 4);
}

LoadSimulationCluster::~LoadSimulationCluster() {
    /* void */
}

void LoadSimulationCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);

    m_dynoSpeedGauge = addElement<LabeledGauge>();
    m_dynoSpeedGauge->m_title = "DYNO. SPEED";
    m_dynoSpeedGauge->m_unit = "RPM";
    m_dynoSpeedGauge->m_precision = 0;
    m_dynoSpeedGauge->setLocalPosition({ 0, 0 });
    m_dynoSpeedGauge->m_gauge->m_min = 0;
    m_dynoSpeedGauge->m_gauge->m_max = 100;
    m_dynoSpeedGauge->m_gauge->m_minorStep = 500;
    m_dynoSpeedGauge->m_gauge->m_majorStep = 1000;
    m_dynoSpeedGauge->m_gauge->m_maxMinorTick = INT_MAX;
    m_dynoSpeedGauge->m_gauge->m_thetaMin = (float)constants::pi * 0.8f;
    m_dynoSpeedGauge->m_gauge->m_thetaMax = (float)constants::pi * 0.2f;
    m_dynoSpeedGauge->m_gauge->m_needleWidth = 4.0f;
    m_dynoSpeedGauge->m_gauge->m_gamma = 1.0f;
    m_dynoSpeedGauge->m_gauge->m_needleKs = 1000.0f;
    m_dynoSpeedGauge->m_gauge->m_needleKd = 5.0f;
    m_dynoSpeedGauge->m_gauge->setBandCount(0);

    m_torqueGauge = addElement<LabeledGauge>();
    m_torqueGauge->m_title = "TORQUE";
    m_torqueGauge->m_unit = "LB-FT";
    m_torqueGauge->m_precision = 0;
    m_torqueGauge->setLocalPosition({ 0, 0 });
    m_torqueGauge->m_gauge->m_min = 0;
    m_torqueGauge->m_gauge->m_max = 1000;
    m_torqueGauge->m_gauge->m_minorStep = 50;
    m_torqueGauge->m_gauge->m_majorStep = 100;
    m_torqueGauge->m_gauge->m_maxMinorTick = INT_MAX;
    m_torqueGauge->m_gauge->m_thetaMin = (float)constants::pi * 0.8f;
    m_torqueGauge->m_gauge->m_thetaMax = (float)constants::pi * 0.2f;
    m_torqueGauge->m_gauge->m_needleWidth = 4.0f;
    m_torqueGauge->m_gauge->m_gamma = 1.0f;
    m_torqueGauge->m_gauge->m_needleKs = 1000.0f;
    m_torqueGauge->m_gauge->m_needleKd = 5.0f;
    m_torqueGauge->m_gauge->setBandCount(0);

    m_hpGauge = addElement<LabeledGauge>();
    m_hpGauge->m_title = "POWER";
    m_hpGauge->m_unit = "HP";
    m_hpGauge->m_precision = 0;
    m_hpGauge->setLocalPosition({ 0, 0 });
    m_hpGauge->m_gauge->m_min = 0;
    m_hpGauge->m_gauge->m_max = 1000;
    m_hpGauge->m_gauge->m_minorStep = 50;
    m_hpGauge->m_gauge->m_majorStep = 100;
    m_hpGauge->m_gauge->m_maxMinorTick = INT_MAX;
    m_hpGauge->m_gauge->m_thetaMin = (float)constants::pi * 0.8f;
    m_hpGauge->m_gauge->m_thetaMax = (float)constants::pi * 0.2f;
    m_hpGauge->m_gauge->m_needleWidth = 4.0f;
    m_hpGauge->m_gauge->m_gamma = 1.0f;
    m_hpGauge->m_gauge->m_needleKs = 1000.0f;
    m_hpGauge->m_gauge->m_needleKd = 5.0f;
    m_hpGauge->m_gauge->setBandCount(0);

    m_clutchPressureGauge = addElement<LabeledGauge>();
    m_clutchPressureGauge->m_title = "CLUTCH";
    m_clutchPressureGauge->m_unit = "";
    m_clutchPressureGauge->m_spaceBeforeUnit = false;
    m_clutchPressureGauge->m_precision = 0;
    m_clutchPressureGauge->setLocalPosition({ 0, 0 });
    m_clutchPressureGauge->m_gauge->m_min = 0;
    m_clutchPressureGauge->m_gauge->m_max = 100;
    m_clutchPressureGauge->m_gauge->m_minorStep = 10;
    m_clutchPressureGauge->m_gauge->m_majorStep = 50;
    m_clutchPressureGauge->m_gauge->m_maxMinorTick = 200;
    m_clutchPressureGauge->m_gauge->m_thetaMin = (float)constants::pi * 0.8f;
    m_clutchPressureGauge->m_gauge->m_thetaMax = (float)constants::pi * 0.2f;
    m_clutchPressureGauge->m_gauge->m_needleWidth = 4.0f;
    m_clutchPressureGauge->m_gauge->m_gamma = 1.0f;
    m_clutchPressureGauge->m_gauge->m_needleKs = 1000.0f;
    m_clutchPressureGauge->m_gauge->m_needleKd = 5.0f;
    m_clutchPressureGauge->m_gauge->m_needleMaxVelocity = 10.0f;
    m_clutchPressureGauge->m_gauge->setBandCount(0);

    m_torqueUnits = app->getAppSettings()->torqueUnits;
    m_powerUnits = app->getAppSettings()->powerUnits;
    setUnits();
}

void LoadSimulationCluster::destroy() {
    /* void */
}

void LoadSimulationCluster::update(float dt) {
    UiElement::update(dt);

    const float systemStatuses[] = {
        m_simulator->getEngine()->getIgnitionModule()->m_enabled ? 1.0f : 0.01f,
        m_simulator->m_starterMotor.m_enabled ? 1.0f : 0.01f,
        m_simulator->m_dyno.m_enabled ? 1.0f : 0.01f,
        m_simulator->m_dyno.m_hold ? (m_simulator->m_dyno.m_enabled ? 1.0f : 0.25f) : 0.01f
    };

    constexpr float RC = 0.08f;
    const float alpha = dt / (dt + RC);

    for (int i = 0; i < 4; ++i) {
        const float next = systemStatuses[i];
        m_systemStatusLights[i] = (1 - alpha) * m_systemStatusLights[i] + alpha * next;
    }

    if (m_app->getEngine()->ProcessKeyDown(ysKey::Code::I)) {
        std::stringstream ss;
        ss << std::setprecision(0) << std::fixed;
        if (m_powerUnits == "HP")
            ss << m_peakHorsepower << "HP @ " << m_peakHorsepowerRpm << "rpm";
        else
            ss << m_peakHorsepower << "KW @ " << m_peakHorsepowerRpm << "rpm";
        ss << " | ";
        if (m_torqueUnits == "FTLBS")
            ss << m_peakTorque << "lb-ft @ " << m_peakTorqueRpm << "rpm";
        else
            ss << m_peakTorque << "Nm. @ " << m_peakTorqueRpm << "rpm";
        m_app->getInfoCluster()->setLogMessage(ss.str());
    }

    updateHpAndTorque(dt);
}

void LoadSimulationCluster::render() {
    Grid grid;
    grid.h_cells = 3;
    grid.v_cells = 2;

    const Bounds gearBounds = grid.get(m_bounds, 2, 0);
    drawCurrentGear(gearBounds);

    const Bounds clutchBounds = grid.get(m_bounds, 1, 0);
    drawClutchPressureGauge(clutchBounds);

    const Bounds systemStatusBounds = grid.get(m_bounds, 0, 0);
    drawSystemStatus(systemStatusBounds);

    const Bounds dynoSpeedBounds = grid.get(m_bounds, 0, 1);
    m_dynoSpeedGauge->m_gauge->m_value = 
       (float)units::toRpm(std::abs(m_simulator->m_dyno.m_rotationSpeed));
    m_dynoSpeedGauge->m_bounds = dynoSpeedBounds;

    constexpr float shortenAngle = (float)units::angle(1.0, units::deg);
    const double redline = units::toRpm(m_simulator->getEngine()->getRedline());
    const double maxRpm = std::floor(redline / 500.0) * 500.0;
    m_dynoSpeedGauge->m_gauge->m_max = (int)(maxRpm);
    m_dynoSpeedGauge->m_gauge->setBandCount(1);
    m_dynoSpeedGauge->m_gauge->setBand(
        { m_app->getRed(), (float)redline, (float)maxRpm, 3.0f, 6.0f, shortenAngle, -shortenAngle }, 0);

    const Bounds torqueBounds = grid.get(m_bounds, 1, 1);
    m_torqueGauge->m_gauge->m_value = m_simulator->m_dyno.m_enabled
        ? (float)m_filteredTorque
        : (float)m_peakTorque;
    m_torqueGauge->m_bounds = torqueBounds;

    const Bounds horsepowerBounds = grid.get(m_bounds, 2, 1);
    m_hpGauge->m_gauge->m_value = m_simulator->m_dyno.m_enabled
        ? (float)m_filteredHorsepower
        : (float)m_peakHorsepower;
    m_hpGauge->m_bounds = horsepowerBounds;

    UiElement::render();
}

void LoadSimulationCluster::drawCurrentGear(const Bounds &bounds) {
    const Bounds insetBounds = bounds.inset(10.0f);
    const Bounds title = insetBounds.verticalSplit(0.9f, 1.0f);
    const Bounds body = insetBounds.verticalSplit(0.0f, 0.9f);

    drawFrame(bounds, 1.0f, m_app->getWhite(), m_app->getBackgroundColor());
    drawCenteredText("Gear", title.inset(10.0f), 24.0f);

    const int gear = m_simulator->getTransmission()->getGear();
    std::stringstream ss;
    
    if (gear != -1) ss << (gear + 1);
    else ss << "N";

    drawCenteredText(ss.str(), body, 64.0f, Bounds::center);
}

void LoadSimulationCluster::drawClutchPressureGauge(const Bounds &bounds) {
    m_clutchPressureGauge->m_bounds = bounds;
    m_clutchPressureGauge->m_gauge->m_value =
        (float)m_simulator->getTransmission()->getClutchPressure() * 100.0f;
}

void LoadSimulationCluster::drawSystemStatus(const Bounds &bounds) {
    const Bounds left = bounds.horizontalSplit(0.0f, 0.6f);
    const Bounds right = bounds.horizontalSplit(0.6f, 1.0f);

    drawFrame(bounds, 1.0f, m_app->getWhite(), m_app->getBackgroundColor());

    Grid grid;
    grid.v_cells = 4;
    grid.h_cells = 1;

    drawText(
        "Ignition",
        grid.get(left, 0, 0).inset(10.0f),
        20.0,
        Bounds::lm);
    drawText(
        "Starter",
        grid.get(left, 0, 1).inset(10.0f),
        20.0,
        Bounds::lm);
    drawText(
        "Dyno.",
        grid.get(left, 0, 2).inset(10.0f),
        20.0,
        Bounds::lm);
    drawText(
        "Hold",
        grid.get(left, 0, 3).inset(10.0f),
        20.0,
        Bounds::lm);

    for (int i = 0; i < 4; ++i) {
        const Bounds rawBounds = grid.get(right, 0, i);
        const float width = std::fmax(rawBounds.width(), rawBounds.height());
        const Bounds squareBounds(width - 20.0f, 5.0f, rawBounds.getPosition(Bounds::center), Bounds::center);

        drawFrame(
            squareBounds,
            1.0f,
            mix(m_app->getBackgroundColor(), m_app->getWhite(), 0.001f),
            mix(m_app->getBackgroundColor(), m_app->getRed(), m_systemStatusLights[i])
        );
    }
}

void LoadSimulationCluster::updateHpAndTorque(float dt) {
    constexpr double RC = 1.0;
    const double alpha = dt / (dt + RC);

    double torque = (m_torqueUnits == "NM") ? (units::convert(m_simulator->getFilteredDynoTorque(), units::Nm)) : (units::convert(m_simulator->getFilteredDynoTorque(), units::ft_lb));
    const double hp = getPower(torque);

    m_filteredTorque = (1 - alpha) * m_filteredTorque + alpha * torque;
    m_filteredHorsepower = (1 - alpha) * m_filteredHorsepower + alpha * hp;

    if (m_filteredTorque > m_peakTorque) {
        m_peakTorque = m_filteredTorque;
        m_peakTorqueRpm = m_simulator->getEngine()->getRpm();
    }

    if (m_filteredHorsepower > m_peakHorsepower) {
        m_peakHorsepower = std::fmax(m_peakHorsepower, m_filteredHorsepower);
        m_peakHorsepowerRpm = m_simulator->getEngine()->getRpm();
    }
}


double LoadSimulationCluster::getPower(double torque)
{
    double power = 0;
    if (m_powerUnits == "HP")
    {
        if(m_torqueUnits == "NM")
            power = torque * m_simulator->getEngine()->getRpm() / 7127.0;
        else
            power = torque * m_simulator->getEngine()->getRpm() / 5252.0;
    }
    else if (m_powerUnits == "KW")
    {
        if (m_torqueUnits == "NM")
            power = torque * m_simulator->getEngine()->getRpm() / 9549.0;
        else
            power = torque * m_simulator->getEngine()->getRpm() / 7127.0;
    }
    return power;
}
void LoadSimulationCluster::setUnits(){
    //Set Torque First
    if (m_torqueUnits == "FTLBS")
    {
        m_torqueGauge->m_unit = "LB-FT";
        m_torqueGauge->m_precision = 0;
        m_torqueGauge->m_gauge->m_min = 0;
        m_torqueGauge->m_gauge->m_max = 1000;
        m_torqueGauge->m_gauge->m_minorStep = 50;
        m_torqueGauge->m_gauge->m_majorStep = 100;
    }
    else if (m_torqueUnits == "NM")
    {
        m_torqueGauge->m_unit = "Nm";
        m_torqueGauge->m_precision = 1;
        m_torqueGauge->m_gauge->m_min = 0;
        m_torqueGauge->m_gauge->m_max = 1000;
        m_torqueGauge->m_gauge->m_minorStep = 50;
        m_torqueGauge->m_gauge->m_majorStep = 100;
    }

    //Then Power
    if (m_powerUnits == "HP")
    {
        m_hpGauge->m_unit = "HP";
        m_hpGauge->m_precision = 0;

        m_hpGauge->m_gauge->m_min = 0;
        m_hpGauge->m_gauge->m_max = 1000;
        m_hpGauge->m_gauge->m_minorStep = 50;
        m_hpGauge->m_gauge->m_majorStep = 100;
    }
    else if (m_powerUnits == "KW")
    {
        m_hpGauge->m_unit = "Kw";
        m_hpGauge->m_precision = 1;
        m_hpGauge->m_gauge->m_min = 0;
        m_hpGauge->m_gauge->m_max = 1000;
        m_hpGauge->m_gauge->m_minorStep = 50;
        m_hpGauge->m_gauge->m_majorStep = 100;
    }
}