#include "../include/piston_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"

PistonObject::PistonObject() {
    m_piston = nullptr;
}

PistonObject::~PistonObject() {
    /* void */
}

void PistonObject::generateGeometry() {
    GeometryGenerator *gen = m_app->getGeometryGenerator();

    GeometryGenerator::Line2dParameters params;
    params.x0 = -(float)m_piston->m_bank->m_bore / 2;
    params.x1 = (float)m_piston->m_bank->m_bore / 2;
    params.y0 = params.y1 = 0.0f;
    params.lineWidth = m_piston->m_compressionHeight;

    gen->startShape();
    gen->generateLine2d(params);
    gen->endShape(&m_pistonBody);
}

void PistonObject::render(const ViewParameters *view) {
    const int layer = m_piston->m_rod->m_journal;
    if (layer > view->Layer1 || layer < view->Layer0) return;

    ysVector col = ysMath::Constants::One;
    for (int i = view->Layer0; i < layer; ++i) {
        col = ysMath::Add(
            ysMath::Mul(col, ysMath::LoadScalar(0.2f)),
            ysMath::Mul(ysColor::srgbiToLinear(0x0E1012), ysMath::LoadScalar(0.8f))
        );
    }

    resetShader();
    setTransform(&m_piston->m_body);

    m_app->getShaders()->SetBaseColor(col);
    m_app->drawGenerated(m_pistonBody, 0xFF - layer);
}

void PistonObject::process(float dt) {
    /* void */
}

void PistonObject::destroy() {
    /* void */
}
