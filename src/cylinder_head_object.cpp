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

    CylinderBank *bank = m_head->getCylinderBank();
    const double s = bank->getBore() / 2.0;
    const double boreSurfaceArea =
        constants::pi * bank->getBore() * bank->getBore() / 4.0;
    const double chamberHeight = m_head->getCombustionChamberVolume() / boreSurfaceArea;

    const double theta = bank->getAngle();
    double x, y;
    bank->getPositionAboveDeck(chamberHeight, &x, &y);

    const ysMatrix scale = ysMath::ScaleTransform(ysMath::LoadScalar(s));
    const ysMatrix rotation = ysMath::RotationTransform(
            ysMath::Constants::ZAxis, theta);
    const ysMatrix translation = ysMath::TranslationTransform(
            ysMath::LoadVector(x, y));
    const ysMatrix T_headObject = ysMath::MatMult(translation, rotation);
    const ysMatrix T_head = ysMath::MatMult(
            T_headObject,
            scale);

    const ysVector col = m_app->getPink();  ysMath::Add(
        ysMath::Mul(ysMath::Constants::One, ysMath::LoadScalar(0.01f)),
        ysMath::Mul(m_app->getBackgroundColor(), ysMath::LoadScalar(0.99f))
    );
    const ysVector moving = m_app->getWhite();

    GeometryGenerator::GeometryIndices
        valveShadow,
        valveRoller,
        valveRollerShadow,
        valveRollerPin,
        camCenter;
    GeometryGenerator *gen = m_app->getGeometryGenerator();
    GeometryGenerator::Line2dParameters params;

    gen->startShape();
    params.lineWidth = 0.1 + m_app->pixelsToUnits(5.0f) / s;
    params.x0 = -0.689352;
    params.y0 = 0.101737;
    params.x1 = -0.88632;
    params.y1 = -0.077975;
    gen->generateLine2d(params);

    params.x0 = -(params.x0 + 0.5) - 0.5;
    params.x1 = -(params.x1 + 0.5) - 0.5;
    gen->generateLine2d(params);

    params.x0 = 0.689352;
    params.x1 = 0.88632;
    gen->generateLine2d(params);

    params.x0 = -(params.x0 - 0.5) + 0.5;
    params.x1 = -(params.x1 - 0.5) + 0.5;
    gen->generateLine2d(params);

    params.lineWidth = 0.0917 + m_app->pixelsToUnits(5.0f) / s;
    params.x0 = -0.5;
    params.y0 = 0.5;
    params.x1 = -0.5;
    params.y1 = 1.5;
    gen->generateLine2d(params);

    params.x0 = 0.5;
    params.x1 = 0.5;
    gen->generateLine2d(params);

    gen->endShape(&valveShadow);

    const float rollerRadius = units::distance(300.0, units::thou);
    GeometryGenerator::Circle2dParameters circleParams;
    circleParams.radius = rollerRadius / s;
    circleParams.center_x = 0.0f;
    circleParams.center_y = 1.99f;
    gen->startShape();
    gen->generateCircle2d(circleParams);
    gen->endShape(&valveRoller);

    circleParams.radius = (rollerRadius + m_app->pixelsToUnits(5.0f) / 2) / (float)s;
    gen->startShape();
    gen->generateCircle2d(circleParams);
    gen->endShape(&valveRollerShadow);

    circleParams.radius = (rollerRadius * 0.25f) / (float)s;
    gen->startShape();
    gen->generateCircle2d(circleParams);
    gen->endShape(&valveRollerPin);

    circleParams.radius = (rollerRadius * 0.25f);
    circleParams.center_x = 0.0f;
    circleParams.center_y = 0.0f;
    gen->startShape();
    gen->generateCircle2d(circleParams);
    gen->endShape(&camCenter);

    m_app->getShaders()->SetObjectTransform(T_head);
    m_app->getShaders()->SetBaseColor(col);
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("CylinderHead"),
        0x10);
    m_app->getShaders()->SetObjectTransform(T_head);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(valveShadow, 0x11);

    const double intakeValvePosition = (m_head->getFlipDisplay())
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
        0x11);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(valveRollerShadow, 0x11);
    m_app->getShaders()->SetBaseColor(m_app->getBlue());
    m_app->drawGenerated(valveRoller, 0x11);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(valveRollerPin, 0x11);

    const float exhaustLift = (float)m_head->exhaustValveLift(layer);
    const ysMatrix T_exhaustValve = ysMath::MatMult(
        T_head,
        ysMath::TranslationTransform(ysMath::LoadVector(-intakeValvePosition, -exhaustLift / s, 0.0f, 0.0f)));

    m_app->getShaders()->SetObjectTransform(T_exhaustValve);
    m_app->getShaders()->SetBaseColor(m_app->getYellow());
    m_app->getEngine()->DrawModel(
        m_app->getShaders()->GetRegularFlags(),
        m_app->getAssetManager()->GetModelAsset("Valve"),
        0x11);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(valveRollerShadow, 0x11);
    m_app->getShaders()->SetBaseColor(m_app->getYellow());
    m_app->drawGenerated(valveRoller, 0x11);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(valveRollerPin, 0x11);

    Camshaft *intakeCam = m_head->getIntakeCamshaft();
    Camshaft *exhaustCam = m_head->getExhaustCamshaft();
    GeometryGenerator::GeometryIndices intake, intakeShadow, exhaust;
    generateCamshaft(intakeCam, 0.0, rollerRadius, &intake);
    generateCamshaft(intakeCam, m_app->pixelsToUnits(5.0) / 2, rollerRadius, &intakeShadow);
    generateCamshaft(exhaustCam, 0.0, rollerRadius, &exhaust);

    ysMatrix T_exhaustCam = ysMath::MatMult(
        T_headObject,
        ysMath::TranslationTransform(ysMath::LoadVector(
            -intakeValvePosition * s,
            1.99 * s + m_app->pixelsToUnits(5.0) / 2 + exhaustCam->getBaseRadius() + rollerRadius,
            0.0f,
            0.0f)));
    T_exhaustCam = ysMath::MatMult(
        T_exhaustCam,
        ysMath::RotationTransform(
            ysMath::Constants::ZAxis, -exhaustCam->getAngle() + exhaustCam->getLobeCenterline(layer) - exhaustCam->getAdvance() / 2));

    m_app->getShaders()->SetObjectTransform(T_exhaustCam);
    m_app->getShaders()->SetBaseColor(m_app->getYellow());
    m_app->drawGenerated(exhaust);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(camCenter);

    ysMatrix T_intakeCam = ysMath::MatMult(
        T_headObject,
        ysMath::TranslationTransform(ysMath::LoadVector(
            intakeValvePosition * s,
            1.99 * s + m_app->pixelsToUnits(5.0) / 2 + intakeCam->getBaseRadius() + rollerRadius,
            0.0f,
            0.0f)));
    T_intakeCam = ysMath::MatMult(
        T_intakeCam,
        ysMath::RotationTransform(
            ysMath::Constants::ZAxis, -intakeCam->getAngle() + intakeCam->getLobeCenterline(layer) - intakeCam->getAdvance() / 2));

    m_app->getShaders()->SetObjectTransform(T_intakeCam);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(intakeShadow);

    m_app->getShaders()->SetObjectTransform(T_intakeCam);
    m_app->getShaders()->SetBaseColor(m_app->getBlue());
    m_app->drawGenerated(intake);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(camCenter);
}

void CylinderHeadObject::process(float dt) {
    /* void */
}

void CylinderHeadObject::destroy() {
    /* void */
}

void CylinderHeadObject::generateCamshaft(
    Camshaft *camshaft,
    double padding,
    double rollerRadius,
    GeometryGenerator::GeometryIndices *indices)
{
    GeometryGenerator::Cam2dParameters params;
    params.baseRadius = (float)(camshaft->getBaseRadius() + padding);
    params.center_x = 0.0;
    params.center_y = 0.0;
    params.rollerRadius = (float)rollerRadius;
    params.lift = camshaft->getLobeProfile();
    params.maxEdgeLength = units::distance(50, units::thou);

    m_app->getGeometryGenerator()->startShape();
    m_app->getGeometryGenerator()->generateCam(params);
    m_app->getGeometryGenerator()->endShape(indices);
}
