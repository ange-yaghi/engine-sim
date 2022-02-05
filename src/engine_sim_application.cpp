#include "../include/engine_sim_application.h"

EngineSimApplication::EngineSimApplication() {
    m_cameraTarget = ysMath::Constants::Zero;
    m_cameraPosition = ysMath::LoadVector(0.0f, 5.0f, 5.0f);
    m_cameraUp = ysMath::Constants::ZAxis;

    m_assetPath = "";

    m_geometryVertexBuffer = nullptr;
    m_geometryIndexBuffer = nullptr;
}

EngineSimApplication::~EngineSimApplication() {
    /* void */
}

void EngineSimApplication::initialize(void *instance, ysContextObject::DeviceAPI api) {
    dbasic::Path modulePath = dbasic::GetModulePath();
    dbasic::Path confPath = modulePath.Append("delta.conf");

    std::string enginePath = "../dependencies/submodules/delta-studio/engines/basic";
    m_assetPath = "../assets";
    if (confPath.Exists()) {
        std::fstream confFile(confPath.ToString(), std::ios::in);

        std::getline(confFile, enginePath);
        std::getline(confFile, m_assetPath);
        enginePath = modulePath.Append(enginePath).ToString();
        m_assetPath = modulePath.Append(m_assetPath).ToString();

        confFile.close();
    }

    m_engine.GetConsole()->SetDefaultFontDirectory(enginePath + "/fonts/");

    const std::string shaderPath = enginePath + "/shaders/";

    dbasic::DeltaEngine::GameEngineSettings settings;
    settings.API = api;
    settings.DepthBuffer = true;
    settings.Instance = instance;
    settings.ShaderDirectory = shaderPath.c_str();
    settings.WindowTitle = "Engine Sim | AngeTheGreat";
    settings.WindowPositionX = 0;
    settings.WindowPositionY = 0;
    settings.WindowStyle = ysWindow::WindowStyle::Windowed;

    m_engine.CreateGameWindow(settings);

    m_engine.InitializeShaderSet(&m_shaderSet);
    m_engine.InitializeDefaultShaders(&m_shaders, &m_shaderSet);
    m_engine.InitializeConsoleShaders(&m_shaderSet);
    m_engine.SetShaderSet(&m_shaderSet);

    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));

    m_assetManager.SetEngine(&m_engine);

    m_shaders.SetCameraMode(dbasic::DefaultShaders::CameraMode::Target);

    m_engine.GetDevice()->CreateIndexBuffer(
        &m_geometryIndexBuffer, sizeof(unsigned short) * 100000, nullptr);
    m_engine.GetDevice()->CreateVertexBuffer(
        &m_geometryVertexBuffer, sizeof(dbasic::Vertex) * 50000, nullptr);

    m_geometryGenerator.initialize(50000, 100000);

    initialize();
}

void EngineSimApplication::initialize() {
    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));
    m_assetManager.CompileInterchangeFile((m_assetPath + "/icosphere").c_str(), 1.0f, true);
    m_assetManager.LoadSceneFile((m_assetPath + "/icosphere").c_str(), true);
}

void EngineSimApplication::process(float dt) {
    /* void */
}

void EngineSimApplication::render() {
    m_shaders.ResetLights();
    m_shaders.SetAmbientLight(ysMath::GetVector4(ysColor::srgbiToLinear(0x34, 0x98, 0xdb)));

    dbasic::Light light;
    light.Active = 1;
    light.Attenuation0 = 0.0f;
    light.Attenuation1 = 0.0f;
    light.Color = ysVector4(0.85f, 0.85f, 0.8f, 1.0f);
    light.Direction = ysVector4(0.0f, 0.0f, 0.0f, 0.0f);
    light.FalloffEnabled = 0;
    light.Position = ysVector4(10.0f, 10.0f, 10.0f);
    m_shaders.AddLight(light);

    dbasic::Light light2;
    light2.Active = 1;
    light2.Attenuation0 = -1.0f;
    light2.Attenuation1 = -1.0f;
    light2.Color = ysVector4(0.3f, 0.3f, 0.5f, 1.0f);
    light2.Direction = ysVector4(0.0f, 0.0f, 0.0f, 0.0f);
    light2.FalloffEnabled = 0;
    light2.Position = ysVector4(-10.0f, 10.0f, 10.0f);
    m_shaders.AddLight(light2);

    dbasic::Light glow;
    glow.Active = 1;
    glow.Attenuation0 = 0.0f;
    glow.Attenuation1 = 0.0f;
    glow.Color = ysVector4(5.0f * 0.0f, 0.0f, 0.0f, 1.0f);
    glow.Direction = ysVector4(0.0f, 0.0f, 0.0f, 0.0f);
    glow.FalloffEnabled = 1;
    glow.Position = ysVector4(0.0f, 0.0f, 0.0f);
    m_shaders.AddLight(glow);

    const ysMatrix rotationTurntable = ysMath::RotationTransform(ysMath::Constants::ZAxis, 0);

    m_shaders.ResetBrdfParameters();
    m_shaders.SetMetallic(0.8f);
    m_shaders.SetIncidentSpecular(0.8f);
    m_shaders.SetSpecularRoughness(0.7f);
    m_shaders.SetSpecularMix(1.0f);
    m_shaders.SetDiffuseMix(1.0f);
    m_shaders.SetEmission(ysMath::Mul(ysColor::srgbiToLinear(0xff, 0x0, 0x0), ysMath::LoadScalar(0)));
    m_shaders.SetBaseColor(ysColor::srgbiToLinear(0x34, 0x49, 0x5e));
    m_shaders.SetColorReplace(true);
    m_shaders.SetObjectTransform(ysMath::MatMult(ysMath::TranslationTransform(ysMath::LoadVector(0.0f, 0.0f, 0.0f)), rotationTurntable));
    m_engine.DrawModel(m_shaders.GetRegularFlags(), m_assetManager.GetModelAsset("Icosphere"));

    m_shaders.ResetBrdfParameters();
    m_shaders.SetMetallic(0.0f);
    m_shaders.SetIncidentSpecular(0.0f);
    m_shaders.SetSpecularRoughness(0.8f);
    m_shaders.SetSpecularMix(0.1f);
    m_shaders.SetDiffuseMix(1.0f);
    m_shaders.SetColorReplace(true);
    m_shaders.SetBaseColor(ysColor::srgbiToLinear(0xbd, 0xc3, 0xc7));
    m_shaders.SetObjectTransform(ysMath::MatMult(ysMath::TranslationTransform(ysMath::LoadVector(0.0f, 0.0f, 0.0f)), rotationTurntable));
    m_shaders.SetObjectTransform(ysMath::MatMult(ysMath::TranslationTransform(ysMath::LoadVector(0.0f, 0.0f, -1.0f)), rotationTurntable));
    m_engine.DrawModel(m_shaders.GetRegularFlags(), m_assetManager.GetModelAsset("Floor"));
}

void EngineSimApplication::run() {
    while (m_engine.IsOpen()) {
        m_engine.StartFrame();

        process(m_engine.GetFrameLength());
        renderScene();

        m_engine.EndFrame();
    }
}

void EngineSimApplication::destroy() {
    m_shaderSet.Destroy();

    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryVertexBuffer);
    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryIndexBuffer);

    m_assetManager.Destroy();
    m_engine.Destroy();
}

void EngineSimApplication::drawGenerated(const GeometryGenerator::GeometryIndices &indices) {
    if (indices.Failed) return;

    m_engine.DrawGeneric(
        m_shaders.GetRegularFlags(),
        m_geometryIndexBuffer,
        m_geometryVertexBuffer,
        sizeof(dbasic::Vertex),
        indices.BaseIndex,
        indices.BaseVertex,
        indices.FaceCount);
}

void EngineSimApplication::renderScene() {
    const int screenWidth = m_engine.GetGameWindow()->GetGameWidth();
    const int screenHeight = m_engine.GetGameWindow()->GetGameHeight();

    m_shaders.SetScreenDimensions((float)screenWidth, (float)screenHeight);

    m_shaders.SetCameraPosition(m_cameraPosition);
    m_shaders.SetCameraTarget(m_cameraTarget);
    m_shaders.SetCameraUp(m_cameraUp);
    m_shaders.CalculateCamera();

    m_geometryGenerator.reset();

    render();

    m_engine.GetDevice()->EditBufferDataRange(
        m_geometryVertexBuffer,
        (char *)m_geometryGenerator.getVertexData(),
        sizeof(dbasic::Vertex) * m_geometryGenerator.getCurrentVertexCount(),
        0);

    m_engine.GetDevice()->EditBufferDataRange(
        m_geometryIndexBuffer,
        (char *)m_geometryGenerator.getIndexData(),
        sizeof(unsigned short) * m_geometryGenerator.getCurrentIndexCount(),
        0);
}
