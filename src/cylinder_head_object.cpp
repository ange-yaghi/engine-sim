#include "../include/cylinder_head_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"
#include "../include/constants.h"

CylinderHeadObject::CylinderHeadObject() {
    m_head = nullptr;
    m_engine = nullptr;
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
    const double s = (float)bank->getBore() / 2.0f;
    const double boreSurfaceArea =
        constants::pi * bank->getBore() * bank->getBore() / 4.0;
    const double chamberHeight = m_head->getCombustionChamberVolume() / boreSurfaceArea;

    Piston *frontmostPiston = getForemostPiston(bank, view->Layer0);
    if (frontmostPiston == nullptr) return;

    const double theta = bank->getAngle();
    double x, y;
    bank->getPositionAboveDeck(chamberHeight, &x, &y);

    const ysMatrix scale = ysMath::ScaleTransform(ysMath::LoadScalar((float)s));
    const ysMatrix rotation = ysMath::RotationTransform(
            ysMath::Constants::ZAxis, (float)theta);
    const ysMatrix translation = ysMath::TranslationTransform(
            ysMath::LoadVector((float)x, (float)y));
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
    params.lineWidth = 0.1f + m_app->pixelsToUnits(5.0f) / (float)s;
    params.x0 = -0.689352f;
    params.y0 = 0.085f;
    params.x1 = -0.88632f;
    params.y1 = -0.077975f;
    gen->generateLine2d(params);

    params.x0 = -(params.x0 + 0.5f) - 0.5f;
    params.x1 = -(params.x1 + 0.5f) - 0.5f;
    gen->generateLine2d(params);

    params.x0 = 0.689352f;
    params.x1 = 0.88632f;
    gen->generateLine2d(params);

    params.x0 = -(params.x0 - 0.5f) + 0.5f;
    params.x1 = -(params.x1 - 0.5f) + 0.5f;
    gen->generateLine2d(params);

    params.lineWidth = 0.0917f + m_app->pixelsToUnits(5.0f) / (float)s;
    params.x0 = -0.5f;
    params.y0 = 0.5f;
    params.x1 = -0.5f;
    params.y1 = 1.5f;
    gen->generateLine2d(params);

    params.x0 = 0.5f;
    params.x1 = 0.5f;
    gen->generateLine2d(params);

    gen->endShape(&valveShadow);

    constexpr float rollerRadius = (float)units::distance(300.0, units::thou);
    GeometryGenerator::Circle2dParameters circleParams;
    circleParams.radius = rollerRadius / (float)s;
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

    const int layer = frontmostPiston->getCylinderIndex();

    const float intakeLift = (float)m_head->intakeValveLift(layer);
    const ysMatrix T_intakeValve = ysMath::MatMult(
            T_head,
            ysMath::TranslationTransform(
                ysMath::LoadVector(
                    (float)intakeValvePosition,
                    (float)(-intakeLift / s),
                    0.0f,
                    0.0f)));

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

    const double exhaustLift = (float)m_head->exhaustValveLift(layer);
    const ysMatrix T_exhaustValve = ysMath::MatMult(
        T_head,
        ysMath::TranslationTransform(
            ysMath::LoadVector(
                (float)(-intakeValvePosition),
                (float)(-exhaustLift / s),
                0.0f,
                0.0f)));

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
            (float)(-intakeValvePosition * s),
            m_app->pixelsToUnits(5.0f) / 2 + (float)(1.99 * s + exhaustCam->getBaseRadius() + rollerRadius),
            0.0f,
            0.0f)));
    T_exhaustCam = ysMath::MatMult(
        T_exhaustCam,
        ysMath::RotationTransform(
            ysMath::Constants::ZAxis,
            (float)(
                exhaustCam->getAngle()
                + exhaustCam->getLobeCenterline(layer))));

    m_app->getShaders()->SetObjectTransform(T_exhaustCam);
    m_app->getShaders()->SetBaseColor(m_app->getYellow());
    m_app->drawGenerated(exhaust);
    m_app->getShaders()->SetBaseColor(m_app->getBackgroundColor());
    m_app->drawGenerated(camCenter);

    ysMatrix T_intakeCam = ysMath::MatMult(
        T_headObject,
        ysMath::TranslationTransform(ysMath::LoadVector(
            (float)(intakeValvePosition * s),
            rollerRadius + m_app->pixelsToUnits(5.0f) / 2 + (float)(1.99 * s + intakeCam->getBaseRadius()),
            0.0f,
            0.0f)));
    T_intakeCam = ysMath::MatMult(
        T_intakeCam,
        ysMath::RotationTransform(
            ysMath::Constants::ZAxis,
            (float)(
                intakeCam->getAngle()
                + intakeCam->getLobeCenterline(layer))));

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
    params.center_x = 0.0f;
    params.center_y = 0.0f;
    params.rollerRadius = (float)rollerRadius;
    params.lift = camshaft->getLobeProfile();
    params.maxEdgeLength = (float)units::distance(50, units::thou);

    m_app->getGeometryGenerator()->startShape();
    m_app->getGeometryGenerator()->generateCam(params);
    m_app->getGeometryGenerator()->endShape(indices);
}
