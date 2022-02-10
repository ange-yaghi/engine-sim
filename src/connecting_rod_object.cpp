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

void ConnectingRodObject::render() {
    resetShader();
    setTransform(&m_connectingRod->m_body);

    m_app->getShaders()->SetBaseColor(ysMath::Constants::One);
    m_app->drawGenerated(m_connectingRodBody);
}

void ConnectingRodObject::process(float dt) {
    /* void */
}

void ConnectingRodObject::destroy() {
    /* void */
}
