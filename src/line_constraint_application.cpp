#include "../include/line_constraint_application.h"

#include <sstream>

LineConstraintApplication::LineConstraintApplication() {
    m_r0 = 0.75;
    m_r1 = 1.5;
    m_g = 10.0;
}

LineConstraintApplication::~LineConstraintApplication() {
    /* void */
}

void LineConstraintApplication::initialize() {
    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));
    m_assetManager.CompileInterchangeFile((m_assetPath + "/icosphere").c_str(), 1.0f, true);
    m_assetManager.LoadSceneFile((m_assetPath + "/icosphere").c_str(), true);
    m_assetManager.LoadTexture((m_assetPath + "/chicken.png").c_str(), "Chicken");
    m_assetManager.LoadTexture(
            (m_assetPath + "/dashed_line_pencil_2.png").c_str(), "Dashed Line");

    setCameraPosition(ysMath::LoadVector(0.0f, 0.0f, 10.0f));
    setCameraTarget(ysMath::LoadVector(0.0f, 0.0f, 0.0f));
    setCameraUp(ysMath::Constants::YAxis);

    m_textRenderer.SetEngine(&m_engine);
    m_textRenderer.SetRenderer(m_engine.GetUiRenderer());
    m_textRenderer.SetFont(m_engine.GetConsole()->GetFont());

    const double rodDensity = 1.0;

    m_pendulum0.m_mass = rodDensity * m_r0 * 2 * 10;
    m_pendulum0.m_inertiaTensor = m_pendulum0.m_mass * m_r0 * m_r0;
    m_pendulum0.m_positionY = -2.0 + m_r0;
    m_pendulum0.m_orientation = ysMath::Constants::PI / 2;
    m_pendulum0.m_angularVelocity = 10.0;

    m_pendulum1.m_mass = rodDensity * m_r0 * 2;
    m_pendulum1.m_inertiaTensor = m_pendulum1.m_mass * m_r1 * m_r1;
    m_pendulum1.m_positionY = 2.0 + m_r1;
    m_pendulum1.m_positionX = 0.1;
    m_pendulum1.m_orientation = ysMath::Constants::PI / 2;

    m_c0.m_body = &m_pendulum0;
    m_c0.m_bodyX = -m_r0;

    m_c1.m_body1 = &m_pendulum0;
    m_c1.m_body2 = &m_pendulum1;
    m_c1.m_body1PositionX = m_r0;
    m_c1.m_body2PositionX = -m_r1;

    m_c2.m_body = &m_pendulum1;
    m_c2.m_bodyX = m_r1;
    m_c2.m_dX = 0.0;
    m_c2.m_dY = 1.0;
    m_c2.m_kd = 2.0;
    m_c2.m_ks = 500.0;

    m_fg0.m_body = &m_pendulum0;
    m_fg0.m_forceY = -m_g * m_pendulum0.m_mass;
    
    m_fg1.m_body = &m_pendulum1;
    m_fg1.m_forceY = -m_g * m_pendulum1.m_mass;

    m_friction1.m_body0 = &m_pendulum0;
    m_friction1.m_body1 = &m_pendulum1;
    m_friction1.m_constraint = &m_c1;
    m_friction1.m_frictionCoefficient = 0.025;

    m_friction0.m_body0 = &m_pendulum0;
    m_friction0.m_constraint = &m_c0;
    m_friction0.m_frictionCoefficient = 0.025;

    m_rigidBodySystem.addConstraint(&m_c0);
    m_rigidBodySystem.addConstraint(&m_c1);
    m_rigidBodySystem.addConstraint(&m_c2);

    m_rigidBodySystem.addForceGenerator(&m_fg0);
    m_rigidBodySystem.addForceGenerator(&m_fg1);
    //m_rigidBodySystem.addForceGenerator(&m_friction0);
    //m_rigidBodySystem.addForceGenerator(&m_friction1);

    m_rigidBodySystem.addRigidBody(&m_pendulum0);
    m_rigidBodySystem.addRigidBody(&m_pendulum1);

    m_rigidBodySystem.initialize();
}

void LineConstraintApplication::process(float dt) {
    m_rigidBodySystem.process(1 / 60.0);

    if (m_engine.ProcessKeyDown(ysKey::Code::Space)) {
        m_pendulum1.m_angularVelocity = 1;
    }
}

void LineConstraintApplication::render() {
    drawPendulum(0, 0, m_pendulum0.m_positionX, m_pendulum0.m_positionY, m_pendulum0.m_orientation, m_r0);
    drawPendulum(0, 0, m_pendulum1.m_positionX, m_pendulum1.m_positionY, m_pendulum1.m_orientation, m_r1);

    const double g_potential =
        m_g * (m_pendulum1.m_positionY + m_r0 * 2 + m_r1) * m_pendulum1.m_mass +
        m_g * (m_pendulum0.m_positionY + m_r0) * m_pendulum0.m_mass;

    std::stringstream ss;
    ss << "Line Constraint Demo\n";
    ss << " - System Energy    \n";
    ss << " - Framerate            \n";

    const float screenWidth = (float)m_engine.GetScreenWidth();
    const float screenHeight = (float)m_engine.GetScreenHeight();
    m_textRenderer.RenderText(
            ss.str(), -screenWidth / 2.0f, screenHeight / 2.0f - 32.0f, 32.0f);

    ss = std::stringstream();
    ss << m_pendulum0.energy() + m_pendulum1.energy() + g_potential << "       \n";
    ss << (int)std::round(m_engine.GetAverageFramerate()) << " FPS               ";

    m_textRenderer.RenderText(
        ss.str(), -screenWidth / 2.0f + 400.0f, screenHeight / 2.0f - 64.0f, 32.0f);
}

void LineConstraintApplication::drawPendulum(
    double hinge_x,
    double hinge_y,
    double x,
    double y,
    double angle,
    double r)
{
    ysMatrix mat = ysMath::RotationTransform(
        ysMath::LoadVector(0.0f, 0.0f, 1.0f),
        (float)angle);
    mat = ysMath::MatMult(
        ysMath::TranslationTransform(
            ysMath::LoadVector(
                (float)x,
                (float)y,
                0.0f,
                0.0f)),
        mat);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetMetallic(0.8f);
    m_shaders.SetIncidentSpecular(0.8f);
    m_shaders.SetSpecularRoughness(0.7f);
    m_shaders.SetSpecularMix(1.0f);
    m_shaders.SetDiffuseMix(1.0f);
    m_shaders.SetEmission(
        ysMath::Mul(ysColor::srgbiToLinear(0xff, 0x0, 0x0), ysMath::LoadScalar(0)));
    m_shaders.SetBaseColor(ysMath::LoadVector(1.0f, 1.0f, 1.0f, 1.0f));
    m_shaders.SetColorReplace(false);
    m_shaders.SetLit(false);
    m_shaders.SetDiffuseTexture(m_assetManager.GetTexture("Dashed Line")->GetTexture());

    GeometryGenerator::GeometryIndices indices;

    m_shaders.SetObjectTransform(ysMath::TranslationTransform(
        ysMath::LoadVector(
            (float)hinge_x,
            (float)hinge_y,
            0.0f,
            0.0f)));

    GeometryGenerator::LineRingParameters params{};
    params.center = ysMath::LoadVector(0.0f, 0.0f, 0.0f);
    params.normal = ysMath::Constants::ZAxis;
    params.radius = 0.3f;
    params.maxEdgeLength = 0.1f;
    params.startAngle = 0.0f;
    params.endAngle = ysMath::Constants::TWO_PI;
    params.patternHeight = 1.0f;
    params.textureWidthHeightRatio = 5.0;
    params.taperTail = ysMath::Constants::TWO_PI * 0.1f;

    m_geometryGenerator.generateLineRingBalanced(
        &indices,
        params);
    drawGenerated(indices);

    m_shaders.SetObjectTransform(mat);

    GeometryGenerator::LineParameters lineParams{};
    lineParams.start = ysMath::LoadVector((float)-r, 0.0f, 0.0f);
    lineParams.end = ysMath::LoadVector((float)r, 0.0f, 0.0f);
    lineParams.patternHeight = 1.0f;
    lineParams.textureOffset = 0.0f;
    lineParams.textureWidthHeightRatio = 5.0f;
    lineParams.taperTail = 0.0f;

    m_geometryGenerator.generateLine(
        &indices,
        lineParams);
    drawGenerated(indices);

    GeometryGenerator::LineRingParameters lineEndParams{};
    lineEndParams.center = ysMath::LoadVector(-r, 0.0f, 0.0f);
    lineEndParams.normal = ysMath::Constants::ZAxis;
    lineEndParams.radius = 0.01f;
    lineEndParams.maxEdgeLength = 0.1f;
    lineEndParams.startAngle = 0.0f;
    lineEndParams.endAngle = ysMath::Constants::TWO_PI;
    lineEndParams.patternHeight = 1.0f;
    lineEndParams.textureWidthHeightRatio = 5.0;
    lineEndParams.taperTail = ysMath::Constants::TWO_PI * 0.1f;

    m_geometryGenerator.generateLineRingBalanced(
        &indices,
        lineEndParams);
    drawGenerated(indices);

    lineEndParams.center = ysMath::LoadVector(r, 0.0f, 0.0f);
    lineEndParams.normal = ysMath::Constants::ZAxis;
    lineEndParams.radius = 0.01f;
    lineEndParams.maxEdgeLength = 0.1f;
    lineEndParams.startAngle = 0.0f;
    lineEndParams.endAngle = ysMath::Constants::TWO_PI;
    lineEndParams.patternHeight = 1.0f;
    lineEndParams.textureWidthHeightRatio = 5.0;
    lineEndParams.taperTail = ysMath::Constants::TWO_PI * 0.1f;

    m_geometryGenerator.generateLineRingBalanced(
        &indices,
        lineEndParams);
    drawGenerated(indices);
}
