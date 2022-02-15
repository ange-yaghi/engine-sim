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

    GeometryGenerator::Circle2dParameters circleParams;
    circleParams.center_x = 0.0f;
    circleParams.center_y = m_piston->m_wristPinLocation;
    circleParams.maxEdgeLength = m_app->pixelsToUnits(5.0f);
    circleParams.radius = (m_piston->m_bank->m_bore / 10) * 0.75f;
    gen->startShape();
    gen->generateCircle2d(circleParams);
    gen->endShape(&m_wristPinHole);
}

void PistonObject::render(const ViewParameters *view) {
    const int layer = m_piston->m_rod->m_journal;
    if (layer > view->Layer1 || layer < view->Layer0) return;

    ysVector col = ysColor::srgbiToLinear(0x77CEE0);
    ysVector holeCol = m_app->getBackgroundColor();
    for (int i = view->Layer0; i < layer; ++i) {
        col = ysMath::Add(
            ysMath::Mul(col, ysMath::LoadScalar(0.2f)),
            ysMath::Mul(ysColor::srgbiToLinear(0x0E1012), ysMath::LoadScalar(0.8f))
        );

        holeCol = ysMath::Add(
            ysMath::Mul(holeCol, ysMath::LoadScalar(0.2f)),
            ysMath::Mul(ysColor::srgbiToLinear(0x0E1012), ysMath::LoadScalar(0.8f))
        );
    }

    resetShader();
    setTransform(
        &m_piston->m_body,
        m_piston->m_bank->m_bore / 2,
        0.0f,
        -m_piston->m_compressionHeight - m_piston->m_wristPinLocation);

    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Piston"),
        0x10 - layer);

    setTransform(&m_piston->m_body);
    m_app->getShaders()->SetBaseColor(holeCol);
    m_app->drawGenerated(m_wristPinHole, 0x10 - layer);
}

void PistonObject::process(float dt) {
    /* void */
}

void PistonObject::destroy() {
    /* void */
}
