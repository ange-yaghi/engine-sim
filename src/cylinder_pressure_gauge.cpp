#include "../include/cylinder_pressure_gauge.h"

#include "../include/units.h"

#include <sstream>

CylinderPressureGauge::CylinderPressureGauge() {
    m_simulator = nullptr;
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
    /* void */
}

void CylinderPressureGauge::render() {
    drawFrame(m_bounds, 1.0, ysMath::Constants::One, ysMath::Constants::Zero3);

    const Bounds title = m_bounds.verticalSplit(1.0f, 0.9f);
    const Bounds body = m_bounds.verticalSplit(0.0f, 0.9f);

    drawCenteredText("Cyl. Press. (PSI)", title.inset(10.0f), 24.0f);

    Engine *engine = m_simulator->getEngine();
    const int banks = engine->getCylinderBankCount();

    Grid grid;
    grid.h_cells = banks;
    grid.v_cells = 1;

    for (int i = 0; i < engine->getCylinderCount(); ++i) {
        Piston *piston = engine->getPiston(i);
        CombustionChamber *chamber = m_simulator->getCombustionChamber(i);
        const int bankIndex = piston->m_bank->m_index;

        const Bounds &b = grid.get(body, bankIndex, 0);

        Grid bankGrid = { 1, piston->m_bank->m_cylinderCount };
        const Bounds &b_cyl =
            bankGrid.get(
                b,
                0,
                piston->m_bank->m_cylinderCount - piston->m_cylinderIndex - 1).inset(5.0f);

        std::stringstream ss;
        ss << std::lroundf(units::convert(chamber->getPressure(), units::psi));
        //ss << std::lroundf(units::convert(chamber->volume(), units::cc));
        drawCenteredText(ss.str(), b_cyl, b_cyl.height() / 4);
    }
}
