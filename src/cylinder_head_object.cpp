#include "../include/cylinder_head_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"
#include "../include/constants.h"

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
    const double boreSurfaceArea =
        Constants::pi * m_head->m_bank->m_bore * m_head->m_bank->m_bore / 4.0;
    const double chamberHeight = m_head->m_combustionChamberVolume / boreSurfaceArea;

    const double theta = m_head->m_bank->m_angle;
    double x, y;
    m_head->m_bank->getPositionAboveDeck(chamberHeight, &x, &y);

    const ysMatrix scale = ysMath::ScaleTransform(ysMath::LoadScalar(s));
    const ysMatrix rotation = ysMath::RotationTransform(
            ysMath::Constants::ZAxis, theta);
    const ysMatrix translation = ysMath::TranslationTransform(
            ysMath::LoadVector(x, y));
    const ysMatrix T_head = ysMath::MatMult(
            ysMath::MatMult(translation, rotation),
            scale);

    const ysVector col = m_app->getPink();  ysMath::Add(
        ysMath::Mul(ysMath::Constants::One, ysMath::LoadScalar(0.01f)),
        ysMath::Mul(m_app->getBackgroundColor(), ysMath::LoadScalar(0.99f))
    );
    const ysVector moving = m_app->getWhite();

    m_app->getShaders()->SetObjectTransform(T_head);
    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("CylinderHead"),
        0x10);

    const double intakeValvePosition = (m_head->m_flipDisplay)
        ? 0.5f
        : -0.5f;

    const int layer = view->Layer0;
    const float intakeLift = (float)m_head->intakeValveLift(layer);
    const ysMatrix T_intakeValve = ysMath::MatMult(
            T_head,
            ysMath::TranslationTransform(ysMath::LoadVector(intakeValvePosition, -intakeLift / s, 0.0f, 0.0f)));

    m_app->getShaders()->SetObjectTransform(T_intakeValve);
    m_app->getShaders()->SetBaseColor(m_app->getBlue());
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Valve"),
        0x10);

    const float exhaustLift = (float)m_head->exhaustValveLift(layer);
    const ysMatrix T_exhaustValve = ysMath::MatMult(
        T_head,
        ysMath::TranslationTransform(ysMath::LoadVector(-intakeValvePosition, -exhaustLift / s, 0.0f, 0.0f)));

    m_app->getShaders()->SetObjectTransform(T_exhaustValve);
    m_app->getShaders()->SetBaseColor(m_app->getYellow());
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
