#include "../include/firing_order_display.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

#include <sstream>

#undef min

FiringOrderDisplay::FiringOrderDisplay() {
    m_engine = nullptr;
    m_cylinderCount = 0;
    m_cylinderLit = nullptr;
}

FiringOrderDisplay::~FiringOrderDisplay() {
    /* void */
}

void FiringOrderDisplay::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);
}

void FiringOrderDisplay::destroy() {
    UiElement::destroy();
}

void FiringOrderDisplay::update(float dt) {
    UiElement::update(dt);

    if (m_engine->getCylinderCount() != m_cylinderCount) {
        if (m_cylinderLit != nullptr) {
            delete[] m_cylinderLit;
        }

        m_cylinderCount = m_engine->getCylinderCount();
        m_cylinderLit = new float[m_cylinderCount];
        memset(m_cylinderLit, 0, sizeof(float) * m_cylinderCount);
    }

    for (int i = 0; i < m_cylinderCount; ++i) {
        if (m_engine->getChamber(i)->popLitLastFrame() || m_engine->getChamber(i)->isLit()) {
            m_cylinderLit[i] = 0.05f + 0.95f * m_cylinderLit[i];
        }
        else {
            m_cylinderLit[i] *= (dt / (dt + 0.01f));
        }
    }
}

void FiringOrderDisplay::render() {
    drawFrame(m_bounds, 1.0, ysMath::Constants::One, m_app->getBackgroundColor());

    const Bounds title = m_bounds.verticalSplit(1.0f, 0.9f);
    const Bounds body = m_bounds.verticalSplit(0.0f, 0.9f);

    drawCenteredText("Ignition", title.inset(20.0f), 24.0f);

    const int banks = m_engine->getCylinderBankCount();

    Grid grid;
    grid.h_cells = banks;
    grid.v_cells = 1;

    GeometryGenerator *generator = m_app->getGeometryGenerator();

    const ysVector background = m_app->getBackgroundColor();
    const ysVector hot = mix(background, m_app->getWhite(), 1.0f);
    const ysVector fixed = mix(background, m_app->getWhite(), 0.01f);
    const ysVector cold = mix(background, m_app->getWhite(), 0.001f);

    std::vector<CylinderBank *> orderedBanks;
    std::map<CylinderBank *, int> bankToIndex;
    for (int i = 0; i < m_engine->getCylinderBankCount(); ++i) {
        orderedBanks.push_back(m_engine->getCylinderBank(i));
    }

    std::sort(
        orderedBanks.begin(),
        orderedBanks.end(),
        [](CylinderBank *a, CylinderBank *b) {
            return a->getAngle() < b->getAngle();
        });
    for (int i = 0; i < m_engine->getCylinderBankCount(); ++i) {
        bankToIndex[orderedBanks[i]] = i;
    }

    for (int i = 0; i < m_engine->getCylinderCount(); ++i) {
        Piston *piston = m_engine->getPiston(i);
        CombustionChamber *chamber = m_engine->getChamber(i);
        CylinderBank *bank = piston->getCylinderBank();
        const int bankIndex = bankToIndex[bank];
        const double lit = m_cylinderLit[i];

        const Bounds &b = grid.get(body, banks - bankIndex - 1, 0);

        Grid bankGrid = { 1, bank->getCylinderCount() };
        const Bounds &b_cyl =
            bankGrid.get(
                b,
                0,
                bank->getCylinderCount() - piston->getCylinderIndex() - 1).inset(5.0f);

        const double temperature = chamber->m_system.temperature();

        const Bounds worldBounds = getRenderBounds(b_cyl);
        const Point position = worldBounds.getPosition(Bounds::center);

        const float radius = std::min(worldBounds.height() / 2, worldBounds.width() / 2);

        GeometryGenerator::Circle2dParameters params;
        params.center_x = position.x;
        params.center_y = position.y;
        params.radius = radius * 0.75f;
        params.maxEdgeLength = pixelsToUnits(5.0f);

        GeometryGenerator::Ring2dParameters ringParams;
        ringParams.center_x = position.x;
        ringParams.center_y = position.y;
        ringParams.innerRadius = radius * 0.8f;
        ringParams.outerRadius = radius * 0.85f;
        ringParams.maxEdgeLength = pixelsToUnits(5.0f);

        GeometryGenerator::GeometryIndices ring, light;
        generator->startShape();
        generator->generateRing2d(ringParams);
        generator->endShape(&ring);

        generator->startShape();
        generator->generateCircle2d(params);
        generator->endShape(&light);

        m_app->getShaders()->SetBaseColor(fixed);
        m_app->drawGenerated(ring, 0x11, m_app->getShaders()->GetUiFlags());

        m_app->getShaders()->SetBaseColor(mix(cold, hot, (float)lit));
        m_app->drawGenerated(light, 0x11, m_app->getShaders()->GetUiFlags());
    }

    UiElement::render();
}
