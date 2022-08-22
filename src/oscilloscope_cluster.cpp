#include "../include/oscilloscope_cluster.h"

#include "../include/engine_sim_application.h"

#include <sstream>

OscilloscopeCluster::OscilloscopeCluster() {
    m_simulator = nullptr;
    m_torqueScope = nullptr;
    m_hpScope = nullptr;
    m_totalExhaustFlowScope = nullptr;
    m_intakeFlowScope = nullptr;
    m_exhaustFlowScope = nullptr;
    m_exhaustValveLiftScope = nullptr;
    m_intakeValveLiftScope = nullptr;
    m_audioWaveformScope = nullptr;
    m_cylinderPressureScope = nullptr;
    m_sparkAdvanceScope = nullptr;
    m_cylinderMoleculesScope = nullptr;
    m_pvScope = nullptr;

    for (int i = 0; i < MaxLayeredScopes; ++i) {
        m_currentFocusScopes[i] = nullptr;
    }

    m_torque = 0;
    m_hp = 0;

    m_updatePeriod = 0.25f;
    m_updateTimer = 0.0f;
}

OscilloscopeCluster::~OscilloscopeCluster() {
    /* void */
}

void OscilloscopeCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_torqueScope = addElement<Oscilloscope>(this);
    m_hpScope = addElement<Oscilloscope>(this);
    m_exhaustFlowScope = addElement<Oscilloscope>(this);
    m_totalExhaustFlowScope = addElement<Oscilloscope>(this);
    m_intakeFlowScope = addElement<Oscilloscope>(this);
    m_audioWaveformScope = addElement<Oscilloscope>(this);
    m_intakeValveLiftScope = addElement<Oscilloscope>(this);
    m_exhaustValveLiftScope = addElement<Oscilloscope>(this);
    m_cylinderPressureScope = addElement<Oscilloscope>(this);
    m_sparkAdvanceScope = addElement<Oscilloscope>(this);
    m_cylinderMoleculesScope = addElement<Oscilloscope>(this);
    m_pvScope = addElement<Oscilloscope>(this);

    // Torque
    m_torqueScope->setBufferSize(100);
    m_torqueScope->m_xMin = 0.0f;
    m_torqueScope->m_yMin = 0.0f;
    m_torqueScope->m_yMax = 0.0f;
    m_torqueScope->m_lineWidth = 2.0f;
    m_torqueScope->m_drawReverse = false;
    m_torqueScope->m_dynamicallyResizeX = true;
    m_torqueScope->i_color = m_app->getOrange();

    // Horsepower
    m_hpScope->setBufferSize(100);
    m_hpScope->m_xMin = 0.0f;
    m_hpScope->m_yMin = 0.0f;
    m_hpScope->m_yMax = 0.0f;
    m_hpScope->m_lineWidth = 2.0f;
    m_hpScope->m_drawReverse = false;
    m_hpScope->m_dynamicallyResizeX = true;
    m_hpScope->i_color = m_app->getPink();

    // Total exhaust flow
    m_totalExhaustFlowScope->setBufferSize(1024);
    m_totalExhaustFlowScope->m_xMin = 0.0f;
    m_totalExhaustFlowScope->m_xMax = constants::pi * 4;
    m_totalExhaustFlowScope->m_yMin = -units::flow(10, units::scfm);
    m_totalExhaustFlowScope->m_yMax = units::flow(10, units::scfm);
    m_totalExhaustFlowScope->m_lineWidth = 2.0f;
    m_totalExhaustFlowScope->m_drawReverse = false;
    m_totalExhaustFlowScope->i_color = m_app->getOrange();

    // Exhaust flow
    m_exhaustFlowScope->setBufferSize(1024);
    m_exhaustFlowScope->m_xMin = 0.0f;
    m_exhaustFlowScope->m_xMax = constants::pi * 4;
    m_exhaustFlowScope->m_yMin = -units::flow(10.0, units::scfm);
    m_exhaustFlowScope->m_yMax = units::flow(10.0, units::scfm);
    m_exhaustFlowScope->m_lineWidth = 2.0f;
    m_exhaustFlowScope->m_drawReverse = false;
    m_exhaustFlowScope->i_color = m_app->getOrange();

    // Intake flow
    m_intakeFlowScope->setBufferSize(1024);
    m_intakeFlowScope->m_xMin = 0.0f;
    m_intakeFlowScope->m_xMax = constants::pi * 4;
    m_intakeFlowScope->m_yMin = -units::flow(10.0, units::scfm);
    m_intakeFlowScope->m_yMax = units::flow(10.0, units::scfm);
    m_intakeFlowScope->m_lineWidth = 2.0f;
    m_intakeFlowScope->m_drawReverse = false;
    m_intakeFlowScope->i_color = m_app->getBlue();

    // Cylinder molcules
    m_cylinderMoleculesScope->setBufferSize(1024);
    m_cylinderMoleculesScope->m_xMin = 0.0f;
    m_cylinderMoleculesScope->m_xMax = constants::pi * 4;
    m_cylinderMoleculesScope->m_yMin = -0.05;
    m_cylinderMoleculesScope->m_yMax = 0.2;
    m_cylinderMoleculesScope->m_lineWidth = 4.0f;
    m_cylinderMoleculesScope->m_drawReverse = false;
    m_cylinderMoleculesScope->i_color = m_app->getWhite();

    // Audio waveform scope
    m_audioWaveformScope->setBufferSize(44100 / 50);
    m_audioWaveformScope->m_xMin = 0.0f;
    m_audioWaveformScope->m_xMax = 44100 / 10;
    m_audioWaveformScope->m_yMin = -1.5f;
    m_audioWaveformScope->m_yMax = 1.5f;
    m_audioWaveformScope->m_lineWidth = 2.0f;
    m_audioWaveformScope->m_drawReverse = false;
    m_audioWaveformScope->i_color = m_app->getBlue();

    // Valve lift scopes
    m_exhaustValveLiftScope->setBufferSize(1024);
    m_exhaustValveLiftScope->m_xMin = 0.0f;
    m_exhaustValveLiftScope->m_xMax = constants::pi * 4;
    m_exhaustValveLiftScope->m_yMin = (float)units::distance(-10, units::thou);
    m_exhaustValveLiftScope->m_yMax = (float)units::distance(10, units::thou);
    m_exhaustValveLiftScope->m_lineWidth = 2.0f;
    m_exhaustValveLiftScope->m_drawReverse = false;
    m_exhaustValveLiftScope->i_color = m_app->getOrange();

    m_intakeValveLiftScope->setBufferSize(1024);
    m_intakeValveLiftScope->m_xMin = 0.0f;
    m_intakeValveLiftScope->m_xMax = constants::pi * 4;
    m_intakeValveLiftScope->m_yMin = (float)units::distance(-10, units::thou);
    m_intakeValveLiftScope->m_yMax = (float)units::distance(10, units::thou);
    m_intakeValveLiftScope->m_lineWidth = 2.0f;
    m_intakeValveLiftScope->m_drawReverse = false;
    m_intakeValveLiftScope->i_color = m_app->getBlue();

    // Cylinder pressure scope
    m_cylinderPressureScope->setBufferSize(1024);
    m_cylinderPressureScope->m_xMin = 0.0f;
    m_cylinderPressureScope->m_xMax = constants::pi * 4;
    m_cylinderPressureScope->m_yMin = -(float)std::sqrt(units::pressure(1, units::psi));
    m_cylinderPressureScope->m_yMax = (float)std::sqrt(units::pressure(1, units::psi));
    m_cylinderPressureScope->m_lineWidth = 2.0f;
    m_cylinderPressureScope->m_drawReverse = false;
    m_cylinderPressureScope->i_color = m_app->getOrange();

    // Pressure volume scope
    m_pvScope->setBufferSize(1024);
    m_pvScope->m_xMin = 0.0f;
    m_pvScope->m_xMax = units::volume(0.1, units::L);
    m_pvScope->m_yMin = -(float)std::sqrt(units::pressure(1, units::psi));
    m_pvScope->m_yMax = (float)std::sqrt(units::pressure(1, units::psi));
    m_pvScope->m_lineWidth = 2.0f;
    m_pvScope->m_drawReverse = true;
    m_pvScope->i_color = m_app->getOrange();
    m_pvScope->m_dynamicallyResizeX = true;

    // Spark advance scope
    m_sparkAdvanceScope->setBufferSize(1024);
    m_sparkAdvanceScope->m_xMin = 0.0f;
    m_sparkAdvanceScope->m_xMax = units::rpm(10000);
    m_sparkAdvanceScope->m_yMin = -units::angle(30, units::deg);
    m_sparkAdvanceScope->m_yMax = units::angle(60, units::deg);
    m_sparkAdvanceScope->m_lineWidth = 2.0f;
    m_sparkAdvanceScope->m_drawReverse = true;
    m_sparkAdvanceScope->i_color = m_app->getOrange();

    m_currentFocusScopes[0] = m_totalExhaustFlowScope;
    m_currentFocusScopes[1] = nullptr;

    m_torqueUnits = app->getAppSettings()->torqueUnits;
    m_powerUnits = app->getAppSettings()->powerUnits;
}

void OscilloscopeCluster::destroy() {
    UiElement::destroy();
}

void OscilloscopeCluster::signal(UiElement *element, Event event) {
    if (event == Event::Clicked) {
        if (element == m_audioWaveformScope) {
            m_currentFocusScopes[0] = m_audioWaveformScope;
            m_currentFocusScopes[1] = nullptr;
        }
        else if (element == m_hpScope || element == m_torqueScope) {
            m_currentFocusScopes[0] = m_torqueScope;
            m_currentFocusScopes[1] = m_hpScope;
            m_currentFocusScopes[2] = nullptr;
        }
        else if (element == m_totalExhaustFlowScope) {
            m_currentFocusScopes[0] = m_totalExhaustFlowScope;
            m_currentFocusScopes[1] = nullptr;
        }
        else if (
            element == m_intakeValveLiftScope
            || element == m_exhaustValveLiftScope)
        {
            m_currentFocusScopes[0] = m_intakeValveLiftScope;
            m_currentFocusScopes[1] = m_exhaustValveLiftScope;
            m_currentFocusScopes[2] = nullptr;
        }
        else if (element == m_pvScope) {
            m_currentFocusScopes[0] = m_pvScope;
            m_currentFocusScopes[1] = nullptr;
        }
        else if (
            element == m_intakeFlowScope
            || element == m_exhaustFlowScope
            || element == m_cylinderMoleculesScope)
        {
            m_currentFocusScopes[0] = m_intakeFlowScope;
            m_currentFocusScopes[1] = m_exhaustFlowScope;
            m_currentFocusScopes[2] = m_cylinderMoleculesScope;
            m_currentFocusScopes[3] = nullptr;
        }
        else if (element == m_cylinderPressureScope) {
            m_currentFocusScopes[0] = m_cylinderPressureScope;
            m_currentFocusScopes[1] = nullptr;
        }
        else if (element == m_sparkAdvanceScope) {
            m_currentFocusScopes[0] = m_sparkAdvanceScope;
            m_currentFocusScopes[1] = nullptr;
        }
    }
}

void OscilloscopeCluster::update(float dt) {
    Engine *engine = m_simulator->getEngine();

    const double torque =
        (m_torqueUnits == "NM") ? (units::convert(m_simulator->getFilteredDynoTorque(), units::Nm)) : (units::convert(m_simulator->getFilteredDynoTorque(), units::ft_lb));

    const double hp = getPower(torque);

    m_torque = m_torque * 0.95 + 0.05 * torque;
    m_hp = m_hp * 0.95 + 0.05 * hp;

    if (m_updateTimer <= 0 && m_simulator->m_dyno.m_enabled) {
        m_updateTimer = m_updatePeriod;
        
        m_torqueScope->addDataPoint(engine->getRpm(), m_torque);
        m_hpScope->addDataPoint(engine->getRpm(), m_hp);
    }

    m_sparkAdvanceScope->addDataPoint(
        -m_simulator->getEngine()->getCrankshaft(0)->m_body.v_theta,
        m_simulator->getEngine()->getIgnitionModule()->getTimingAdvance());

    m_updateTimer -= dt;

    UiElement::update(dt);
}

void OscilloscopeCluster::render() {
    Grid grid;
    grid.h_cells = 3;
    grid.v_cells = 4;

    const Bounds &hpTorqueBounds = grid.get(m_bounds, 0, 3);
    renderScope(m_torqueScope, hpTorqueBounds, "Torque/Power");
    renderScope(m_hpScope, hpTorqueBounds, "", true);

    const Bounds &valveLiftBounds = grid.get(m_bounds, 2, 2);
    renderScope(m_intakeValveLiftScope, valveLiftBounds, "Valve Lift");
    renderScope(m_exhaustValveLiftScope, valveLiftBounds, "", true);

    const Bounds &flowBounds = grid.get(m_bounds, 2, 3);
    renderScope(m_intakeFlowScope, flowBounds, "Flow");
    renderScope(m_exhaustFlowScope, flowBounds, "", true);
    renderScope(m_cylinderMoleculesScope, flowBounds, "", true);

    const Bounds &audioWaveformBounds = grid.get(m_bounds, 0, 2);
    renderScope(m_audioWaveformScope, audioWaveformBounds, "Waveform");

    const Bounds &cylinderPressureBounds = grid.get(m_bounds, 1, 3);
    //renderScope(m_cylinderPressureScope, cylinderPressureBounds, "Cylinder Pressure");
    renderScope(m_pvScope, cylinderPressureBounds, "pressure-volume");

    const Bounds &totalExhaustPressureBounds = grid.get(m_bounds, 1, 2);
    renderScope(m_totalExhaustFlowScope, totalExhaustPressureBounds, "Total Exhaust Flow");

    const Bounds &focusBounds = grid.get(m_bounds, 0, 0, 3, 2);
    Bounds focusTitle = focusBounds;
    focusTitle.m0.y = focusTitle.m1.y - (24.0f + 15.0f);
    Bounds focusBody = focusBounds;
    focusBody.m1 = focusBody.m1 - Point(0.0f, 24.0f + 15.0f);

    drawFrame(focusTitle, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());
    drawFrame(focusBody, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    for (int i = 0; i < MaxLayeredScopes; ++i) {
        if (m_currentFocusScopes[i] != nullptr) {
            m_currentFocusScopes[i]->render(focusBody);
        }
        else break;
    }

    UiElement::render();
}

void OscilloscopeCluster::sample() {
    const double cylinderPressure = m_simulator->getEngine()->getChamber(0)->m_system.pressure()
        + m_simulator->getEngine()->getChamber(0)->m_system.dynamicPressure(-1.0, 0.0);

    if (m_simulator->getCurrentIteration() % 2 == 0) {
        double cycleAngle = m_simulator->getEngine()->getCrankshaft(0)->getCycleAngle();
        if (!m_simulator->getEngine()->isSpinningCw()) {
            cycleAngle = 4 * constants::pi - cycleAngle;
        }

        getTotalExhaustFlowOscilloscope()->addDataPoint(
            cycleAngle,
            m_simulator->getTotalExhaustFlow() / m_simulator->getTimestep());
        getCylinderPressureScope()->addDataPoint(
            m_simulator->getEngine()->getCrankshaft(0)->getCycleAngle(constants::pi),
            std::sqrt(cylinderPressure));
        getExhaustFlowOscilloscope()->addDataPoint(
            cycleAngle,
            m_simulator->getEngine()->getChamber(0)->getLastTimestepExhaustFlow() / m_simulator->getTimestep());
        getIntakeFlowOscilloscope()->addDataPoint(
            cycleAngle,
            m_simulator->getEngine()->getChamber(0)->getLastTimestepIntakeFlow() / m_simulator->getTimestep());
        getCylinderMoleculesScope()->addDataPoint(
            cycleAngle,
            m_simulator->getEngine()->getChamber(0)->m_system.n());
        getExhaustValveLiftOscilloscope()->addDataPoint(
            cycleAngle,
            m_simulator->getEngine()->getChamber(0)->getCylinderHead()->exhaustValveLift(
                m_simulator->getEngine()->getChamber(0)->getPiston()->getCylinderIndex()));
        getIntakeValveLiftOscilloscope()->addDataPoint(
            cycleAngle,
            m_simulator->getEngine()->getChamber(0)->getCylinderHead()->intakeValveLift(
                m_simulator->getEngine()->getChamber(0)->getPiston()->getCylinderIndex()));
        getPvScope()->addDataPoint(
            m_simulator->getEngine()->getChamber(0)->getVolume(),
            std::sqrt(m_simulator->getEngine()->getChamber(0)->m_system.pressure()));
    }

    m_exhaustFlowScope->m_yMin = m_intakeFlowScope->m_yMin =
        std::fmin(m_intakeFlowScope->m_yMin, m_exhaustFlowScope->m_yMin);
    m_exhaustFlowScope->m_yMax = m_intakeFlowScope->m_yMax =
        std::fmax(m_intakeFlowScope->m_yMax, m_exhaustFlowScope->m_yMax);

    m_torqueScope->m_yMin = m_hpScope->m_yMin =
        std::fmin(m_torqueScope->m_yMin, m_hpScope->m_yMin);
    m_torqueScope->m_yMax = m_hpScope->m_yMax =
        std::fmax(m_torqueScope->m_yMax, m_hpScope->m_yMax);
}

void OscilloscopeCluster::setSimulator(Simulator *simulator) {
    m_simulator = simulator;
    Engine *engine = m_simulator->getEngine();

    m_hpScope->m_xMax = m_torqueScope->m_xMax =
        units::toRpm(engine->getRedline());
}

void OscilloscopeCluster::renderScope(
    Oscilloscope *osc,
    const Bounds &bounds,
    const std::string &title,
    bool overlay)
{
    if (!overlay) {
        drawFrame(bounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());
    }

    if (osc == m_currentFocusScopes[0]) {
        Grid grid;
        grid.h_cells = 3;
        grid.v_cells = 4;

        const Bounds &focusBounds = grid.get(m_bounds, 0, 0, 3, 2);
        Bounds focusTitle = focusBounds;
        focusTitle.m1 -= Point(0.0f, 24.0f + 15.0f);
        Bounds focusBody = focusBounds;
        focusTitle.m1 += Point(0.0f, 24.0f + 15.0f);

        drawText(title, focusTitle.inset(20.0f), 24.0f, Bounds::tl);
    }

    osc->m_bounds = bounds;
}

double OscilloscopeCluster::getPower(double torque)
{
    double power = 0;
    if (m_powerUnits == "HP")
    {
        if (m_torqueUnits == "NM")
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