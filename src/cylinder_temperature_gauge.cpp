#include "../include/cylinder_temperature_gauge.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

#include <sstream>

#undef min

CylinderTemperatureGauge::CylinderTemperatureGauge() {
    m_engine = nullptr;
    m_maxTemperature = 2000.0;
    m_minTemperature = 200.0;
}

CylinderTemperatureGauge::~CylinderTemperatureGauge() {
    /* void */
}

void CylinderTemperatureGauge::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);
}

void CylinderTemperatureGauge::destroy() {
    UiElement::destroy();
}

void CylinderTemperatureGauge::update(float dt) {
    UiElement::update(dt);

    const double decay = dt / (0.0001 + dt);

    double maxTemperature = m_maxTemperature;
    double minTemperature = m_minTemperature;

    for (int i = 0; i < m_engine->getCylinderCount(); ++i) {
        Piston *piston = m_engine->getPiston(i);
        CombustionChamber *chamber = m_engine->getChamber(i);

        const double temperature = chamber->m_system.temperature();
        double value = temperature - m_minTemperature;

        m_maxTemperature = std::fmax(m_maxTemperature, value);
        m_minTemperature = std::fmin(m_minTemperature, temperature);
    }

    m_maxTemperature *= decay;
    m_minTemperature *= decay;
}

void CylinderTemperatureGauge::render() {
    drawFrame(m_bounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    const Bounds title = m_bounds.verticalSplit(1.0f, 0.9f);
    const Bounds body = m_bounds.verticalSplit(0.0f, 0.9f);

    drawCenteredText("Cyl. Temp.", title.inset(10.0f), 24.0f);

    const int banks = m_engine->getCylinderBankCount();

    Grid grid;
    grid.h_cells = banks;
    grid.v_cells = 1;

    GeometryGenerator *generator = m_app->getGeometryGenerator();

    const ysVector background = m_app->getBackgroundColor();
    const ysVector hot = mix(background, m_app->getRed(), 0.1f);
    const ysVector cold = mix(background, m_app->getBlue(), 0.001f);

    for (int i = 0; i < m_engine->getCylinderCount(); ++i) {
        Piston *piston = m_engine->getPiston(i);
        CombustionChamber *chamber = m_engine->getChamber(i);
        const int bankIndex = piston->m_bank->m_index;

        const Bounds &b = grid.get(body, bankIndex, 0);

        Grid bankGrid = { 1, piston->m_bank->m_cylinderCount };
        const Bounds &b_cyl =
            bankGrid.get(
                b,
                0,
                piston->m_bank->m_cylinderCount - piston->m_cylinderIndex - 1).inset(5.0f);

        const double temperature = chamber->m_system.temperature();
        double value = temperature - m_minTemperature;

        const Bounds worldBounds = getRenderBounds(b_cyl);
        const Point position = worldBounds.getPosition(Bounds::center);

        GeometryGenerator::Circle2dParameters params;
        params.center_x = position.x;
        params.center_y = position.y;
        params.radius = (worldBounds.height() / 2) * 0.9f;
        params.maxEdgeLength = pixelsToUnits(5.0f);

        GeometryGenerator::GeometryIndices indices;
        generator->startShape();
        generator->generateCircle2d(params);
        generator->endShape(&indices);

        m_app->getShaders()->SetBaseColor(mix(cold, hot, value / m_maxTemperature));
        m_app->drawGenerated(indices, 0x11, m_app->getShaders()->GetUiFlags());
    }

    UiElement::render();
}
