#include "../include/oscilloscope_cluster.h"

#include "../include/engine_sim_application.h"

#include <sstream>

OscilloscopeCluster::OscilloscopeCluster() {
    m_simulator = nullptr;
    m_torqueScope = nullptr;
    m_hpScope = nullptr;
    m_exhaustFlowScope = nullptr;
    m_audioWaveformScope = nullptr;
    m_currentFocusScope0 = nullptr;
    m_currentFocusScope1 = nullptr;

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
    m_audioWaveformScope = addElement<Oscilloscope>(this);

    // Torque
    m_torqueScope->setBufferSize(100);
    m_torqueScope->m_xMin = 0.0f;
    m_torqueScope->m_xMax = 9000.0f;
    m_torqueScope->m_yMin = -1000.0;
    m_torqueScope->m_yMax = 1000.0;
    m_torqueScope->m_lineWidth = 1.0f;
    m_torqueScope->m_drawReverse = true;
    m_torqueScope->i_color = m_app->getOrange();

    // Horsepower
    m_hpScope->setBufferSize(100);
    m_hpScope->m_xMin = 0.0f;
    m_hpScope->m_xMax = 9000.0f;
    m_hpScope->m_yMin = -1000.0f;
    m_hpScope->m_yMax = 1000.0f;
    m_hpScope->m_lineWidth = 1.0f;
    m_hpScope->m_drawReverse = true;
    m_hpScope->i_color = m_app->getPink();

    // Exhaust flow
    m_exhaustFlowScope->setBufferSize(2048);
    m_exhaustFlowScope->m_xMin = 0.0f;
    m_exhaustFlowScope->m_xMax = constants::pi * 4;
    m_exhaustFlowScope->m_yMin = 0.0; -units::flow(400.0, units::scfm);
    m_exhaustFlowScope->m_yMax = units::pressure(100, units::psi); units::flow(1000.0, units::scfm);
    m_exhaustFlowScope->m_lineWidth = 1.0f;
    m_exhaustFlowScope->m_drawReverse = false;
    m_exhaustFlowScope->i_color = m_app->getOrange();

    // Audio waveform scope
    m_audioWaveformScope->setBufferSize(44100 / 50);
    m_audioWaveformScope->m_xMin = 0.0f;
    m_audioWaveformScope->m_xMax = 44100 / 10;
    m_audioWaveformScope->m_yMin = -1.5f;
    m_audioWaveformScope->m_yMax = 1.5f;
    m_audioWaveformScope->m_lineWidth = 1.0f;
    m_audioWaveformScope->m_drawReverse = false;
    m_audioWaveformScope->i_color = m_app->getBlue();

    m_currentFocusScope0 = m_exhaustFlowScope;
    m_currentFocusScope1 = nullptr;
}

void OscilloscopeCluster::destroy() {
    UiElement::destroy();
}

void OscilloscopeCluster::signal(UiElement *element, Event event) {
    if (event == Event::Clicked) {
        if (element == m_audioWaveformScope) {
            m_currentFocusScope0 = m_audioWaveformScope;
            m_currentFocusScope1 = nullptr;
        }
        else if (element == m_hpScope || element == m_torqueScope) {
            m_currentFocusScope0 = m_hpScope;
            m_currentFocusScope1 = m_torqueScope;
        }
        else if (element == m_exhaustFlowScope) {
            m_currentFocusScope0 = m_exhaustFlowScope;
            m_currentFocusScope1 = nullptr;
        }
    }
}

void OscilloscopeCluster::update(float dt) {
    Engine *engine = m_simulator->getEngine();
    const double torque =
        units::convert(m_simulator->getCrankshaftLoad(0)->getDynoTorque(), units::ft_lb);
    const double hp = torque * engine->getRpm() / 5252.0;

    m_torque = m_torque * 0.99 + 0.01 * torque;
    m_hp = m_hp * 0.99 + 0.01 * hp;

    if (m_updateTimer <= 0) {
        m_updateTimer = m_updatePeriod;
        
        m_torqueScope->addDataPoint(engine->getRpm(), m_torque);
        m_hpScope->addDataPoint(engine->getRpm(), m_hp);
    }

    m_updateTimer -= dt;

    UiElement::update(dt);
}

void OscilloscopeCluster::render() {
    Grid grid;
    grid.h_cells = 3;
    grid.v_cells = 4;

    const Bounds &hpTorqueBounds = grid.get(m_bounds, 0, 3);
    renderScope(m_torqueScope, hpTorqueBounds, "Torque");
    renderScope(m_hpScope, hpTorqueBounds, "", true);
    
    const Bounds &exhaustFlowBounds = grid.get(m_bounds, 1, 3);
    renderScope(m_exhaustFlowScope, exhaustFlowBounds, "Exhaust Flow");

    const Bounds &audioWaveformBounds = grid.get(m_bounds, 2, 3);
    renderScope(m_audioWaveformScope, audioWaveformBounds, "Waveform");

    const Bounds &focusBounds = grid.get(m_bounds, 0, 0, 3, 3);
    drawFrame(focusBounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    if (m_currentFocusScope0 != nullptr) {
        m_currentFocusScope0->render(focusBounds);
    }
    
    if (m_currentFocusScope1 != nullptr) {
        m_currentFocusScope1->render(focusBounds);
    }

    UiElement::render();
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

    osc->m_bounds = bounds;
}
