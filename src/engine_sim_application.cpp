#include "../include/engine_sim_application.h"

#include "../include/piston_object.h"
#include "../include/connecting_rod_object.h"
#include "../include/constants.h"
#include "../include/units.h"
#include "../include/crankshaft_object.h"
#include "../include/cylinder_bank_object.h"
#include "../include/cylinder_head_object.h"
#include "../include/ui_button.h"
#include "../include/combustion_chamber_object.h"
#include "../include/csv_io.h"
#include "../include/exhaust_system.h"
#include "../include/feedback_comb_filter.h"

#include <stdlib.h>
#include <sstream>

EngineSimApplication::EngineSimApplication() {
    m_assetPath = "";

    m_geometryVertexBuffer = nullptr;
    m_geometryIndexBuffer = nullptr;

    m_paused = false;
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
    m_pink = ysColor::srgbiToLinear(0xF394BE);
    m_red = ysColor::srgbiToLinear(0xEE4445);
    m_orange = ysColor::srgbiToLinear(0xF4802A);
    m_yellow = ysColor::srgbiToLinear(0xFDBD2E);
    m_blue = ysColor::srgbiToLinear(0x77CEE0);
    m_green = ysColor::srgbiToLinear(0xBDD869);

    m_displayHeight = (float)units::distance(2.0, units::foot);
    m_outputAudioBuffer = nullptr;
    m_audioSource = nullptr;
    m_lastAudioSample = 0;

    m_torque = 0;
    m_dynoSpeed = 0;

    m_averageAudioSyncedTimeDelta = 0;
    m_audioSyncedTimeDelta = 0;

    m_engineView = nullptr;
    m_gaugeCluster = nullptr;
    m_temperatureGauge = nullptr;

    m_oscillatorDataLeft = "../assets/oscillator_data_left.csv";
    m_oscillatorDataRight = "../assets/oscillator_data_right.csv";
    m_oscillatorsLeft = m_oscillatorsRight = nullptr;
    m_oscillatorCountLeft = m_oscillatorCountRight = 0;
    updateOscillatorData();
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
    m_shaders.Initialize(
        &m_shaderSet,
        m_engine.GetScreenRenderTarget(),
        m_engine.GetDefaultShaderProgram(),
        m_engine.GetDefaultInputLayout());
    m_engine.InitializeConsoleShaders(&m_shaderSet);
    m_engine.SetShaderSet(&m_shaderSet);

    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));

    m_assetManager.SetEngine(&m_engine);

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
    engineParams.ExhaustSystemCount = 2;
    engineParams.IntakeCount = 1;
    m_iceEngine.initialize(engineParams);

    Piston::Parameters pistonParams;
    pistonParams.CompressionHeight = units::distance(1.640, units::inch);
    pistonParams.Displacement = 0;
    pistonParams.WristPinPosition = 0;
    pistonParams.Mass = units::mass(880, units::g);

    pistonParams.CylinderIndex = 0;
    pistonParams.Bank = m_iceEngine.getCylinderBank(1);
    pistonParams.Rod = m_iceEngine.getConnectingRod(0);
    m_iceEngine.getPiston(0)->initialize(pistonParams);

    pistonParams.Bank = m_iceEngine.getCylinderBank(0);
    pistonParams.Rod = m_iceEngine.getConnectingRod(1);
    m_iceEngine.getPiston(1)->initialize(pistonParams);

    pistonParams.CylinderIndex = 1;
    pistonParams.Bank = m_iceEngine.getCylinderBank(1);
    pistonParams.Rod = m_iceEngine.getConnectingRod(2);
    m_iceEngine.getPiston(2)->initialize(pistonParams);

    pistonParams.Bank = m_iceEngine.getCylinderBank(0);
    pistonParams.Rod = m_iceEngine.getConnectingRod(3);
    m_iceEngine.getPiston(3)->initialize(pistonParams);

    pistonParams.CylinderIndex = 2;
    pistonParams.Bank = m_iceEngine.getCylinderBank(1);
    pistonParams.Rod = m_iceEngine.getConnectingRod(4);
    m_iceEngine.getPiston(4)->initialize(pistonParams);

    pistonParams.Bank = m_iceEngine.getCylinderBank(0);
    pistonParams.Rod = m_iceEngine.getConnectingRod(5);
    m_iceEngine.getPiston(5)->initialize(pistonParams);

    pistonParams.CylinderIndex = 3;
    pistonParams.Bank = m_iceEngine.getCylinderBank(1);
    pistonParams.Rod = m_iceEngine.getConnectingRod(6);
    m_iceEngine.getPiston(6)->initialize(pistonParams);

    pistonParams.Bank = m_iceEngine.getCylinderBank(0);
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
    const double I_crank = (1 / 2.0) * crankshaftParams.Mass * crank_r * crank_r;
    const double I_flywheel =
        (1 / 2.0) * crankshaftParams.FlywheelMass * flywheel_r * flywheel_r;

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
    Function *camLift0 = new Function;
    camLift0->initialize(1, units::angle(20, units::deg));
    camLift0->addSample(0.0, units::distance(300, units::thou));
    camLift0->addSample(-units::angle(20, units::deg), units::distance(150, units::thou));
    camLift0->addSample(units::angle(20, units::deg), units::distance(150, units::thou));
    camLift0->addSample(-units::angle(40, units::deg), units::distance(10, units::thou));
    camLift0->addSample(units::angle(40, units::deg), units::distance(10, units::thou));
    camLift0->addSample(-units::angle(60, units::deg), units::distance(0, units::thou));
    camLift0->addSample(units::angle(60, units::deg), units::distance(0, units::thou));
    camLift0->addSample(-units::angle(80, units::deg), units::distance(0, units::thou));
    camLift0->addSample(units::angle(80, units::deg), units::distance(0, units::thou));

    Function *camLift1 = new Function;
    camLift1->initialize(1, units::angle(20, units::deg));
    camLift1->addSample(0.0, units::distance(100, units::thou));
    camLift1->addSample(-units::angle(20, units::deg), units::distance(100, units::thou));
    camLift1->addSample(units::angle(20, units::deg), units::distance(0, units::thou));
    camLift1->addSample(-units::angle(40, units::deg), units::distance(0, units::thou));
    camLift1->addSample(units::angle(40, units::deg), units::distance(0, units::thou));
    camLift1->addSample(-units::angle(60, units::deg), units::distance(0, units::thou));
    camLift1->addSample(units::angle(60, units::deg), units::distance(0, units::thou));
    camLift1->addSample(-units::angle(80, units::deg), units::distance(0, units::thou));
    camLift1->addSample(units::angle(80, units::deg), units::distance(0, units::thou));

    Camshaft::Parameters camParams;
    camParams.Crankshaft = m_iceEngine.getCrankshaft(0);
    camParams.Lobes = 4;

    camParams.LobeProfile = camLift0;
    exhaustCamRight->initialize(camParams);
    exhaustCamLeft->initialize(camParams);
    camParams.LobeProfile = camLift0;
    intakeCamLeft->initialize(camParams);
    intakeCamRight->initialize(camParams);

    // 1 8 4 3 6 5 7 2
    const double lobeSeparation = 109;
    exhaustCamRight->setLobeCenterline(0, units::angle(360 - lobeSeparation, units::deg));
    exhaustCamRight->setLobeCenterline(1, units::angle(360 - lobeSeparation, units::deg) + 3 * units::angle(360, units::deg) / 4);
    exhaustCamRight->setLobeCenterline(2, units::angle(360 - lobeSeparation, units::deg) + 5 * units::angle(360, units::deg) / 4);
    exhaustCamRight->setLobeCenterline(3, units::angle(360 - lobeSeparation, units::deg) + 6 * units::angle(360, units::deg) / 4);
    intakeCamRight->setLobeCenterline(0, units::angle(360 + lobeSeparation, units::deg));
    intakeCamRight->setLobeCenterline(1, units::angle(360 + lobeSeparation, units::deg) + 3 * units::angle(360, units::deg) / 4);
    intakeCamRight->setLobeCenterline(2, units::angle(360 + lobeSeparation, units::deg) + 5 * units::angle(360, units::deg) / 4);
    intakeCamRight->setLobeCenterline(3, units::angle(360 + lobeSeparation, units::deg) + 6 * units::angle(360, units::deg) / 4);

    exhaustCamLeft->setLobeCenterline(0, units::angle(360 - lobeSeparation, units::deg) + 7 * units::angle(360, units::deg) / 4);
    exhaustCamLeft->setLobeCenterline(1, units::angle(360 - lobeSeparation, units::deg) + 2 * units::angle(360, units::deg) / 4);
    exhaustCamLeft->setLobeCenterline(2, units::angle(360 - lobeSeparation, units::deg) + 4 * units::angle(360, units::deg) / 4);
    exhaustCamLeft->setLobeCenterline(3, units::angle(360 - lobeSeparation, units::deg) + 1 * units::angle(360, units::deg) / 4);
    intakeCamLeft->setLobeCenterline(0, units::angle(360 + lobeSeparation, units::deg) + 7 * units::angle(360, units::deg) / 4);
    intakeCamLeft->setLobeCenterline(1, units::angle(360 + lobeSeparation, units::deg) + 2 * units::angle(360, units::deg) / 4);
    intakeCamLeft->setLobeCenterline(2, units::angle(360 + lobeSeparation, units::deg) + 4 * units::angle(360, units::deg) / 4);
    intakeCamLeft->setLobeCenterline(3, units::angle(360 + lobeSeparation, units::deg) + 1 * units::angle(360, units::deg) / 4);

    // n = PV / RT
    const double flow_t = units::celcius(25.0);
    const double P = units::pressure(1.0, units::psi);
    const double v = units::flow(1, units::cfm);
    const double n_flow = 1.0 * (P * v) / (Constants::R * flow_t);

    Function *flow = new Function;
    flow->initialize(1, units::distance(100, units::thou));
    flow->addSample(units::distance(0, units::thou), 0.0);
    flow->addSample(units::distance(100, units::thou), n_flow * 70);
    flow->addSample(units::distance(200, units::thou), n_flow * 133);
    flow->addSample(units::distance(300, units::thou), n_flow * 180);
    flow->addSample(units::distance(400, units::thou), n_flow * 215);
    flow->addSample(units::distance(500, units::thou), n_flow * 237);

    Function *exhaustFlow = new Function;
    exhaustFlow->initialize(1, units::distance(100, units::thou));
    exhaustFlow->addSample(units::distance(0, units::thou), 0.0);
    exhaustFlow->addSample(units::distance(100, units::thou), n_flow * 70);
    exhaustFlow->addSample(units::distance(200, units::thou), n_flow * 133);
    exhaustFlow->addSample(units::distance(300, units::thou), n_flow * 180);
    exhaustFlow->addSample(units::distance(400, units::thou), n_flow * 215);
    exhaustFlow->addSample(units::distance(500, units::thou), n_flow * 237);

    CylinderHead::Parameters chParams;
    chParams.IntakePortFlow = flow;
    chParams.ExhaustPortFlow = exhaustFlow;
    chParams.CombustionChamberVolume = units::volume(118.0, units::cc);

    chParams.IntakeCam = intakeCamLeft;
    chParams.ExhaustCam = exhaustCamLeft;
    chParams.Bank = m_iceEngine.getCylinderBank(0);
    m_iceEngine.getHead(0)->initialize(chParams);

    chParams.IntakeCam = intakeCamRight;
    chParams.ExhaustCam = exhaustCamRight;
    chParams.Bank = m_iceEngine.getCylinderBank(1);
    m_iceEngine.getHead(1)->initialize(chParams);

    Intake::Parameters inParams;
    inParams.inputFlowK = n_flow * 800;
    inParams.volume = units::volume(5000.0, units::cc);
    m_iceEngine.getIntake(0)->initialize(inParams);

    ExhaustSystem::Parameters esParams;
    esParams.flowK = n_flow * 2000;
    esParams.volume = units::volume(10.0, units::L);
    m_iceEngine.getExhaustSystem(0)->initialize(esParams); 
    m_iceEngine.getExhaustSystem(1)->initialize(esParams);

    m_iceEngine.getHead(0)->setAllExhaustSystems(m_iceEngine.getExhaustSystem(0));
    m_iceEngine.getHead(1)->setAllExhaustSystems(m_iceEngine.getExhaustSystem(1));

    m_iceEngine.getHead(0)->setAllIntakes(m_iceEngine.getIntake(0));
    m_iceEngine.getHead(1)->setAllIntakes(m_iceEngine.getIntake(0));

    Function *timingCurve = new Function;
    timingCurve->initialize(1, units::rpm(1000));
    timingCurve->addSample(units::rpm(0), units::angle(12, units::deg));
    timingCurve->addSample(units::rpm(1000), units::angle(12, units::deg));
    timingCurve->addSample(units::rpm(2000), units::angle(20, units::deg));
    timingCurve->addSample(units::rpm(3000), units::angle(30, units::deg));
    timingCurve->addSample(units::rpm(4000), units::angle(38, units::deg));
    timingCurve->addSample(units::rpm(5000), units::angle(38, units::deg));
    timingCurve->addSample(units::rpm(6000), units::angle(38, units::deg));

    IgnitionModule::Parameters imParams;
    imParams.Crankshaft = m_iceEngine.getCrankshaft(0);
    imParams.CylinderCount = 8;
    imParams.TimingCurve = timingCurve;
    m_iceEngine.getIgnitionModule()->initialize(imParams);
    const double cycle = units::angle(2 * 360.0, units::deg);
    m_iceEngine.getIgnitionModule()->setFiringOrder(1 - 1, (0 / 8.0) * cycle);
    m_iceEngine.getIgnitionModule()->setFiringOrder(8 - 1, (1 / 8.0) * cycle);
    m_iceEngine.getIgnitionModule()->setFiringOrder(4 - 1, (2 / 8.0) * cycle);
    m_iceEngine.getIgnitionModule()->setFiringOrder(3 - 1, (3 / 8.0) * cycle);
    m_iceEngine.getIgnitionModule()->setFiringOrder(6 - 1, (4 / 8.0) * cycle);
    m_iceEngine.getIgnitionModule()->setFiringOrder(5 - 1, (5 / 8.0) * cycle);
    m_iceEngine.getIgnitionModule()->setFiringOrder(7 - 1, (6 / 8.0) * cycle);
    m_iceEngine.getIgnitionModule()->setFiringOrder(2 - 1, (7 / 8.0) * cycle);

    m_simulator.synthesize(&m_iceEngine, EngineSimulator::SystemType::NsvOptimized);
    createObjects(&m_iceEngine, &m_simulator);

    m_simulator.placeAndInitialize();

    m_dyno.initialize(m_simulator.getCrankshaftLoad(0));

    m_uiManager.initialize(this);

    m_engineView = m_uiManager.getRoot()->addElement<EngineView>();
    m_gaugeCluster = m_uiManager.getRoot()->addElement<GaugeCluster>();
    m_gaugeCluster->m_simulator = &m_simulator;

    m_temperatureGauge = m_uiManager.getRoot()->addElement<CylinderTemperatureGauge>();
    m_temperatureGauge->m_simulator = &m_simulator;

    m_oscilloscope = m_uiManager.getRoot()->addElement<Oscilloscope>();
    m_oscilloscope->setBufferSize(44100 / 30);
    m_oscilloscope->m_bounds = Bounds(200.0f, 200.0f, { 0.0f, 0.0f });
    m_oscilloscope->setLocalPosition({ 50.0f, 900.0f });
    m_oscilloscope->m_xMin = 0.0f;
    m_oscilloscope->m_xMax = cycle; //44100 / 60.0;
    m_oscilloscope->m_yMin = 0.0; // -units::pressure(1.0, units::psi);
    m_oscilloscope->m_yMax = 1.0; // units::pressure(1.0, units::psi);
    m_oscilloscope->m_lineWidth = 1.0f;
    m_oscilloscope->m_drawReverse = true;

    m_audioBuffer.initialize(44100, 44100 * 5);
    m_audioBuffer.m_writePointer = 44100 * 0.1;

    ysAudioParameters params;
    params.m_bitsPerSample = 16;
    params.m_channelCount = 1;
    params.m_sampleRate = 44100;
    m_outputAudioBuffer =
        m_engine.GetAudioDevice()->CreateBuffer(&params, 44100 * 5);

    m_audioSource = m_engine.GetAudioDevice()->CreateSource(m_outputAudioBuffer);
    m_audioSource->SetMode(ysAudioSource::Mode::Loop);
    m_audioSource->SetPan(0.0f);
    m_audioSource->SetVolume(1.0f);

    m_audioImpulseResponse.initialize(16);
    m_audioImpulseResponse.getImpulseResponse()[0] = 1;
    m_audioImpulseResponse.getImpulseResponse()[4] = 0.5;
    m_audioImpulseResponse.getImpulseResponse()[8] = 1;
}

void EngineSimApplication::process(float frame_dt) {
    static double volume = 0;
    static double flow[2] = { 0, 0 };
    static double smoothFlow[2] = { 0, 0 };
    static double flowDerivative[2] = { 0, 0 };
    static double flowDC[2] = { 0.5, 0.5 };
    static double whiteNoise = 0;
    static double continuousSampleDelta = 44100 / 60.0;

    static FeedbackCombFilter *fcf = nullptr;
    if (fcf == nullptr) {
        fcf = new FeedbackCombFilter[2];

        fcf[0].initialize(100);
        fcf[0].a_M = 0.742;

        fcf[1].initialize(50);
        fcf[1].a_M = 0.733;
    }

    const int audioPosition = m_audioSource->GetCurrentPosition();
    continuousSampleDelta =
        0.99 * continuousSampleDelta + 0.01 * m_audioBuffer.offsetDelta(m_lastAudioSample, audioPosition);
    const int sampleDelta = m_audioBuffer.offsetDelta(m_lastAudioSample, audioPosition);

    double speed = 1.0;
    if (m_engine.IsKeyDown(ysKey::Code::Control)) {
        speed = 1 / 30.0;
    }

    const double rt_dt = m_audioBuffer.offsetToTime(sampleDelta);
    const double dt = rt_dt * speed;

    m_audioSyncedTimeDelta = rt_dt;
    m_averageAudioSyncedTimeDelta =
        m_audioSyncedTimeDelta * 0.01 + m_averageAudioSyncedTimeDelta * 0.99;

    if (sampleDelta == 0) return;

    m_lastAudioSample = audioPosition;

    m_simulator.m_steps = std::lround(250 * (dt * 60));
    m_simulator.start();

    m_dynoSpeed = std::fmodf(
        (float)(m_dynoSpeed + units::rpm(300) * dt),
        (float)units::rpm(6000));
    m_dynoSpeed = units::rpm(500);

    //m_oscilloscope->addDataPoint(
    //    -m_iceEngine.getCrankshaft(0)->m_body.v_theta,
    //    m_torque);

    Function exhaustLeft;
    exhaustLeft.initialize(m_simulator.m_steps, (rt_dt / m_simulator.m_steps));
    Function exhaustRight;
    exhaustRight.initialize(m_simulator.m_steps, (rt_dt / m_simulator.m_steps));
    double currentFlowDC[2] = { 0, 0 };

    do {
        const double t =
            m_simulator.getCurrentIteration() * (rt_dt / m_simulator.m_steps);

        // 1 8 4 3 5 7 2
        //double flowLeft = m_iceEngine.getExhaustSystem(0)->m_flow;
        //double flowRight = m_iceEngine.getExhaustSystem(1)->m_flow;

        double flowLeft = 0, flowRight = 0;
        for (int i = 0; i < 1; ++i) {
            if (i % 2 == 0) {
                flowLeft += m_simulator.getCombustionChamber(i)->m_exhaustFlow;
            }
            else {
                flowRight += m_simulator.getCombustionChamber(i)->m_exhaustFlow;
            }
        }

        const double flowRateLeft = flowLeft / (dt / m_simulator.m_steps);
        const double flowRateRight = flowRight / (dt / m_simulator.m_steps);

        if (m_simulator.getCurrentIteration() != 0) {
            flowDerivative[0] = (flowRateLeft - flow[0]) / (dt / m_simulator.m_steps);
            flowDerivative[1] = (flowRateRight - flow[1]) / (dt / m_simulator.m_steps);
        }

        flow[0] = flowRateLeft;
        flow[1] = flowRateRight;

        const double flowDerivativeLeft = std::pow(std::abs(flowDerivative[0]), 0.5) * (flowDerivative[0] < 0 ? -1 : 1);
        const double flowDerivativeRight = std::pow(std::abs(flowDerivative[1]), 0.5) * (flowDerivative[1] < 0 ? -1 : 1);

        if (m_simulator.getCurrentIteration() != 0) {
            m_oscilloscope->addDataPoint(
                m_iceEngine.getCrankshaft(0)->getCycleAngle(),
                //(flowRateLeft + flowRateRight));
                (flowDerivativeLeft + flowDerivativeRight) * 0.01);
        }
        
        exhaustLeft.addSample(
            t,
            flowDerivativeLeft * 5000);
        exhaustRight.addSample(
            t,
            flowDerivativeRight * 5000);

        currentFlowDC[0] += flowRateLeft;
        currentFlowDC[1] += flowRateRight;
    } while (m_simulator.simulateStep(dt));

    double pulse = 0;
    if (m_engine.ProcessKeyDown(ysKey::Code::T)) {
        pulse = 15;
    }

    for (int i = 0; i < sampleDelta; ++i) {
        const double newFlowLeft =
            0.0 * flow[0] + 1.0 * (exhaustLeft.sampleTriangle(m_audioBuffer.offsetToTime(i)));
        const double newFlowRight =
            0.0 * flow[1] + 1.0 * (exhaustRight.sampleTriangle(m_audioBuffer.offsetToTime(i)));

        const double newSmoothFlowLeft = (0.9 * smoothFlow[0] / 1.2 + 0.1 * newFlowLeft) * 1.2;
        const double newSmoothFlowRight = (0.9 * smoothFlow[1] / 1.2 + 0.1 * newFlowRight) * 1.2;

        flowDerivative[0] = newFlowLeft;
        flowDerivative[1] = newFlowRight;

        smoothFlow[0] = newSmoothFlowLeft;
        smoothFlow[1] = newSmoothFlowRight;

        flowDC[0] = 0.999 * flowDC[0] + 0.001 * flow[0];
        flowDC[1] = 0.999 * flowDC[1] + 0.001 * flow[1];

        whiteNoise = 0.9 * whiteNoise + 0.1 * (((double)rand() / RAND_MAX) - 0.5) * 12.0;

        double sample = 0;
        for (int j = 0; j < m_oscillatorCountLeft; ++j) {
            Oscillator &osc = m_oscillatorsLeft[j];
            const double ks_sqrt = osc.freq * 2 * Constants::pi;
            const double ks = ks_sqrt * ks_sqrt;
            const double sgn = osc.disp < 0
                ? -1.0
                : 1.0;

            // Spring energy = 0.5 * k * x^2
            // Kinetic energy = 0.5 * m * v^2
            // k * pulse * pulse = m * v * v
            // => pulse * pulse * k = = v * v
            // => pulse * sqrt(k) = v

            osc.vel +=
                ((sgn * -osc.disp * osc.disp * ks - osc.k_d * osc.vel)) * m_audioBuffer.offsetToTime(1);
            if (i == 0) {
                osc.vel += ks_sqrt * pulse;
            }

            osc.vel += ks_sqrt * std::abs(0.001 * (flowDerivative[0]));
            osc.vel = std::fmin(std::fmax(osc.vel, -10000), 10000);

            osc.disp += osc.vel * m_audioBuffer.offsetToTime(1);

            sample += (1.0 / m_oscillatorCountLeft) * osc.s * osc.disp;
        }

        for (int j = 0; j < m_oscillatorCountRight; ++j) {
            Oscillator &osc = m_oscillatorsRight[j];
            const double ks_sqrt = osc.freq * 2 * Constants::pi;
            const double ks = ks_sqrt * ks_sqrt;
            const double sgn = osc.disp < 0
                ? -1.0
                : 1.0;

            osc.vel +=
                ((sgn * -osc.disp * osc.disp * ks - osc.k_d * osc.vel)) * m_audioBuffer.offsetToTime(1);
            if (i == 0) {
                osc.vel += ks_sqrt * pulse;
            }

            osc.vel += ks_sqrt * std::abs(0.001 * (flowDerivative[1]));
            osc.vel = std::fmin(std::fmax(osc.vel, -10000), 10000);

            osc.disp += osc.vel * m_audioBuffer.offsetToTime(1);
            osc.disp = std::fmin(osc.disp, 1.0);

            sample += (1.0 / m_oscillatorCountRight) * osc.s * osc.disp;
        }

        //sample = 0.00001 * flowDerivative;

        //sample += ((smoothFlow - flowDC) * 0.15);
        //sample = ((flow[0] - flowDC[0]) * 0.1);
        //sample *= 0.2;
        //sample += (flowDerivative[0] + flowDerivative[1]) * 0.0001;
        //sample = (flowDerivative[0] + flowDerivative[1]) * 0.1;
        //sample += (flow[0] + flow[1]);
        //sample += 0.0001 * flowDerivative;
        //sample = m_audioImpulseResponse.f(sample);
        //sample = 0.9 * sample + 0.1 * sample * whiteNoise;
        //sample += whiteNoise * (flow[0] + flow[1]);

        //sample += flow[0] + flow[1];

        sample *= 0.004;
        sample *= 0.5;
        sample *= 0.02;

        //sample = std::pow(std::abs(sample), 0.5) * (sample < 0 ? -1 : 1);

        sample = fcf[0].f(sample) * 0.5 + fcf[1].f(sample) * 0.5;

        if (sample > 0.5) sample -= (sample - 0.5) * 0.95;
        else if (sample < -0.5) sample -= -(-0.5 - sample) * 0.95;

        //sample = std::erf(sample) * 0.8;

        //m_oscilloscope->addDataPoint(
        //    i,
        //    sample);

        if (sample > volume) {
            volume = sample;
        }

        volume = volume * 0.999;

        m_audioBuffer.writeSample(std::lround(sample * 60000), m_audioBuffer.m_writePointer, i);
    }

    SampleOffset size0, size1;
    void *data0, *data1;

    m_audioSource->LockBufferSegment(
            m_audioBuffer.m_writePointer, sampleDelta, &data0, &size0, &data1, &size1);

    m_audioBuffer.copyBuffer(
            reinterpret_cast<int16_t *>(data0), m_audioBuffer.m_writePointer, size0);
    m_audioBuffer.copyBuffer(
            reinterpret_cast<int16_t *>(data1),
            m_audioBuffer.getBufferIndex(m_audioBuffer.m_writePointer, size0),
            size1);

    m_audioSource->UnlockBufferSegments(data0, size0, data1, size1);
    m_audioBuffer.commitBlock(sampleDelta);

    exhaustLeft.destroy();
    exhaustRight.destroy();
}

void EngineSimApplication::render() {
    SimulationObject::ViewParameters view;
    view.Layer0 = 0;
    view.Layer1 = 3;

    for (SimulationObject *object : m_objects) {
        object->generateGeometry();
        object->render(&view);
    }

    m_torque = m_torque * 0.95 + 0.05 * units::convert(m_simulator.getCrankshaftLoad(0)->getDynoTorque(), units::ft_lb);

    double burnedFuel = 0;
    for (int i = 0; i < m_iceEngine.getCylinderCount(); ++i) {
        burnedFuel += m_simulator.getCombustionChamber(i)->m_nBurntFuel;
    }

    std::stringstream ss;
    /*ss << m_averageAudioSyncedTimeDelta / (1 / 60.0) << "\n";
    ss << "Dyno: " << m_torque << " ft-lb // " << m_torque * m_iceEngine.getRpm() / 5252.0 << " hp    \n";
    ss << units::convert(burnedFuel, units::g) << " g \n";
    ss << std::lroundf(m_simulator.getAverageProcessingTime()) << " us    \n";
    ss << std::lroundf(m_iceEngine.getRpm()) << " RPM     \n";
    ss << std::lroundf(m_engine.GetAverageFramerate()) << " FPS       \n";*/
    ss << units::convert(m_iceEngine.getIntake(0)->m_system.pressure(), units::psi) << "\n";
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

void EngineSimApplication::updateOscillatorData() {
    if (m_oscillatorsLeft != nullptr) delete[] m_oscillatorsLeft;
    if (m_oscillatorsRight != nullptr) delete[] m_oscillatorsRight;

    atg_csv::CsvData csvData;
    csvData.initialize(4);
    atg_csv::CsvData::ErrorCode err = csvData.loadCsv(m_oscillatorDataLeft.c_str());
    if (err != atg_csv::CsvData::ErrorCode::Success) {
        m_oscillatorCountLeft = 0;
        m_oscillatorsLeft = nullptr;

        return;
    }

    m_oscillatorsLeft = new Oscillator[(size_t)csvData.m_rows - 1];
    m_oscillatorCountLeft = csvData.m_rows - 1;

    for (int i = 1; i < csvData.m_rows; ++i) {
        Oscillator &osc = m_oscillatorsLeft[(size_t)i - 1];
        osc.freq = strtod(csvData.readEntry(i, 0), nullptr);
        osc.k_d = strtod(csvData.readEntry(i, 1), nullptr);
        osc.s = strtod(csvData.readEntry(i, 2), nullptr);
    }

    csvData.destroy();

    // Right bank
    err = csvData.loadCsv(m_oscillatorDataRight.c_str());
    if (err != atg_csv::CsvData::ErrorCode::Success) {
        m_oscillatorCountRight = 0;
        m_oscillatorsRight = nullptr;

        return;
    }

    m_oscillatorsRight = new Oscillator[(size_t)csvData.m_rows - 1];
    m_oscillatorCountRight = csvData.m_rows - 1;

    for (int i = 1; i < csvData.m_rows; ++i) {
        Oscillator &osc = m_oscillatorsRight[(size_t)i - 1];
        osc.freq = strtod(csvData.readEntry(i, 0), nullptr);
        osc.k_d = strtod(csvData.readEntry(i, 1), nullptr);
        osc.s = strtod(csvData.readEntry(i, 2), nullptr);
    }

    csvData.destroy();
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

        if (m_engine.ProcessKeyDown(ysKey::Code::R)) {
            updateOscillatorData();
        }

        double throttle = 0.999; 
        if (m_engine.IsKeyDown(ysKey::Code::A)) {
            throttle = 0.0;
        }
        else if (m_engine.IsKeyDown(ysKey::Code::S)) {
            throttle = 1.0;
        }

        m_iceEngine.getIntake(0)->m_throttle = throttle;

        m_dyno.setSpeed(m_dynoSpeed);
        m_dyno.setSpeed(units::rpm(500.0));

        if (m_engine.ProcessKeyDown(ysKey::Code::D)) {
            m_dyno.setEnabled(!m_dyno.isEnabled());
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
    drawGenerated(indices, layer, m_shaders.GetRegularFlags());
}

void EngineSimApplication::drawGenerated(
    const GeometryGenerator::GeometryIndices &indices,
    int layer,
    dbasic::StageEnableFlags flags)
{
    m_engine.DrawGeneric(
        flags,
        m_geometryIndexBuffer,
        m_geometryVertexBuffer,
        sizeof(dbasic::Vertex),
        indices.BaseIndex,
        indices.BaseVertex,
        indices.FaceCount,
        false,
        layer);
}

void EngineSimApplication::createObjects(Engine *engine, EngineSimulator *simulator) {
    for (int i = 0; i < engine->getCylinderCount(); ++i) {
        ConnectingRodObject *rodObject = new ConnectingRodObject;
        rodObject->initialize(this);
        rodObject->m_connectingRod = engine->getConnectingRod(i);
        m_objects.push_back(rodObject);

        PistonObject *pistonObject = new PistonObject;
        pistonObject->initialize(this);
        pistonObject->m_piston = engine->getPiston(i);
        m_objects.push_back(pistonObject);

        CombustionChamberObject *ccObject = new CombustionChamberObject;
        ccObject->initialize(this);
        ccObject->m_chamber = simulator->getCombustionChamber(i);
        m_objects.push_back(ccObject);
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
    const float aspectRatio = screenWidth / (float)screenHeight;

    const Point cameraPos = m_engineView->getCameraPosition();
    m_shaders.m_cameraPosition = ysMath::LoadVector(cameraPos.x, cameraPos.y);

    m_shaders.CalculateCamera(
        aspectRatio * m_displayHeight / m_engineView->m_zoom,
        m_displayHeight / m_engineView->m_zoom);
    m_shaders.CalculateUiCamera(aspectRatio * m_displayHeight, m_displayHeight);

    Bounds windowBounds((float)screenWidth, (float)screenHeight, { 0, (float)screenHeight });
    Grid grid;
    grid.v_cells = 16;
    grid.h_cells = 19;
    m_engineView->m_bounds = grid.get(windowBounds, 4, 1, 11, 11);
    m_engineView->setLocalPosition({ 0, 0 });

    m_gaugeCluster->m_bounds = grid.get(windowBounds, 14, 1, 5, 11);
    m_temperatureGauge->m_bounds = grid.get(windowBounds, 0, 1, 4, 11);

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
