#include "../include/simple_pendulum_application.h"

#include "../include/matrix.h"
#include "../include/euler_solver.h"
#include "../include/rk4_solver.h"
#include "../include/gauss_seidel_sle_solver.h"

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
    m_state.ConstrainedPosition_X = 0.0f;
    m_state.ConstrainedPosition_Y = 0.0f;
    m_state.Force_X = 0.0f;
    m_state.Force_Y = 0.0f;
    m_state.Torque = 0.0f;

    m_textRenderer.SetEngine(&m_engine);
    m_textRenderer.SetRenderer(m_engine.GetUiRenderer());
    m_textRenderer.SetFont(m_engine.GetConsole()->GetFont());

    m_solver = new Rk4Solver();
    m_sleSolver = new GaussSeidelSleSolver();

    OdeSolver::initializeSystem(&m_system, 1, 0.0);
    m_system.Angles[0] = -(ysMath::Constants::PI / 2.0f) * 0.75f;
    m_system.AngularVelocity[0] = 0.0;
    m_system.Position_X[0] = 0.0f;
    m_system.Position_Y[0] = 0.0f;
    m_system.Velocity_X[0] = 0.0f;
    m_system.Velocity_Y[0] = 0.0f;

    m_lambda.initialize(1, 2, 0.0);
}

void SimplePendulumApplication::process(float dt) {
    const int steps = 1000;

    m_t += dt;

    double sim_dt = 1 / 60.0;
    
    OdeSolver::System s0, s1;
    OdeSolver::initializeSystem(&s0, 1, sim_dt / steps);
    OdeSolver::initializeSystem(&s1, 1, sim_dt / steps);

    for (int i = 0; i < steps; ++i) {
        m_solver->start(&m_system, sim_dt / steps);

        while (true) {
            const bool done = m_solver->step(&s0);
            updatePhysics(&s0, &s1, s0.dt);

            m_solver->solve(&s1, &s0);

            if (done) break;
        }

        OdeSolver::copySystem(&s0, &m_system);
        
        m_solver->end();
    }

    OdeSolver::destroySystem(&s0);
    OdeSolver::destroySystem(&s1);
}

void SimplePendulumApplication::render() {
    ysMatrix mat = ysMath::RotationTransform(
            ysMath::LoadVector(0.0f, 0.0f, 1.0f),
            (float)m_system.Angles[0]);
    mat = ysMath::MatMult(
        ysMath::TranslationTransform(
            ysMath::LoadVector(
                (float)m_system.Position_X[0],
                (float)m_system.Position_Y[0],
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
    lineParams.start = ysMath::LoadVector((float)-m_state.Radius, 0.0f, 0.0f);
    lineParams.end = ysMath::LoadVector((float)m_state.Radius, 0.0f, 0.0f);
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
    ss << (m_system.AngularVelocity[0] / (ysMath::Constants::TWO_PI)) * 60 << " RPM";

    const float screenWidth = (float)m_engine.GetScreenWidth();
    const float screenHeight = (float)m_engine.GetScreenHeight();
    m_textRenderer.RenderText(ss.str(), -screenWidth / 2.0f, screenHeight / 2.0f - 32.0f, 32.0f);

    ss = std::stringstream();
    ss << m_engine.GetAverageFramerate() << " FPS";
    m_textRenderer.RenderText(ss.str(), -screenWidth / 2.0f, screenHeight / 2.0f - 64.0f, 32.0f);
}

void SimplePendulumApplication::updatePhysics(
        OdeSolver::System *in,
        OdeSolver::System *out,
        double dt)
{
    const double m1 = 1.0f;
    const double i1 = 2.0f;

    m_state.Force_Y = -10.0f * m1;

    if (std::abs(in->AngularVelocity[0]) > 0.00001f) {
        m_state.Torque = -(in->AngularVelocity[0] / std::abs(in->AngularVelocity[0])) * 10.0f;
    }
    else {
        m_state.Torque = 0.0f;
    }

    if (m_engine.IsKeyDown(ysKey::Code::A)) {
        m_state.Torque -= 500.0f;
    }

    if (m_engine.IsKeyDown(ysKey::Code::S)) {
        m_state.Torque += 500.0f;
    }

    const double q1 = in->Position_X[0];
    const double q2 = in->Position_Y[0];
    const double q3 = in->Angles[0];

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
    const double dpivotX_dq3_2 = -cos(q3) * m_state.Radius;

    const double dpivotY_dq1_2 = 0.0;
    const double dpivotY_dq2_2 = 0.0;
    const double dpivotY_dq3_2 = -sin(q3) * m_state.Radius;

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
    J_dot.set(0, 0, dC1_dq1_2 * in->Velocity_X[0]);
    J_dot.set(1, 0, dC1_dq2_2 * in->Velocity_Y[0]);
    J_dot.set(2, 0, dC1_dq3_2 * in->AngularVelocity[0]);
    J_dot.set(0, 1, dC2_dq1_2 * in->Velocity_X[0]);
    J_dot.set(1, 1, dC2_dq2_2 * in->Velocity_Y[0]);
    J_dot.set(2, 1, dC2_dq3_2 * in->AngularVelocity[0]);

    Matrix q_dot(1, 3, 0.0);
    q_dot.set(0, 0, in->Velocity_X[0]);
    q_dot.set(0, 1, in->Velocity_Y[0]);
    q_dot.set(0, 2, in->AngularVelocity[0]);

    Matrix C_ks(1, 2, 0.0);
    C_ks.set(0, 0, C1 * 5000.0);
    C_ks.set(0, 1, C2 * 5000.0);

    Matrix C_kd(1, 2, 0.0);
    J.multiply(q_dot, &C_kd);
    C_kd.set(0, 0, C_kd.get(0, 0) * 20.0);
    C_kd.set(0, 1, C_kd.get(0, 1) * 20.0);

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
    const double invDet =
        1 / (left.get(0, 0) * left.get(1, 1) - left.get(1, 0) * left.get(0, 1));
    leftInv.set(0, 0, invDet * left.get(1, 1));
    leftInv.set(1, 0, invDet * -left.get(1, 0));
    leftInv.set(0, 1, invDet * -left.get(0, 1));
    leftInv.set(1, 1, invDet * left.get(0, 0));

    Matrix lambda(1, 2, 0.0);
    leftInv.multiply(right, &lambda);

    Matrix lambdaSleSolver(1, 2, 0.0);
    m_sleSolver->solve(&left, &right, &m_lambda, &m_lambda);

    Matrix F_C(1, 3, 0.0);
    J_T.multiply(m_lambda, &F_C);

    Matrix F_C_T(3, 1, 0.0);
    F_C.transpose(&F_C_T);

    // Update system
    const double F_C_x = F_C.get(0, 0);
    const double F_C_y = F_C.get(0, 1);
    const double F_C_t = F_C.get(0, 2);

    out->dt = dt;
    out->BodyCount = in->BodyCount;

    out->Acceleration_X[0] = (F_C_x + m_state.Force_X) / m1;
    out->Acceleration_Y[0] = (F_C_y + m_state.Force_Y) / m1;
    out->AngularAcceleration[0] = (F_C_t + m_state.Torque) / i1;

    out->Angles[0] = in->Angles[0];
    out->AngularVelocity[0] = in->AngularVelocity[0];
    out->Position_X[0] = in->Position_X[0];
    out->Position_Y[0] = in->Position_Y[0];
    out->Velocity_X[0] = in->Velocity_X[0];
    out->Velocity_Y[0] = in->Velocity_Y[0];
}
