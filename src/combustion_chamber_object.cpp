#include "../include/combustion_chamber_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"
#include "../include/constants.h"

CombustionChamberObject::CombustionChamberObject() {
    m_chamber = nullptr;
}

CombustionChamberObject::~CombustionChamberObject() {
    /* void */
}

void CombustionChamberObject::generateGeometry() {
    if (m_chamber->m_lit) {
        int a = 0;
    }

    GeometryGenerator *gen = m_app->getGeometryGenerator();
    CylinderBank *bank = m_chamber->m_bank;
    CylinderHead *head = m_chamber->m_head;

    const float lineWidth = (float)bank->m_bore;
    double flameTop_x, flameTop_y;
    double flameBottom_x, flameBottom_y;
    double chamberHeight = head->m_combustionChamberVolume / bank->boreSurfaceArea();

    bank->getPositionAboveDeck(chamberHeight, &flameTop_x, &flameTop_y);
    bank->getPositionAboveDeck(chamberHeight - m_chamber->m_flameEvent.travel, &flameBottom_x, &flameBottom_y);

    GeometryGenerator::Line2dParameters params;
    params.lineWidth = lineWidth;

    gen->startShape();

    params.x0 = (float)flameTop_x;
    params.y0 = (float)flameTop_y;
    params.x1 = (float)flameBottom_x;
    params.y1 = (float)flameBottom_y;
    gen->generateLine2d(params);

    gen->endShape(&m_indices);
}

void CombustionChamberObject::render(const ViewParameters *view) {
    resetShader();

    if (m_chamber->m_piston->m_cylinderIndex == view->Layer0) {
        if (m_chamber->m_lit) {
            m_app->getShaders()->SetBaseColor(ysMath::Mul(m_app->getOrange(), ysMath::LoadVector(1.0f, 1.0f, 1.0f, 0.5f)));
            m_app->drawGenerated(m_indices, 0x10);
        }
    }
}

void CombustionChamberObject::process(float dt) {
    /* void */
}

void CombustionChamberObject::destroy() {
    /* void */
}
