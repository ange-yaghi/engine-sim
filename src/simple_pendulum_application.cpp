#include "../include/simple_pendulum_application.h"

#include "../include/matrix.h"

#include <sstream>

SimplePendulumApplication::SimplePendulumApplication() {
    m_texture = nullptr;
    m_t = 0.0f;
}

SimplePendulumApplication::~SimplePendulumApplication() {
    /* void */
}

void SimplePendulumApplication::initialize() {
    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));
    m_assetManager.CompileInterchangeFile((m_assetPath + "/icosphere").c_str(), 1.0f, true);
    m_assetManager.LoadSceneFile((m_assetPath + "/icosphere").c_str(), true);
    m_assetManager.LoadTexture((m_assetPath + "/chicken.png").c_str(), "Chicken");
    m_assetManager.LoadTexture((m_assetPath + "/dashed_line_pencil_2.png").c_str(), "Dashed Line");

    setCameraPosition(ysMath::LoadVector(0.0f, 0.0f, 10.0f));
    setCameraTarget(ysMath::LoadVector(0.0f, 0.0f, 0.0f));
    setCameraUp(ysMath::Constants::YAxis);

    m_state.Radius = 2.0f;
    m_state.Angle = -(ysMath::Constants::PI / 2.0f) * 0.75f;
    m_state.AngularVelocity = 100.0; // 2 * ysMath::Constants::PI * 100;
    m_state.ConstrainedPosition_X = 0.0f;
    m_state.ConstrainedPosition_Y = 0.0f;
    m_state.Force_X = 0.0f;
    m_state.Force_Y = 0.0f;
    m_state.Position_X = 0.0f;
    m_state.Position_Y = 0.0f;
    m_state.Torque = 0.0f;
    m_state.Velocity_X = 0.0f;
    m_state.Velocity_Y = 0.0f;

    m_textRenderer.SetEngine(&m_engine);
    m_textRenderer.SetRenderer(m_engine.GetUiRenderer());
    m_textRenderer.SetFont(m_engine.GetConsole()->GetFont());
}

void SimplePendulumApplication::process(float dt) {
    m_t += dt;

    for (int i = 0; i < 100; ++i) {
        updatePhysics(dt / (double)100);
    }
}

void SimplePendulumApplication::render() {
    ysMatrix mat = ysMath::RotationTransform(ysMath::LoadVector(0.0f, 0.0f, 1.0f), m_state.Angle);
    mat = ysMath::MatMult(
        ysMath::TranslationTransform(ysMath::LoadVector(m_state.Position_X, m_state.Position_Y, 0.0f, 0.0f)),
        mat);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetMetallic(0.8f);
    m_shaders.SetIncidentSpecular(0.8f);
    m_shaders.SetSpecularRoughness(0.7f);
    m_shaders.SetSpecularMix(1.0f);
    m_shaders.SetDiffuseMix(1.0f);
    m_shaders.SetEmission(ysMath::Mul(ysColor::srgbiToLinear(0xff, 0x0, 0x0), ysMath::LoadScalar(0)));
    m_shaders.SetBaseColor(ysMath::LoadVector(1.0f, 1.0f, 1.0f, 1.0f));
    m_shaders.SetColorReplace(false);
    m_shaders.SetLit(false);
    m_shaders.SetDiffuseTexture(m_assetManager.GetTexture("Dashed Line")->GetTexture());

    GeometryGenerator::GeometryIndices indices;

    m_shaders.SetObjectTransform(ysMath::LoadIdentity());

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
    lineParams.start = ysMath::LoadVector(-m_state.Radius, 0.0f, 0.0f);
    lineParams.end = ysMath::LoadVector(m_state.Radius, 0.0f, 0.0f);
    lineParams.patternHeight = 1.0f;
    lineParams.textureOffset = 0.0f;
    lineParams.textureWidthHeightRatio = 5.0f;
    lineParams.taperTail = 0.0f;
    
    m_geometryGenerator.generateLine(
        &indices,
        lineParams);
    drawGenerated(indices);

    GeometryGenerator::LineRingParameters lineEndParams{};
    lineEndParams.center = ysMath::LoadVector(-m_state.Radius, 0.0f, 0.0f);
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

    lineEndParams.center = ysMath::LoadVector(m_state.Radius, 0.0f, 0.0f);
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

    std::stringstream ss;
    ss << m_state.AngularVelocity << " rad/sec";

    const float screenWidth = m_engine.GetScreenWidth();
    const float screenHeight = m_engine.GetScreenHeight();
    m_textRenderer.RenderText(ss.str(), -screenWidth / 2.0f, screenHeight / 2.0f - 32.0f, 32.0f);
}

void SimplePendulumApplication::updatePhysics(double dt) {
    const double m1 = 1.0f;
    const double i1 = 2.0f;

    m_state.Force_Y = -10.0f * m1;

    if (std::abs(m_state.AngularVelocity) > 0.00001f) {
        m_state.Torque = -(m_state.AngularVelocity / std::abs(m_state.AngularVelocity)) * 10.0f;
    }

    if (m_engine.IsKeyDown(ysKey::Code::Space)) {
        m_state.Torque += 50.0f;
    }

    const double q1 = m_state.Position_X;
    const double q2 = m_state.Position_Y;
    const double q3 = m_state.Angle;

    const double pivotX = q1 + cos(q3) * m_state.Radius;
    const double pivotY = q2 + sin(q3) * m_state.Radius;

    const double dpivotX_dq1 = 1.0;
    const double dpivotX_dq2 = 0.0;
    const double dpivotX_dq3 = -sin(q3) * m_state.Radius;

    const double dpivotY_dq1 = 0.0;
    const double dpivotY_dq2 = 1.0;
    const double dpivotY_dq3 = cos(q3) * m_state.Radius;

    const double dpivotX_dq1_2 = 0.0;
    const double dpivotX_dq2_2 = 0.0;
    const double dpivotX_dq3_2 = -cosf(q3) * m_state.Radius;

    const double dpivotY_dq1_2 = 0.0;
    const double dpivotY_dq2_2 = 0.0;
    const double dpivotY_dq3_2 = -sinf(q3) * m_state.Radius;

    const double C1 = pivotX - m_state.ConstrainedPosition_X;
    const double C2 = pivotY - m_state.ConstrainedPosition_Y;

    const double dC1_dq1 = dpivotX_dq1;
    const double dC1_dq2 = dpivotX_dq2;
    const double dC1_dq3 = dpivotX_dq3;

    const double dC2_dq1 = dpivotY_dq1;
    const double dC2_dq2 = dpivotY_dq2;
    const double dC2_dq3 = dpivotY_dq3;

    const double dC1_dq1_2 = dpivotX_dq1_2;
    const double dC1_dq2_2 = dpivotX_dq2_2;
    const double dC1_dq3_2 = dpivotX_dq3_2;

    const double dC2_dq1_2 = dpivotY_dq1_2;
    const double dC2_dq2_2 = dpivotY_dq2_2;
    const double dC2_dq3_2 = dpivotY_dq3_2;

    Matrix temp0, temp1, temp2;
    
    Matrix M(3, 3, 0.0);
    M.set(0, 0, m1);
    M.set(1, 1, m1);
    M.set(2, 2, i1);

    Matrix M_inv(3, 3, 0.0);
    M_inv.set(0, 0, 1 / m1);
    M_inv.set(1, 1, 1 / m1);
    M_inv.set(2, 2, 1 / i1);

    Matrix J(3, 2, 0.0);
    J.set(0, 0, dC1_dq1);
    J.set(1, 0, dC1_dq2);
    J.set(2, 0, dC1_dq3);
    J.set(0, 1, dC2_dq1);
    J.set(1, 1, dC2_dq2);
    J.set(2, 1, dC2_dq3);

    Matrix J_T(2, 3, 0.0);
    J.transpose(&J_T);

    Matrix J_dot(3, 2, 0.0);
    J_dot.set(0, 0, dC1_dq1_2 * m_state.Velocity_X);
    J_dot.set(1, 0, dC1_dq2_2 * m_state.Velocity_Y);
    J_dot.set(2, 0, dC1_dq3_2 * m_state.AngularVelocity);
    J_dot.set(0, 1, dC2_dq1_2 * m_state.Velocity_X);
    J_dot.set(1, 1, dC2_dq2_2 * m_state.Velocity_Y);
    J_dot.set(2, 1, dC2_dq3_2 * m_state.AngularVelocity);

    Matrix q_dot(1, 3, 0.0);
    q_dot.set(0, 0, m_state.Velocity_X);
    q_dot.set(0, 1, m_state.Velocity_Y);
    q_dot.set(0, 2, m_state.AngularVelocity);

    Matrix C_ks(1, 2, 0.0);
    C_ks.set(0, 0, C1 * 5000.1);
    C_ks.set(0, 1, C2 * 5000.1);

    Matrix C_kd(1, 2, 0.0);
    J.multiply(q_dot, &C_kd);
    C_kd.set(0, 0, C_kd.get(0, 0) * 60.0);
    C_kd.set(0, 1, C_kd.get(0, 1) * 60.0);

    Matrix F_ext(1, 3, 0.0);
    F_ext.set(0, 0, m_state.Force_X);
    F_ext.set(0, 1, m_state.Force_Y);
    F_ext.set(0, 2, m_state.Torque);

    temp0.initialize(3, 2, 0.0);
    Matrix left(2, 2, 0.0);
    J.multiply(M_inv, &temp0);
    temp0.multiply(J_T, &left);

    temp0.initialize(1, 2, 0.0);
    temp1.initialize(1, 2, 0.0);
    J_dot.multiply(q_dot, &temp0);
    temp0.negate(&temp1);

    temp0.initialize(1, 2, 0.0);
    temp2.initialize(3, 2, 0.0);
    J.multiply(M_inv, &temp2);
    temp2.multiply(F_ext, &temp0);

    Matrix right(1, 2, 0.0);
    temp2.initialize(1, 2, 0.0);
    temp1.subtract(temp0, &temp2);
    temp2.subtract(C_ks, &temp1);
    temp1.subtract(C_kd, &right);

    Matrix leftInv(2, 2, 0.0);
    const double invDet = 1 / (left.get(0, 0) * left.get(1, 1) - left.get(1, 0) * left.get(0, 1));
    leftInv.set(0, 0, invDet * left.get(1, 1));
    leftInv.set(1, 0, invDet * -left.get(1, 0));
    leftInv.set(0, 1, invDet * -left.get(0, 1));
    leftInv.set(1, 1, invDet * left.get(0, 0));

    Matrix lambda(1, 2, 0.0);
    leftInv.multiply(right, &lambda);

    Matrix F_C(1, 3, 0.0);
    J_T.multiply(lambda, &F_C);

    Matrix F_C_T(3, 1, 0.0);
    F_C.transpose(&F_C_T);

    // Update states
    const float F_C_x = F_C.get(0, 0);
    const float F_C_y = F_C.get(0, 1);
    const float F_C_t = F_C.get(0, 2);

    m_state.Position_X += m_state.Velocity_X * dt;
    m_state.Position_Y += m_state.Velocity_Y * dt;
    m_state.Angle += m_state.AngularVelocity * dt;

    const float a_x = (F_C_x + m_state.Force_X) / m1;
    const float a_y = (F_C_y + m_state.Force_Y) / m1;
    const float a_t = (F_C_t + m_state.Torque) / i1;

    m_state.Velocity_X += a_x * dt;
    m_state.Velocity_Y += a_y * dt;
    m_state.AngularVelocity += a_t * dt;
}
