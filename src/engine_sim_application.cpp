#include "../include/engine_sim_application.h"

#include "../include/piston_object.h"
#include "../include/connecting_rod_object.h"
#include "../include/constants.h"
#include "../include/cylinder_pressure_gauge.h"
#include "../include/units.h"
#include "../include/crankshaft_object.h"
#include "../include/cylinder_bank_object.h"
#include "../include/cylinder_head_object.h"

#include <sstream>

EngineSimApplication::EngineSimApplication() {
    m_cameraTarget = ysMath::Constants::Zero;
    m_cameraPosition = ysMath::LoadVector(0.0f, 0.0f, 5.0f);
    m_cameraUp = ysMath::Constants::YAxis;

    m_assetPath = "";

    m_geometryVertexBuffer = nullptr;
    m_geometryIndexBuffer = nullptr;

    m_paused = true;
    m_recording = false;
    m_screenResolutionIndex = 0;
    for (int i = 0; i < ScreenResolutionHistoryLength; ++i) {
        m_screenResolution[i][0] = m_screenResolution[i][1] = 0;
    }

    m_background = ysColor::srgbiToLinear(0x0E1012);
    m_foreground = ysColor::srgbiToLinear(0xFFFFFF);
    m_shadow = ysColor::srgbiToLinear(0x0E1012);
    m_highlight1 = ysColor::srgbiToLinear(0xEF4545);
    m_highlight2 = ysColor::srgbiToLinear(0xFFFFFF);

    m_displayHeight = units::distance(2.0, units::foot);
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
    settings.DepthBuffer = false;
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
    m_assetManager.CompileInterchangeFile((m_assetPath + "/assets").c_str(), 1.0f, true);
    m_assetManager.LoadSceneFile((m_assetPath + "/assets").c_str(), true);

    m_textRenderer.SetEngine(&m_engine);
    m_textRenderer.SetRenderer(m_engine.GetUiRenderer());
    m_textRenderer.SetFont(m_engine.GetConsole()->GetFont());

    Engine::Parameters engineParams;
    engineParams.CylinderBanks = 2;
    engineParams.CylinderCount = 8;
    engineParams.CrankshaftCount = 1;
    m_iceEngine.initialize(engineParams);

    Piston::Parameters pistonParams;
    pistonParams.Bank = m_iceEngine.getCylinderBank(0);
    pistonParams.CompressionHeight = units::distance(1.640, units::inch);
    pistonParams.CylinderIndex = 0;
    pistonParams.Displacement = 0;
    pistonParams.Rod = m_iceEngine.getConnectingRod(0);
    pistonParams.WristPinPosition = 0;
    pistonParams.Mass = units::mass(880, units::g);
    m_iceEngine.getPiston(0)->initialize(pistonParams);

    pistonParams.Bank = m_iceEngine.getCylinderBank(1);
    pistonParams.Rod = m_iceEngine.getConnectingRod(1);
    m_iceEngine.getPiston(1)->initialize(pistonParams);

    pistonParams.CylinderIndex = 1;
    pistonParams.Bank = m_iceEngine.getCylinderBank(0);
    pistonParams.Rod = m_iceEngine.getConnectingRod(2);
    m_iceEngine.getPiston(2)->initialize(pistonParams);

    pistonParams.Bank = m_iceEngine.getCylinderBank(1);
    pistonParams.Rod = m_iceEngine.getConnectingRod(3);
    m_iceEngine.getPiston(3)->initialize(pistonParams);

    pistonParams.CylinderIndex = 2;
    pistonParams.Bank = m_iceEngine.getCylinderBank(0);
    pistonParams.Rod = m_iceEngine.getConnectingRod(4);
    m_iceEngine.getPiston(4)->initialize(pistonParams);

    pistonParams.Bank = m_iceEngine.getCylinderBank(1);
    pistonParams.Rod = m_iceEngine.getConnectingRod(5);
    m_iceEngine.getPiston(5)->initialize(pistonParams);

    pistonParams.CylinderIndex = 3;
    pistonParams.Bank = m_iceEngine.getCylinderBank(0);
    pistonParams.Rod = m_iceEngine.getConnectingRod(6);
    m_iceEngine.getPiston(6)->initialize(pistonParams);

    pistonParams.Bank = m_iceEngine.getCylinderBank(1);
    pistonParams.Rod = m_iceEngine.getConnectingRod(7);
    m_iceEngine.getPiston(7)->initialize(pistonParams);

    CylinderBank::Parameters cbParams;
    cbParams.Bore = units::distance(4.25, units::inch);
    cbParams.CylinderCount = 4;
    cbParams.DeckHeight = units::distance(9.8, units::inch);

    cbParams.Index = 0;
    cbParams.Angle = Constants::pi / 4;
    m_iceEngine.getCylinderBank(0)->initialize(cbParams);

    cbParams.Index = 1;
    cbParams.Angle = -Constants::pi / 4;
    m_iceEngine.getCylinderBank(1)->initialize(cbParams);

    Crankshaft::Parameters crankshaftParams;
    crankshaftParams.CrankThrow = units::distance(2.0, units::inch);
    crankshaftParams.FlywheelMass = units::mass(29, units::lb);
    crankshaftParams.Mass = units::mass(75, units::lb);

    // Temporary moment of inertia approximation
    const double crank_r = crankshaftParams.CrankThrow;
    const double flywheel_r = units::distance(14.0, units::inch) / 2.0;
    const double I_crank = (1 / 2.0) * crankshaftParams.Mass * crank_r;
    const double I_flywheel =
        (1 / 12.0) * crankshaftParams.FlywheelMass * flywheel_r * flywheel_r;

    crankshaftParams.MomentOfInertia = I_crank + I_flywheel;
    crankshaftParams.Pos_x = 0;
    crankshaftParams.Pos_y = 0;
    crankshaftParams.RodJournals = 4;
    crankshaftParams.TDC = Constants::pi / 4 - 2 * Constants::pi;
    m_iceEngine.getCrankshaft(0)->initialize(crankshaftParams);
    m_iceEngine.getCrankshaft(0)->setRodJournalAngle(0, 0);
    m_iceEngine.getCrankshaft(0)->setRodJournalAngle(1, -Constants::pi / 2);
    m_iceEngine.getCrankshaft(0)->setRodJournalAngle(2, -3 * Constants::pi / 2);
    m_iceEngine.getCrankshaft(0)->setRodJournalAngle(3, Constants::pi);

    ConnectingRod::Parameters crParams;
    crParams.CenterOfMass = 0;
    crParams.Crankshaft = m_iceEngine.getCrankshaft(0);
    crParams.Journal = 0;
    crParams.Length = units::distance(6.135, units::inch);
    crParams.Mass = units::mass(785, units::g);
    crParams.MomentOfInertia = (1 / 12.0) * crParams.Mass * crParams.Length * crParams.Length;

    crParams.Piston = m_iceEngine.getPiston(0);
    m_iceEngine.getConnectingRod(0)->initialize(crParams);

    crParams.Piston = m_iceEngine.getPiston(1);
    m_iceEngine.getConnectingRod(1)->initialize(crParams);

    crParams.Journal = 1;
    crParams.Piston = m_iceEngine.getPiston(2);
    m_iceEngine.getConnectingRod(2)->initialize(crParams);

    crParams.Piston = m_iceEngine.getPiston(3);
    m_iceEngine.getConnectingRod(3)->initialize(crParams);

    crParams.Journal = 2;
    crParams.Piston = m_iceEngine.getPiston(4);
    m_iceEngine.getConnectingRod(4)->initialize(crParams);

    crParams.Piston = m_iceEngine.getPiston(5);
    m_iceEngine.getConnectingRod(5)->initialize(crParams);

    crParams.Journal = 3;
    crParams.Piston = m_iceEngine.getPiston(6);
    m_iceEngine.getConnectingRod(6)->initialize(crParams);

    crParams.Piston = m_iceEngine.getPiston(7);
    m_iceEngine.getConnectingRod(7)->initialize(crParams);

    // Camshaft
    Camshaft *exhaustCamLeft = new Camshaft, *exhaustCamRight = new Camshaft;
    Camshaft *intakeCamLeft = new Camshaft, *intakeCamRight = new Camshaft;
    Function *camLift = new Function;
    Function *noLift = new Function;
    camLift->initialize(1, units::angle(20, units::deg));
    camLift->addSample(0.0, units::distance(500, units::thou));
    camLift->addSample(-units::angle(20, units::deg), units::distance(350, units::thou));
    camLift->addSample(units::angle(20, units::deg), units::distance(350, units::thou));
    camLift->addSample(-units::angle(40, units::deg), units::distance(100, units::thou));
    camLift->addSample(units::angle(40, units::deg), units::distance(100, units::thou));
    camLift->addSample(-units::angle(60, units::deg), units::distance(0, units::thou));
    camLift->addSample(units::angle(60, units::deg), units::distance(0, units::thou));

    Camshaft::Parameters camParams;
    camParams.Crankshaft = m_iceEngine.getCrankshaft(0);
    camParams.LobeProfile = camLift;
    camParams.Lobes = 4;
    exhaustCamRight->initialize(camParams);
    exhaustCamLeft->initialize(camParams);
    intakeCamLeft->initialize(camParams);
    intakeCamRight->initialize(camParams);

    // 1 8 4 3 6 5 7 2
    exhaustCamRight->setLobeCenterline(0, units::angle(360.0 - 109 / 2.0, units::deg));
    exhaustCamRight->setLobeCenterline(1, units::angle(360.0 - 109 / 2.0, units::deg) + 3 * units::angle(360, units::deg) / 4);
    exhaustCamRight->setLobeCenterline(2, units::angle(360.0 - 109 / 2.0, units::deg) + 5 * units::angle(360, units::deg) / 4);
    exhaustCamRight->setLobeCenterline(3, units::angle(360.0 - 109 / 2.0, units::deg) + 6 * units::angle(360, units::deg) / 4);
    intakeCamRight->setLobeCenterline(0, units::angle(360.0 + 109 / 2.0, units::deg));
    intakeCamRight->setLobeCenterline(1, units::angle(360.0 + 109 / 2.0, units::deg) + 3 * units::angle(360, units::deg) / 4);
    intakeCamRight->setLobeCenterline(2, units::angle(360.0 + 109 / 2.0, units::deg) + 5 * units::angle(360, units::deg) / 4);
    intakeCamRight->setLobeCenterline(3, units::angle(360.0 + 109 / 2.0, units::deg) + 6 * units::angle(360, units::deg) / 4);

    exhaustCamLeft->setLobeCenterline(0, units::angle(360.0 - 109 / 2.0, units::deg) + 7 * units::angle(360, units::deg) / 4);
    exhaustCamLeft->setLobeCenterline(1, units::angle(360.0 - 109 / 2.0, units::deg) + 2 * units::angle(360, units::deg) / 4);
    exhaustCamLeft->setLobeCenterline(2, units::angle(360.0 - 109 / 2.0, units::deg) + 4 * units::angle(360, units::deg) / 4);
    exhaustCamLeft->setLobeCenterline(3, units::angle(360.0 - 109 / 2.0, units::deg) + 1 * units::angle(360, units::deg) / 4);
    intakeCamLeft->setLobeCenterline(0, units::angle(360.0 + 109 / 2.0, units::deg) + 7 * units::angle(360, units::deg) / 4);
    intakeCamLeft->setLobeCenterline(1, units::angle(360.0 + 109 / 2.0, units::deg) + 2 * units::angle(360, units::deg) / 4);
    intakeCamLeft->setLobeCenterline(2, units::angle(360.0 + 109 / 2.0, units::deg) + 4 * units::angle(360, units::deg) / 4);
    intakeCamLeft->setLobeCenterline(3, units::angle(360.0 + 109 / 2.0, units::deg) + 1 * units::angle(360, units::deg) / 4);

    Function *flow = new Function;
    flow->initialize(1, units::distance(100, units::thou));
    flow->addSample(units::distance(0, units::thou), 0.0);
    flow->addSample(units::distance(100, units::thou), 0.001 * 2);
    flow->addSample(units::distance(200, units::thou), 0.002 * 2);
    flow->addSample(units::distance(300, units::thou), 0.003 * 2);
    flow->addSample(units::distance(400, units::thou), 0.004 * 2);
    flow->addSample(units::distance(500, units::thou), 0.005 * 2);

    CylinderHead::Parameters chParams;
    chParams.IntakePortFlow = chParams.ExhaustPortFlow = flow;

    chParams.IntakeCam = intakeCamLeft;
    chParams.ExhaustCam = exhaustCamLeft;
    chParams.Bank = m_iceEngine.getCylinderBank(0);
    m_iceEngine.getHead(0)->initialize(chParams);

    chParams.IntakeCam = intakeCamRight;
    chParams.ExhaustCam = exhaustCamRight;
    chParams.Bank = m_iceEngine.getCylinderBank(1);
    m_iceEngine.getHead(1)->initialize(chParams);

    m_simulator.synthesize(&m_iceEngine);
    createObjects(&m_iceEngine);

    m_simulator.placeAndInitialize();

    m_uiManager.initialize(this);
    CylinderPressureGauge *gauge = m_uiManager.getRoot()->addElement<CylinderPressureGauge>();
    gauge->m_bounds = Bounds(300.0f, 500.0f, { 0.0f, 0.0f });
    gauge->setLocalPosition(Point((float)m_engine.GetScreenWidth(), 0 ) + Point(-10.0f, 10.0f), Bounds::br);
    gauge->m_simulator = &m_simulator;
}

void EngineSimApplication::process(float dt) {
    m_simulator.m_steps = 100;
    m_simulator.update((1 / 60.0) / 10);
}

void EngineSimApplication::render() {
    SimulationObject::ViewParameters view;
    view.Layer0 = 0;
    view.Layer1 = 3;

    for (SimulationObject *object : m_objects) {
        object->generateGeometry();
        object->render(&view);
    }

    std::stringstream ss;
    ss << std::lroundf(m_simulator.getAverageProcessingTime()) << " us    \n";
    ss << std::lroundf(m_iceEngine.getRpm()) << " RPM     \n";
    ss << std::lroundf(m_engine.GetAverageFramerate()) << " FPS       \n";
    m_textRenderer.RenderText(
        ss.str(),
        50 - m_engine.GetScreenWidth() / 2.0,
        50 + 64 + 64 - m_engine.GetScreenHeight() / 2.0,
        64
    );

    m_uiManager.render();
}

float EngineSimApplication::pixelsToUnits(float pixels) const {
    const float f = m_displayHeight / m_engine.GetGameWindow()->GetGameHeight();
    return pixels * f;
}

float EngineSimApplication::unitsToPixels(float units) const {
    const float f = m_engine.GetGameWindow()->GetGameHeight() / m_displayHeight;
    return units * f;
}

void EngineSimApplication::run() {
    while (true) {
        if (m_engine.ProcessKeyDown(ysKey::Code::Escape)) {
            break;
        }

        m_engine.StartFrame();
        if (!m_engine.IsOpen()) break;

        updateScreenSizeStability();

        if (m_engine.ProcessKeyDown(ysKey::Code::Space) &&
            m_engine.GetGameWindow()->IsActive()) {
            m_paused = !m_paused;
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::F)) {
            m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Fullscreen);
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::V) &&
            m_engine.GetGameWindow()->IsActive()) {
            if (!isRecording() && readyToRecord()) {
                startRecording();
            }
            else if (isRecording()) {
                stopRecording();
            }
        }

        if (isRecording() && !readyToRecord()) {
            stopRecording();
        }

        if (!m_paused || m_engine.ProcessKeyDown(ysKey::Code::Right)) {
            process(1 / 60.0f);
        }

        m_uiManager.update(1 / 60.0f);

        renderScene();

        m_engine.EndFrame();

        if (isRecording()) {
            recordFrame();
        }
    }

    if (isRecording()) {
        stopRecording();
    }
}

void EngineSimApplication::destroy() {
    m_shaderSet.Destroy();

    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryVertexBuffer);
    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryIndexBuffer);

    m_assetManager.Destroy();
    m_engine.Destroy();
}

void EngineSimApplication::drawGenerated(const GeometryGenerator::GeometryIndices &indices, int layer) {
    m_engine.DrawGeneric(
        m_shaders.GetRegularFlags(),
        m_geometryIndexBuffer,
        m_geometryVertexBuffer,
        sizeof(dbasic::Vertex),
        indices.BaseIndex,
        indices.BaseVertex,
        indices.FaceCount,
        false,
        layer);
}

void EngineSimApplication::createObjects(Engine *engine) {
    for (int i = 0; i < engine->getCylinderCount(); ++i) {
        ConnectingRodObject *rodObject = new ConnectingRodObject;
        rodObject->initialize(this);
        rodObject->m_connectingRod = engine->getConnectingRod(i);
        m_objects.push_back(rodObject);

        PistonObject *pistonObject = new PistonObject;
        pistonObject->initialize(this);
        pistonObject->m_piston = engine->getPiston(i);
        m_objects.push_back(pistonObject);
    }

    for (int i = 0; i < engine->getCrankshaftCount(); ++i) {
        CrankshaftObject *crankshaftObject = new CrankshaftObject;
        crankshaftObject->initialize(this);
        crankshaftObject->m_crankshaft = engine->getCrankshaft(i);
        m_objects.push_back(crankshaftObject);
    }

    for (int i = 0; i < engine->getCylinderBankCount(); ++i) {
        CylinderBankObject *cbObject = new CylinderBankObject;
        cbObject->initialize(this);
        cbObject->m_bank = engine->getCylinderBank(i);
        m_objects.push_back(cbObject);

        CylinderHeadObject *chObject = new CylinderHeadObject;
        chObject->initialize(this);
        chObject->m_head = engine->getHead(i);
        m_objects.push_back(chObject);
    }
}

void EngineSimApplication::renderScene() {
    m_shaders.SetClearColor(ysColor::linearToSrgb(m_shadow));

    const int screenWidth = m_engine.GetGameWindow()->GetGameWidth();
    const int screenHeight = m_engine.GetGameWindow()->GetGameHeight();

    m_shaders.SetScreenDimensions((float)screenWidth, (float)screenHeight);

    m_shaders.SetCameraPosition(m_cameraPosition);
    m_shaders.SetCameraTarget(m_cameraTarget);
    m_shaders.SetCameraUp(m_cameraUp);
    m_shaders.CalculateUiCamera();

    if (screenWidth > 0 && screenHeight > 0) {
        const float aspectRatio = screenWidth / (float)screenHeight;
        m_shaders.SetProjection(ysMath::Transpose(
            ysMath::OrthographicProjection(
                aspectRatio * m_displayHeight,
                m_displayHeight,
                0.001f,
                500.0f)));
    }

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

void EngineSimApplication::startRecording() {
    m_recording = true;

#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    atg_dtv::Encoder::VideoSettings settings{};

    // Output filename
    settings.fname = "../workspace/video_capture/engine_sim_video_capture.mp4";
    settings.inputWidth = m_engine.GetScreenWidth();
    settings.inputHeight = m_engine.GetScreenHeight();
    settings.width = settings.inputWidth;
    settings.height = settings.inputHeight;
    settings.hardwareEncoding = true;
    settings.inputAlpha = true;
    settings.bitRate = 40000000;

    m_encoder.run(settings, 2);
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

void EngineSimApplication::updateScreenSizeStability() {
    m_screenResolution[m_screenResolutionIndex][0] = m_engine.GetScreenWidth();
    m_screenResolution[m_screenResolutionIndex][1] = m_engine.GetScreenHeight();

    m_screenResolutionIndex = (m_screenResolutionIndex + 1) % ScreenResolutionHistoryLength;
}

bool EngineSimApplication::readyToRecord() {
    const int w = m_screenResolution[0][0];
    const int h = m_screenResolution[0][1];

    if (w <= 0 && h <= 0) return false;
    if ((w % 2) != 0 || (h % 2) != 0) return false;

    for (int i = 1; i < ScreenResolutionHistoryLength; ++i) {
        if (m_screenResolution[i][0] != w) return false;
        if (m_screenResolution[i][1] != h) return false;
    }

    return true;
}

void EngineSimApplication::stopRecording() {
    m_recording = false;

#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    m_encoder.commit();
    m_encoder.stop();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}

void EngineSimApplication::recordFrame() {
#ifdef ATG_ENGINE_SIM_VIDEO_CAPTURE
    atg_dtv::Frame *frame = m_encoder.newFrame(true);
    if (frame != nullptr && m_encoder.getError() == atg_dtv::Encoder::Error::None) {
        m_engine.GetDevice()->ReadRenderTarget(m_engine.GetScreenRenderTarget(), frame->m_rgb);
    }

    m_encoder.submitFrame();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}
