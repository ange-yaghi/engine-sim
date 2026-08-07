#include "../include/dyno_cluster.h"

#include "../include/engine_sim_application.h"

#include <sstream>
#include <iomanip>

DynoCluster::DynoCluster() {
    m_torqueChart = nullptr;
    m_powerChart = nullptr;
    m_newPullButton = nullptr;

    m_simulator = nullptr;

    m_filteredTorque = 0.0;
    m_filteredPower = 0.0;

    m_updatePeriod = 0.25f;
    m_updateTimer = 0.0f;
}

DynoCluster::~DynoCluster() {
    /* void */
}

void DynoCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_torqueChart = addElement<DynoChart>();
    m_torqueChart->m_title = "TORQUE";
    m_torqueChart->m_lineColor = m_app->getOrange();

    m_powerChart = addElement<DynoChart>();
    m_powerChart->m_title = "POWER";
    m_powerChart->m_lineColor = m_app->getPink();

    m_newPullButton = addElement<UiButton>(this);
    m_newPullButton->m_text = "NEW PULL";

    m_torqueUnits = app->getAppSettings()->torqueUnits;
    m_powerUnits = app->getAppSettings()->powerUnits;
    setUnits();
}

void DynoCluster::destroy() {
    UiElement::destroy();
}

void DynoCluster::signal(UiElement *element, Event event) {
    if (event != Event::Clicked) return;

    if (element == m_newPullButton) {
        newPull();
    }
}

void DynoCluster::setUnits() {
    m_torqueChart->m_unit = m_torqueUnits;
    m_torqueChart->m_precision = (m_torqueUnits == "Nm") ? 1 : 0;

    m_powerChart->m_unit = m_powerUnits;
    m_powerChart->m_precision = (m_powerUnits == "kW") ? 1 : 0;
}

void DynoCluster::newPull() {
    m_torqueChart->reset();
    m_powerChart->reset();
    m_filteredTorque = 0.0;
    m_filteredPower = 0.0;
}

void DynoCluster::update(float dt) {
    UiElement::update(dt);

    if (m_simulator == nullptr) return;

    Engine *engine = m_simulator->getEngine();
    if (engine == nullptr) return;

    const double redlineRpm = units::toRpm(engine->getRedline());
    m_torqueChart->m_xMax = redlineRpm * 1.05;
    m_torqueChart->m_redlineRpm = redlineRpm;
    m_powerChart->m_xMax = redlineRpm * 1.05;
    m_powerChart->m_redlineRpm = redlineRpm;

    const double torque = (m_torqueUnits == "Nm")
        ? units::convert(m_simulator->getFilteredDynoTorque(), units::Nm)
        : units::convert(m_simulator->getFilteredDynoTorque(), units::ft_lb);

    const double power = (m_powerUnits == "kW")
        ? units::convert(m_simulator->getDynoPower(), units::kW)
        : units::convert(m_simulator->getDynoPower(), units::hp);

    m_filteredTorque = m_filteredTorque * 0.95 + 0.05 * torque;
    m_filteredPower = m_filteredPower * 0.95 + 0.05 * power;

    if (m_updateTimer <= 0 && m_simulator->m_dyno.m_enabled) {
        m_updateTimer = m_updatePeriod;

        m_torqueChart->addDataPoint(engine->getRpm(), m_filteredTorque);
        m_powerChart->addDataPoint(engine->getRpm(), m_filteredPower);
    }

    m_updateTimer -= dt;
}

void DynoCluster::render() {
    const Bounds statRowBounds = m_bounds.verticalSplit(0.0f, 0.12f);
    const Bounds chartsBounds = m_bounds.verticalSplit(0.12f, 1.0f);

    m_torqueChart->m_bounds = chartsBounds.verticalSplit(0.51f, 1.0f).inset(6.0f);
    m_powerChart->m_bounds = chartsBounds.verticalSplit(0.0f, 0.49f).inset(6.0f);

    drawPanel(statRowBounds);

    Grid statGrid;
    statGrid.h_cells = 3;
    statGrid.v_cells = 1;

    std::stringstream tq;
    tq << std::fixed << std::setprecision(m_torqueChart->m_precision)
       << m_torqueChart->m_peakValue << " " << m_torqueUnits << " PEAK TORQUE @ "
       << std::setprecision(0) << m_torqueChart->m_peakRpm << " RPM";

    m_app->getTextRenderer()->SetColor(m_app->getOrange());
    drawCenteredText(tq.str(), statGrid.get(statRowBounds, 0, 0).inset(10.0f), 22.0f);

    std::stringstream pw;
    pw << std::fixed << std::setprecision(m_powerChart->m_precision)
       << m_powerChart->m_peakValue << " " << m_powerUnits << " PEAK POWER @ "
       << std::setprecision(0) << m_powerChart->m_peakRpm << " RPM";

    m_app->getTextRenderer()->SetColor(m_app->getPink());
    drawCenteredText(pw.str(), statGrid.get(statRowBounds, 1, 0).inset(10.0f), 22.0f);

    m_app->getTextRenderer()->SetColor(m_app->getForegroundColor());

    m_newPullButton->m_bounds = statGrid.get(statRowBounds, 2, 0).inset(10.0f);

    UiElement::render();
}
