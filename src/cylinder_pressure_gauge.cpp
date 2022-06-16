#include "../include/cylinder_pressure_gauge.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"

#include <sstream>

CylinderPressureGauge::CylinderPressureGauge() {
    m_engine = nullptr;
}

CylinderPressureGauge::~CylinderPressureGauge() {
    /* void */
}

void CylinderPressureGauge::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);
}

void CylinderPressureGauge::destroy() {
    UiElement::destroy();
}

void CylinderPressureGauge::update(float dt) {
    UiElement::update(dt);
}

void CylinderPressureGauge::render() {
    drawFrame(m_bounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    const Bounds title = m_bounds.verticalSplit(1.0f, 0.9f);
    const Bounds body = m_bounds.verticalSplit(0.0f, 0.9f);

    drawCenteredText("Cyl. Press. [PSI]", title.inset(10.0f), 24.0f);

    const int banks = m_engine->getCylinderBankCount();

    Grid grid;
    grid.h_cells = banks;
    grid.v_cells = 1;

    while (m_gauges.size() < m_engine->getCylinderCount()) {
        m_gauges.push_back(addElement<Gauge>());
    }

    for (int i = 0; i < m_engine->getCylinderCount(); ++i) {
        Piston *piston = m_engine->getPiston(i);
        CombustionChamber *chamber = m_engine->getChamber(i);
        const int bankIndex = piston->getCylinderBank()->getIndex();

        const Bounds &b = grid.get(body, bankIndex, 0);

        Grid bankGrid = { 1, piston->getCylinderBank()->getCylinderCount() };
        const Bounds &b_cyl =
            bankGrid.get(
                b,
                0,
                piston->getCylinderBank()->getCylinderCount() - piston->getCylinderIndex() - 1).inset(5.0f);

        const double value = units::convert(chamber->m_system.pressure(), units::psi);

        std::stringstream ss;
        ss << std::lroundf(value);
        drawCenteredText(ss.str(), b_cyl.verticalSplit(0.0f, 2 / 6.0f), b_cyl.height() / 6);

        m_gauges[i]->m_bounds = b_cyl;
        m_gauges[i]->setLocalPosition({ 0, 0 });
        m_gauges[i]->m_min = 0;
        m_gauges[i]->m_max = 1000;
        m_gauges[i]->m_minorStep = 20;
        m_gauges[i]->m_majorStep = 100;
        m_gauges[i]->m_maxMinorTick = 400;
        m_gauges[i]->m_thetaMin = constants::pi * 1.2;
        m_gauges[i]->m_thetaMax = -0.2 * constants::pi;
        m_gauges[i]->m_outerRadius = std::fmin(b_cyl.width(), b_cyl.height()) / 2.0f;
        m_gauges[i]->m_value = units::convert(chamber->m_system.pressure(), units::psi);
        m_gauges[i]->m_needleOuterRadius = m_gauges[i]->m_outerRadius * 0.7f;
        m_gauges[i]->m_needleInnerRadius = -m_gauges[i]->m_outerRadius * 0.1f;
        m_gauges[i]->m_needleWidth = 2.0;
        m_gauges[i]->m_gamma = 0.5f;
        m_gauges[i]->setBandCount(2);
        m_gauges[i]->setBand({ ysMath::Constants::One, 400, 1000, 3.0f }, 0);
        m_gauges[i]->setBand({ ysColor::srgbiToLinear(0x77CEE0), 0.0f, 14.6959f, 3.0f, 0.0f }, 1);
    }

    UiElement::render();
}
