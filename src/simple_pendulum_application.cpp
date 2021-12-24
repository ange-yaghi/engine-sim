#include "../include/simple_pendulum_application.h"

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
    m_assetManager.LoadTexture((m_assetPath + "/dashed_line_pencil.png").c_str(), "Dashed Line");

    setCameraPosition(ysMath::LoadVector(0.0f, 0.0f, 10.0f));
    setCameraTarget(ysMath::LoadVector(0.0f, 0.0f, 0.0f));
    setCameraUp(ysMath::Constants::YAxis);
}

void SimplePendulumApplication::process(float dt) {
    m_t += dt;
}

void SimplePendulumApplication::render() {
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
    m_shaders.SetObjectTransform(ysMath::TranslationTransform(ysMath::LoadVector(0.0f, 0.0f, 0.0f)));
    //m_engine.DrawBox(m_shaders.GetRegularFlags());

    GeometryGenerator::GeometryIndices indices;
    m_geometryGenerator.generateLineRing(
        &indices,
        ysMath::Constants::ZAxis,
        ysMath::LoadVector(0.0f, 0.0f, 0.0f),
        (sin(m_t) + 1.0f) * 3.0f / 2.0f + 3.0f,
        1.0f,
        0.1f,
        0.0f,
        ysMath::Constants::TWO_PI * 0.8f,
        ysMath::Constants::TWO_PI * 0.25f);
    drawGenerated(indices);
}
