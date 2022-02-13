#include "../include/connecting_rod_object.h"

#include "../include/engine_sim_application.h"

ConnectingRodObject::ConnectingRodObject() {
    m_connectingRod = nullptr;
}

ConnectingRodObject::~ConnectingRodObject() {
    /* void */
}

void ConnectingRodObject::generateGeometry() {
    GeometryGenerator *gen = m_app->getGeometryGenerator();

    GeometryGenerator::Line2dParameters params;
    params.x0 = params.x1 = 0;
    params.y0 = m_connectingRod->getBigEndLocal();
    params.y1 = m_connectingRod->getLittleEndLocal();
    params.lineWidth = 0.5;

    gen->startShape();
    gen->generateLine2d(params);
    gen->endShape(&m_connectingRodBody);
}

void ConnectingRodObject::render(const ViewParameters *view) {
    const int layer = m_connectingRod->m_journal;
    if (layer > view->Layer1 || layer < view->Layer0) return;

    ysVector col = ysMath::Constants::One;
    for (int i = view->Layer0; i < layer; ++i) {
        col = ysMath::Add(
            ysMath::Mul(col, ysMath::LoadScalar(0.2f)),
            ysMath::Mul(ysColor::srgbiToLinear(0x0E1012), ysMath::LoadScalar(0.8f))
        );
    }

    resetShader();
    setTransform(&m_connectingRod->m_body);

    m_app->getShaders()->SetBaseColor(col);
    m_app->drawGenerated(m_connectingRodBody, 0xFF - layer);
}

void ConnectingRodObject::process(float dt) {
    /* void */
}

void ConnectingRodObject::destroy() {
    /* void */
}
