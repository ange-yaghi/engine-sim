#include "../include/cylinder_head_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"

CylinderHeadObject::CylinderHeadObject() {
    m_head = nullptr;
}

CylinderHeadObject::~CylinderHeadObject() {
    /* void */
}

void CylinderHeadObject::generateGeometry() {
    /* void */
}

void CylinderHeadObject::render(const ViewParameters *view) {
    resetShader();

    const double s = m_head->m_bank->m_bore / 2.0;

    const double theta = m_head->m_bank->m_angle;
    double x, y;
    m_head->m_bank->getTop(&x, &y);

    const ysMatrix scale = ysMath::ScaleTransform(ysMath::LoadScalar(s));
    const ysMatrix rotation = ysMath::RotationTransform(
            ysMath::Constants::ZAxis, theta);
    const ysMatrix translation = ysMath::TranslationTransform(
            ysMath::LoadVector(x, y));
    const ysMatrix T_head = ysMath::MatMult(
            ysMath::MatMult(translation, rotation),
            scale);

    const ysVector col = ysMath::Add(
        ysMath::Mul(ysMath::Constants::One, ysMath::LoadScalar(0.01f)),
        ysMath::Mul(m_app->getBackgroundColor(), ysMath::LoadScalar(0.99f))
    );

    m_app->getShaders()->SetObjectTransform(T_head);
    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("CylinderHead"),
        0x10);

    const int layer = view->Layer0;
    const float intakeLift = (float)m_head->intakeValveLift(layer);
    const ysMatrix T_intakeValve = ysMath::MatMult(
            T_head,
            ysMath::TranslationTransform(ysMath::LoadVector(-0.5f, -intakeLift / s, 0.0f, 0.0f)));

    m_app->getShaders()->SetObjectTransform(T_intakeValve);
    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Valve"),
        0x10);

    const float exhaustLift = (float)m_head->exhaustValveLift(layer);
    const ysMatrix T_exhaustValve = ysMath::MatMult(
        T_head,
        ysMath::TranslationTransform(ysMath::LoadVector(0.5f, -exhaustLift / s, 0.0f, 0.0f)));

    m_app->getShaders()->SetObjectTransform(T_exhaustValve);
    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Valve"),
        0x10);
}

void CylinderHeadObject::process(float dt) {
    /* void */
}

void CylinderHeadObject::destroy() {
    /* void */
}
