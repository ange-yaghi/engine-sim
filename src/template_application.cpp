#include "../include/template_application.h"

TemplateApplication::TemplateApplication() {
    m_demoTexture = nullptr;
    m_currentRotation = 0.0f;
    m_temperature = 0.0f;
}

TemplateApplication::~TemplateApplication() {
    /* void */
}

void TemplateApplication::Initialize(void *instance, ysContextObject::DeviceAPI api) {
    dbasic::Path modulePath = dbasic::GetModulePath();
    dbasic::Path confPath = modulePath.Append("delta.conf");

    std::string enginePath = "../dependencies/submodules/delta-studio/engines/basic";
    std::string assetPath = "../assets";
    if (confPath.Exists()) {
        std::fstream confFile(confPath.ToString(), std::ios::in);
        
        std::getline(confFile, enginePath);
        std::getline(confFile, assetPath);
        enginePath = modulePath.Append(enginePath).ToString();
        assetPath = modulePath.Append(assetPath).ToString();

        confFile.close();
    }

    m_engine.GetConsole()->SetDefaultFontDirectory(enginePath + "/fonts/");

    const std::string shaderPath = enginePath + "/shaders/";

    dbasic::DeltaEngine::GameEngineSettings settings;
    settings.API = api;
    settings.DepthBuffer = true;
    settings.Instance = instance;
    settings.ShaderDirectory = shaderPath.c_str();
    settings.WindowTitle = "Delta Template Application";
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

    m_assetManager.LoadTexture((assetPath + "/chicken.png").c_str(), "Chicken");
    m_demoTexture = m_assetManager.GetTexture("Chicken")->GetTexture();

    m_assetManager.CompileInterchangeFile((assetPath + "/icosphere").c_str(), 1.0f, true);
    m_assetManager.LoadSceneFile((assetPath + "/icosphere").c_str(), true);

    m_shaders.SetCameraMode(dbasic::DefaultShaders::CameraMode::Target);
}

void TemplateApplication::Process() {
    if (m_engine.IsKeyDown(ysKey::Code::Space)) {
        m_currentRotation += m_engine.GetFrameLength();
    }

    if (m_engine.IsKeyDown(ysKey::Code::Up)) {
        m_temperature += m_engine.GetFrameLength() * 0.5f;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::Down)) {
        m_temperature -= m_engine.GetFrameLength() * 0.5f;
    }

    if (m_temperature < 0.0f) m_temperature = 0.0f;
    if (m_temperature > 1.0f) m_temperature = 1.0f;
}

void TemplateApplication::Render() {
    const int screenWidth = m_engine.GetGameWindow()->GetGameWidth();
    const int screenHeight = m_engine.GetGameWindow()->GetGameHeight();

    m_shaders.SetScreenDimensions((float)screenWidth, (float)screenHeight);
    m_shaders.CalculateCamera();

    m_shaders.SetCameraPosition(ysMath::LoadVector(4.0f, 4.0f, 2.0f));
    m_shaders.SetCameraUp(ysMath::Constants::ZAxis);

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
    glow.Color = ysVector4(5.0f * m_temperature, 0.0f, 0.0f, 1.0f);
    glow.Direction = ysVector4(0.0f, 0.0f, 0.0f, 0.0f);
    glow.FalloffEnabled = 1;
    glow.Position = ysVector4(0.0f, 0.0f, 0.0f);
    m_shaders.AddLight(glow);

    const ysMatrix rotationTurntable = ysMath::RotationTransform(ysMath::Constants::ZAxis, m_currentRotation); 

    m_shaders.ResetBrdfParameters();
    m_shaders.SetMetallic(0.8f);
    m_shaders.SetIncidentSpecular(0.8f);
    m_shaders.SetSpecularRoughness(0.7f);
    m_shaders.SetSpecularMix(1.0f);
    m_shaders.SetDiffuseMix(1.0f);
    m_shaders.SetEmission(ysMath::Mul(ysColor::srgbiToLinear(0xff, 0x0, 0x0), ysMath::LoadScalar(m_temperature)));
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

void TemplateApplication::Run() {
    while (m_engine.IsOpen()) {
        m_engine.StartFrame();

        Process();
        Render();

        m_engine.EndFrame();
    }
}

void TemplateApplication::Destroy() {
    m_shaderSet.Destroy();

    m_assetManager.Destroy();
    m_engine.Destroy();
}
