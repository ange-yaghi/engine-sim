#include "../include/connecting_rod_object.h"

#include "../include/engine_sim_application.h"
#include "../include/units.h"

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
    params.y0 = m_connectingRod->getBigEndLocal() + m_connectingRod->m_crankshaft->m_throw * 0.6f;
    params.y1 = m_connectingRod->getLittleEndLocal();
    params.lineWidth = m_connectingRod->m_crankshaft->m_throw * 0.5f;

    gen->startShape();
    gen->generateLine2d(params);
    gen->endShape(&m_connectingRodBody);
}

void ConnectingRodObject::render(const ViewParameters *view) {
    const int layer = m_connectingRod->m_journal;
    if (layer > view->Layer1 || layer < view->Layer0) return;

    ysVector col = m_connectingRod->m_piston->m_bank->m_index % 2 == 0
        ? ysColor::srgbiToLinear(0xEEEEEE)
        : ysColor::srgbiToLinear(0xDDDDDD);
    col = tintByLayer(col, layer);

    resetShader();
    setTransform(
        &m_connectingRod->m_body,
        m_connectingRod->m_crankshaft->m_throw,
        0.0f,
        m_connectingRod->getBigEndLocal());

    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("ConnectingRod"),
        0x10 - layer);

    setTransform(&m_connectingRod->m_body);
    m_app->drawGenerated(m_connectingRodBody, 0x10 - layer);
}

void ConnectingRodObject::process(float dt) {
    /* void */
}

void ConnectingRodObject::destroy() {
    /* void */
}
