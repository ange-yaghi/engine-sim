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
#include "../include/utilities.h"

#include "../scripting/include/compiler.h"

#include <chrono>
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

    m_torque = 0;
    m_dynoSpeed = 0;

    m_engineView = nullptr;
    m_rightGaugeCluster = nullptr;
    m_temperatureGauge = nullptr;
    m_oscCluster = nullptr;
    m_performanceCluster = nullptr;
    m_loadSimulationCluster = nullptr;
    m_mixerCluster = nullptr;
    m_infoCluster = nullptr;
    m_iceEngine = nullptr;

    m_oscillatorSampleOffset = 0;
    m_gameWindowHeight = 256;
    m_screenWidth = 256;
    m_screenHeight = 256;
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
    settings.WindowWidth = 1920;
    settings.WindowHeight = 1081;

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
        &m_geometryIndexBuffer, sizeof(unsigned short) * 200000, nullptr);
    m_engine.GetDevice()->CreateVertexBuffer(
        &m_geometryVertexBuffer, sizeof(dbasic::Vertex) * 100000, nullptr);

    m_geometryGenerator.initialize(100000, 200000);

    initialize();
}

void EngineSimApplication::initialize() {
    m_shaders.SetClearColor(ysColor::srgbiToLinear(0x34, 0x98, 0xdb));
    m_assetManager.CompileInterchangeFile((m_assetPath + "/assets").c_str(), 1.0f, true);
    m_assetManager.LoadSceneFile((m_assetPath + "/assets").c_str(), true);

    m_textRenderer.SetEngine(&m_engine);
    m_textRenderer.SetRenderer(m_engine.GetUiRenderer());
    m_textRenderer.SetFont(m_engine.GetConsole()->GetFont());

    Engine testEngine;
    Engine::Parameters engineParams;
    engineParams.Name = "Chev. 454 V8";
    engineParams.CylinderBanks = 2;
    engineParams.CylinderCount = 8;
    engineParams.CrankshaftCount = 1;
    engineParams.ExhaustSystemCount = 2;
    engineParams.IntakeCount = 1;
    engineParams.StarterTorque = units::torque(500, units::ft_lb);
    testEngine.initialize(engineParams);

    Piston::Parameters pistonParams;
    pistonParams.CompressionHeight = units::distance(1.640, units::inch);
    pistonParams.Displacement = 0;
    pistonParams.WristPinPosition = 0;
    pistonParams.Mass = units::mass(880, units::g);

    pistonParams.CylinderIndex = 0;
    pistonParams.Bank = testEngine.getCylinderBank(1);
    pistonParams.Rod = testEngine.getConnectingRod(0);
    pistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(0.2);
    testEngine.getPiston(0)->initialize(pistonParams);

    pistonParams.Bank = testEngine.getCylinderBank(0);
    pistonParams.Rod = testEngine.getConnectingRod(1);
    pistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(0.1);
    testEngine.getPiston(1)->initialize(pistonParams);

    pistonParams.CylinderIndex = 1;
    pistonParams.Bank = testEngine.getCylinderBank(1);
    pistonParams.Rod = testEngine.getConnectingRod(2);
    pistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(0.1);
    testEngine.getPiston(2)->initialize(pistonParams);

    pistonParams.Bank = testEngine.getCylinderBank(0);
    pistonParams.Rod = testEngine.getConnectingRod(3);
    pistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(0.1);
    testEngine.getPiston(3)->initialize(pistonParams);

    pistonParams.CylinderIndex = 2;
    pistonParams.Bank = testEngine.getCylinderBank(1);
    pistonParams.Rod = testEngine.getConnectingRod(4);
    pistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(0.1);
    testEngine.getPiston(4)->initialize(pistonParams);

    pistonParams.Bank = testEngine.getCylinderBank(0);
    pistonParams.Rod = testEngine.getConnectingRod(5);
    pistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(0.2);
    testEngine.getPiston(5)->initialize(pistonParams);

    pistonParams.CylinderIndex = 3;
    pistonParams.Bank = testEngine.getCylinderBank(1);
    pistonParams.Rod = testEngine.getConnectingRod(6);
    pistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(0.1);
    testEngine.getPiston(6)->initialize(pistonParams);

    pistonParams.Bank = testEngine.getCylinderBank(0);
    pistonParams.Rod = testEngine.getConnectingRod(7);
    pistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(0.3);
    testEngine.getPiston(7)->initialize(pistonParams);

    CylinderBank::Parameters cbParams;
    cbParams.Bore = units::distance(4.25, units::inch);
    cbParams.CylinderCount = 4;
    cbParams.DeckHeight = units::distance(9.8, units::inch);

    cbParams.Index = 0;
    cbParams.Angle = constants::pi / 4;
    testEngine.getCylinderBank(0)->initialize(cbParams);

    cbParams.Index = 1;
    cbParams.Angle = -constants::pi / 4;
    testEngine.getCylinderBank(1)->initialize(cbParams);

    Crankshaft::Parameters crankshaftParams;
    crankshaftParams.CrankThrow = units::distance(2.0, units::inch);
    crankshaftParams.FlywheelMass = 0.5 * units::mass(29, units::lb) * 2;
    crankshaftParams.Mass = 0.5 * units::mass(75, units::lb);
    crankshaftParams.FrictionTorque = units::torque(10.0, units::ft_lb);

    // Temporary moment of inertia approximation
    const double crank_r = crankshaftParams.CrankThrow;
    constexpr double flywheel_r = units::distance(14.0, units::inch) / 2.0;
    const double I_crank = (1 / 2.0) * crankshaftParams.Mass * crank_r * crank_r;
    const double I_flywheel =
        (1 / 2.0) * crankshaftParams.FlywheelMass * flywheel_r * flywheel_r;

    crankshaftParams.MomentOfInertia = I_crank + I_flywheel;
    crankshaftParams.Pos_x = 0;
    crankshaftParams.Pos_y = 0;
    crankshaftParams.RodJournals = 4;
    crankshaftParams.TDC = constants::pi / 4 - 2 * constants::pi;
    testEngine.getCrankshaft(0)->initialize(crankshaftParams);
    testEngine.getCrankshaft(0)->setRodJournalAngle(0, 0);
    testEngine.getCrankshaft(0)->setRodJournalAngle(1, -constants::pi / 2);
    testEngine.getCrankshaft(0)->setRodJournalAngle(2, -3 * constants::pi / 2);
    testEngine.getCrankshaft(0)->setRodJournalAngle(3, constants::pi);

    ConnectingRod::Parameters crParams;
    crParams.CenterOfMass = 0;
    crParams.Crankshaft = testEngine.getCrankshaft(0);
    crParams.Journal = 0;
    crParams.Length = units::distance(6.135, units::inch);
    crParams.Mass = units::mass(785, units::g);
    crParams.MomentOfInertia = (1 / 12.0) * crParams.Mass * crParams.Length * crParams.Length;

    crParams.Piston = testEngine.getPiston(0);
    testEngine.getConnectingRod(0)->initialize(crParams);

    crParams.Piston = testEngine.getPiston(1);
    testEngine.getConnectingRod(1)->initialize(crParams);

    crParams.Journal = 1;
    crParams.Piston = testEngine.getPiston(2);
    testEngine.getConnectingRod(2)->initialize(crParams);

    crParams.Piston = testEngine.getPiston(3);
    testEngine.getConnectingRod(3)->initialize(crParams);

    crParams.Journal = 2;
    crParams.Piston = testEngine.getPiston(4);
    testEngine.getConnectingRod(4)->initialize(crParams);

    crParams.Piston = testEngine.getPiston(5);
    testEngine.getConnectingRod(5)->initialize(crParams);

    crParams.Journal = 3;
    crParams.Piston = testEngine.getPiston(6);
    testEngine.getConnectingRod(6)->initialize(crParams);

    crParams.Piston = testEngine.getPiston(7);
    testEngine.getConnectingRod(7)->initialize(crParams);

    // Camshaft
    Camshaft *exhaustCamLeft = new Camshaft, *exhaustCamRight = new Camshaft;
    Camshaft *intakeCamLeft = new Camshaft, *intakeCamRight = new Camshaft;
    Function *camLift0 = new Function;
    camLift0->initialize(1, units::angle(10, units::deg));
    camLift0->setInputScale(1.0);
    camLift0->setOutputScale(1.0);
    
    camLift0->addSample(0.0, units::distance(578, units::thou));
    camLift0->addSample(-units::angle(10, units::deg), units::distance(560, units::thou));
    camLift0->addSample(units::angle(10, units::deg), units::distance(560, units::thou));
    camLift0->addSample(-units::angle(20, units::deg), units::distance(510, units::thou));
    camLift0->addSample(units::angle(20, units::deg), units::distance(510, units::thou));
    camLift0->addSample(-units::angle(30, units::deg), units::distance(430, units::thou));
    camLift0->addSample(units::angle(30, units::deg), units::distance(430, units::thou));
    camLift0->addSample(-units::angle(40, units::deg), units::distance(350, units::thou));
    camLift0->addSample(units::angle(40, units::deg), units::distance(350, units::thou));
    camLift0->addSample(-units::angle(50, units::deg), units::distance(220, units::thou));
    camLift0->addSample(units::angle(50, units::deg), units::distance(220, units::thou));
    camLift0->addSample(-units::angle(60, units::deg), units::distance(75, units::thou));
    camLift0->addSample(units::angle(60, units::deg), units::distance(75, units::thou));
    camLift0->addSample(-units::angle(70, units::deg), units::distance(0, units::thou));
    camLift0->addSample(units::angle(70, units::deg), units::distance(0, units::thou));
    camLift0->addSample(-units::angle(80, units::deg), units::distance(0, units::thou));
    camLift0->addSample(units::angle(80, units::deg), units::distance(0, units::thou));

    Function *camLift1 = new Function;
    camLift1->initialize(1, units::angle(10, units::deg));
    camLift1->setInputScale(1.0);
    camLift1->setOutputScale(1.0);
    camLift1->addSample(0.0, units::distance(578, units::thou));

    Camshaft::Parameters camParams;
    const double lobeSeparation = 109;
    const double advance = (lobeSeparation - 106);
    camParams.Crankshaft = testEngine.getCrankshaft(0);
    camParams.Lobes = 4;
    camParams.Advance = units::angle(advance, units::deg);

    camParams.LobeProfile = camLift0;
    exhaustCamRight->initialize(camParams);
    exhaustCamLeft->initialize(camParams);
    camParams.LobeProfile = camLift0;
    intakeCamLeft->initialize(camParams);
    intakeCamRight->initialize(camParams);

    // 1 8 4 3 6 5 7 2
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

    Function *flow = new Function;
    flow->initialize(1, units::distance(50, units::thou));
    flow->addSample(units::distance(0, units::thou), 0.0);
    flow->addSample(units::distance(50, units::thou), GasSystem::k_28inH2O(10.0));
    flow->addSample(units::distance(100, units::thou), GasSystem::k_28inH2O(76.0));
    flow->addSample(units::distance(150, units::thou), GasSystem::k_28inH2O(100.0));
    flow->addSample(units::distance(200, units::thou), GasSystem::k_28inH2O(146.0));
    flow->addSample(units::distance(250, units::thou), GasSystem::k_28inH2O(175.0));
    flow->addSample(units::distance(300, units::thou), GasSystem::k_28inH2O(212.0));
    flow->addSample(units::distance(350, units::thou), GasSystem::k_28inH2O(230.0));
    flow->addSample(units::distance(400, units::thou), GasSystem::k_28inH2O(255.0));
    flow->addSample(units::distance(450, units::thou), GasSystem::k_28inH2O(275.0));
    flow->addSample(units::distance(500, units::thou), GasSystem::k_28inH2O(294.0));
    flow->addSample(units::distance(550, units::thou), GasSystem::k_28inH2O(300.0));
    flow->addSample(units::distance(600, units::thou), GasSystem::k_28inH2O(314.0));

    Function *exhaustFlow = new Function;
    exhaustFlow->initialize(1, units::distance(50, units::thou));
    exhaustFlow->addSample(units::distance(0, units::thou), 0.0);
    exhaustFlow->addSample(units::distance(50, units::thou), GasSystem::k_28inH2O(10.0));
    exhaustFlow->addSample(units::distance(100, units::thou), GasSystem::k_28inH2O(70.0));
    exhaustFlow->addSample(units::distance(150, units::thou), GasSystem::k_28inH2O(100.0));
    exhaustFlow->addSample(units::distance(200, units::thou), GasSystem::k_28inH2O(132.0));
    exhaustFlow->addSample(units::distance(250, units::thou), GasSystem::k_28inH2O(140.0));
    exhaustFlow->addSample(units::distance(300, units::thou), GasSystem::k_28inH2O(156.0));
    exhaustFlow->addSample(units::distance(350, units::thou), GasSystem::k_28inH2O(170.0));
    exhaustFlow->addSample(units::distance(400, units::thou), GasSystem::k_28inH2O(181.0));
    exhaustFlow->addSample(units::distance(450, units::thou), GasSystem::k_28inH2O(191.0));
    exhaustFlow->addSample(units::distance(500, units::thou), GasSystem::k_28inH2O(207.0));
    exhaustFlow->addSample(units::distance(550, units::thou), GasSystem::k_28inH2O(214.0));
    exhaustFlow->addSample(units::distance(600, units::thou), GasSystem::k_28inH2O(228.0));

    CylinderHead::Parameters chParams;
    chParams.IntakePortFlow = flow;
    chParams.ExhaustPortFlow = exhaustFlow;
    chParams.CombustionChamberVolume = units::volume(118.0, units::cc);

    chParams.IntakeCam = intakeCamLeft;
    chParams.ExhaustCam = exhaustCamLeft;
    chParams.Bank = testEngine.getCylinderBank(0);
    chParams.FlipDisplay = true;
    testEngine.getHead(0)->initialize(chParams);

    chParams.IntakeCam = intakeCamRight;
    chParams.ExhaustCam = exhaustCamRight;
    chParams.Bank = testEngine.getCylinderBank(1);
    chParams.FlipDisplay = false;
    testEngine.getHead(1)->initialize(chParams);

    Intake::Parameters inParams;
    inParams.InputFlowK = GasSystem::k_carb(950.0);
    inParams.Volume = units::volume(5000.0, units::cc);
    inParams.IdleFlowK = 0.0000015;
    inParams.IdleThrottlePlatePosition = 0.967;
    testEngine.getIntake(0)->initialize(inParams);

    ExhaustSystem::Parameters esParams;
    esParams.OutletFlowRate = GasSystem::k_carb(1000.0);
    esParams.Volume = units::volume(10.0, units::L);
    esParams.AudioVolume = 1.0;
    testEngine.getExhaustSystem(0)->initialize(esParams);

    esParams.AudioVolume = 0.1;
    testEngine.getExhaustSystem(1)->initialize(esParams);

    testEngine.getHead(0)->setAllExhaustSystems(testEngine.getExhaustSystem(0));
    testEngine.getHead(1)->setAllExhaustSystems(testEngine.getExhaustSystem(1));

    testEngine.getHead(0)->setAllIntakes(testEngine.getIntake(0));
    testEngine.getHead(1)->setAllIntakes(testEngine.getIntake(0));

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
    imParams.Crankshaft = testEngine.getCrankshaft(0);
    imParams.CylinderCount = 8;
    imParams.TimingCurve = timingCurve;
    imParams.RevLimit = units::rpm(7000.0);
    testEngine.getIgnitionModule()->initialize(imParams);
    constexpr double cycle = units::angle(2 * 360.0, units::deg);
    testEngine.getIgnitionModule()->setFiringOrder(1 - 1, (0 / 8.0) * cycle);
    testEngine.getIgnitionModule()->setFiringOrder(8 - 1, (1 / 8.0) * cycle);
    testEngine.getIgnitionModule()->setFiringOrder(4 - 1, (2 / 8.0) * cycle);
    testEngine.getIgnitionModule()->setFiringOrder(3 - 1, (3 / 8.0) * cycle);
    testEngine.getIgnitionModule()->setFiringOrder(6 - 1, (4 / 8.0) * cycle);
    testEngine.getIgnitionModule()->setFiringOrder(5 - 1, (5 / 8.0) * cycle);
    testEngine.getIgnitionModule()->setFiringOrder(7 - 1, (6 / 8.0) * cycle);
    testEngine.getIgnitionModule()->setFiringOrder(2 - 1, (7 / 8.0) * cycle);

    Function *turbulenceToFlameSpeedRatio = new Function;
    Function *equivalenceRatioToLaminarFlameSpeed = new Function;

    equivalenceRatioToLaminarFlameSpeed->initialize(12, 0.1);
    equivalenceRatioToLaminarFlameSpeed->addSample(0.8, units::distance(22, units::cm) / units::sec);
    equivalenceRatioToLaminarFlameSpeed->addSample(0.9, units::distance(27, units::cm) / units::sec);
    equivalenceRatioToLaminarFlameSpeed->addSample(1.0, units::distance(32, units::cm) / units::sec);
    equivalenceRatioToLaminarFlameSpeed->addSample(1.1, units::distance(35, units::cm) / units::sec);
    equivalenceRatioToLaminarFlameSpeed->addSample(1.2, units::distance(33, units::cm) / units::sec);
    equivalenceRatioToLaminarFlameSpeed->addSample(1.3, units::distance(30, units::cm) / units::sec);
    equivalenceRatioToLaminarFlameSpeed->addSample(1.4, units::distance(25, units::cm) / units::sec);

    turbulenceToFlameSpeedRatio->initialize(10, 10.0);
    turbulenceToFlameSpeedRatio->addSample(0.0, 1.0);
    turbulenceToFlameSpeedRatio->addSample(5.0, 1.5 * 5.0);
    turbulenceToFlameSpeedRatio->addSample(10.0, 1.5 * 10.0);
    turbulenceToFlameSpeedRatio->addSample(15.0, 1.5 * 15.0);
    turbulenceToFlameSpeedRatio->addSample(20.0, 1.5 * 20.0);
    turbulenceToFlameSpeedRatio->addSample(25.0, 1.5 * 25.0);
    turbulenceToFlameSpeedRatio->addSample(30.0, 1.5 * 30.0);
    turbulenceToFlameSpeedRatio->addSample(35.0, 1.5 * 35.0);
    turbulenceToFlameSpeedRatio->addSample(40.0, 1.5 * 40.0);
    turbulenceToFlameSpeedRatio->addSample(45.0, 1.5 * 45.0);

    Fuel::Parameters fParams;
    fParams.TurbulenceToFlameSpeedRatio = turbulenceToFlameSpeedRatio;
    Fuel *fuel = new Fuel;
    fuel->initialize(fParams);

    Function *meanPistonSpeedToTurbulence = new Function;
    meanPistonSpeedToTurbulence->initialize(30, 1);
    for (int i = 0; i < 30; ++i) {
        const double s = (double)i;
        meanPistonSpeedToTurbulence->addSample(s, s * 0.5);
    }

    CombustionChamber::Parameters ccParams;
    ccParams.CrankcasePressure = units::pressure(1.0, units::atm);
    ccParams.Fuel = fuel;
    ccParams.StartingPressure = units::pressure(1.0, units::atm);
    ccParams.StartingTemperature = units::celcius(25.0);
    ccParams.MeanPistonSpeedToTurbulence = meanPistonSpeedToTurbulence;

    for (int i = 0; i < 8; ++i) {
        ccParams.Piston = testEngine.getPiston(i);
        ccParams.Head = testEngine.getHead(ccParams.Piston->getCylinderBank()->getIndex());
        testEngine.getChamber(i)->initialize(ccParams);
    }

    // Scripting
#ifdef ATG_ENGINE_PIRANHA_ENABLED

    es_script::Compiler compiler;
    compiler.initialize();
    compiler.compile("../assets/test.mr");
    es_script::Compiler::Output output = compiler.execute();
    compiler.destroy();

    m_iceEngine = output.engine;

#endif /* PIRANHA_ENABLED */

    Vehicle::Parameters vehParams;
    vehParams.Mass = units::mass(1597, units::kg);
    vehParams.DiffRatio = 3.42;
    vehParams.TireRadius = units::distance(10, units::inch);
    vehParams.DragCoefficient = 0.25;
    vehParams.CrossSectionArea = units::distance(6.0, units::foot) * units::distance(6.0, units::foot);
    vehParams.RollingResistance = 2000.0;
    Vehicle *vehicle = new Vehicle;
    vehicle->initialize(vehParams);

    const double gearRatios[] = { 2.97, 2.07, 1.43, 1.00, 0.84, 0.56 };
    Transmission::Parameters tParams;
    tParams.GearCount = 6;
    tParams.GearRatios = gearRatios;
    tParams.MaxClutchTorque = units::torque(1000.0, units::ft_lb);
    Transmission *transmission = new Transmission;
    transmission->initialize(tParams);

    Simulator::Parameters simulatorParams;
    simulatorParams.Engine = m_iceEngine;
    simulatorParams.SystemType = Simulator::SystemType::NsvOptimized;
    simulatorParams.Transmission = transmission;
    simulatorParams.Vehicle = vehicle;
    simulatorParams.SimulationFrequency = 11000;
    m_simulator.initialize(simulatorParams);
    m_simulator.startAudioRenderingThread();
    createObjects(m_iceEngine);

    m_uiManager.initialize(this);

    m_engineView = m_uiManager.getRoot()->addElement<EngineView>();
    m_rightGaugeCluster = m_uiManager.getRoot()->addElement<RightGaugeCluster>();
    m_rightGaugeCluster->m_engine = m_iceEngine;
    m_rightGaugeCluster->m_simulator = &m_simulator;

    m_oscCluster = m_uiManager.getRoot()->addElement<OscilloscopeCluster>();
    m_oscCluster->m_simulator = &m_simulator;

    m_performanceCluster = m_uiManager.getRoot()->addElement<PerformanceCluster>();
    m_performanceCluster->setSimulator(&m_simulator);

    m_loadSimulationCluster = m_uiManager.getRoot()->addElement<LoadSimulationCluster>();
    m_loadSimulationCluster->setSimulator(&m_simulator);

    m_mixerCluster = m_uiManager.getRoot()->addElement<MixerCluster>();
    m_mixerCluster->setSimulator(&m_simulator);

    m_infoCluster = m_uiManager.getRoot()->addElement<InfoCluster>();
    m_infoCluster->setEngine(m_iceEngine);

    m_audioBuffer.initialize(44100, 44100);
    m_audioBuffer.m_writePointer = (int)(44100 * 0.1);

    ysAudioParameters params;
    params.m_bitsPerSample = 16;
    params.m_channelCount = 1;
    params.m_sampleRate = 44100;
    m_outputAudioBuffer =
        m_engine.GetAudioDevice()->CreateBuffer(&params, 44100);

    m_audioSource = m_engine.GetAudioDevice()->CreateSource(m_outputAudioBuffer);
    m_audioSource->SetMode(ysAudioSource::Mode::Loop);
    m_audioSource->SetPan(0.0f);
    m_audioSource->SetVolume(1.0f);

    testEngine.destroy();
}

void EngineSimApplication::process(float frame_dt) {
    //frame_dt = clamp(frame_dt, 1 / 100.0f, 1 / 30.0f);

    double speed = 1.0 / 1.0;
    if (m_engine.IsKeyDown(ysKey::Code::N1)) {
        speed = 1 / 10.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N2)) {
        speed = 1 / 100.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N3)) {
        speed = 1 / 200.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N4)) {
        speed = 1 / 500.0;
    }
    else if (m_engine.IsKeyDown(ysKey::Code::N5)) {
        speed = 1 / 1000.0;
    }

    m_simulator.setSimulationSpeed(speed);

    m_simulator.startFrame(frame_dt);

    auto proc_t0 = std::chrono::steady_clock::now();
    const int iterationCount = m_simulator.getFrameIterationCount();
    while (m_simulator.simulateStep()) {
        const double runnerPressure = m_simulator.getEngine()->getChamber(0)->m_intakeRunner.pressure()
            + m_simulator.getEngine()->getChamber(0)->m_intakeRunner.dynamicPressure(1.0, 0.0);
        const double cylinderPressure = m_simulator.getEngine()->getChamber(0)->m_system.pressure()
            + m_simulator.getEngine()->getChamber(0)->m_system.dynamicPressure(-1.0, 0.0);

        if (m_simulator.getCurrentIteration() % 2 == 0) {
            m_oscCluster->getTotalExhaustFlowOscilloscope()->addDataPoint(
                m_simulator.getEngine()->getCrankshaft(0)->getCycleAngle(),
                m_simulator.getTotalExhaustFlow() / m_simulator.getTimestep());
            m_oscCluster->getCylinderPressureScope()->addDataPoint(
                m_simulator.getEngine()->getCrankshaft(0)->getCycleAngle(constants::pi),
                std::sqrt(cylinderPressure));
            m_oscCluster->getExhaustFlowOscilloscope()->addDataPoint(
                m_simulator.getEngine()->getCrankshaft(0)->getCycleAngle(),
                m_simulator.getEngine()->getChamber(0)->getLastTimestepExhaustFlow() / m_simulator.getTimestep());
            m_oscCluster->getIntakeFlowOscilloscope()->addDataPoint(
                m_simulator.getEngine()->getCrankshaft(0)->getCycleAngle(),
                m_simulator.getEngine()->getChamber(0)->getLastTimestepIntakeFlow() / m_simulator.getTimestep());
            m_oscCluster->getCylinderMoleculesScope()->addDataPoint(
                m_simulator.getEngine()->getCrankshaft(0)->getCycleAngle(),
                m_simulator.getEngine()->getChamber(0)->m_system.n());
            m_oscCluster->getExhaustValveLiftOscilloscope()->addDataPoint(
                m_simulator.getEngine()->getCrankshaft(0)->getCycleAngle(),
                m_simulator.getEngine()->getChamber(0)->getCylinderHead()->exhaustValveLift(
                    m_simulator.getEngine()->getChamber(0)->getPiston()->getCylinderIndex()));
            m_oscCluster->getIntakeValveLiftOscilloscope()->addDataPoint(
                m_simulator.getEngine()->getCrankshaft(0)->getCycleAngle(),
                m_simulator.getEngine()->getChamber(0)->getCylinderHead()->intakeValveLift(
                    m_simulator.getEngine()->getChamber(0)->getPiston()->getCylinderIndex()));
            m_oscCluster->getPvScope()->addDataPoint(
                m_simulator.getEngine()->getChamber(0)->getVolume(),
                std::sqrt(m_simulator.getEngine()->getChamber(0)->m_system.pressure()));
        }
    }

    auto proc_t1 = std::chrono::steady_clock::now();

    m_simulator.endFrame();

    auto duration = proc_t1 - proc_t0;
    if (iterationCount > 0) {
        m_performanceCluster->addTimePerTimestepSample(
            (duration.count() / 1E9) / iterationCount);
    }

    const SampleOffset currentAudioPosition = m_audioSource->GetCurrentPosition();
    const SampleOffset safeWritePosition = m_audioSource->GetCurrentWritePosition();
    const SampleOffset writePosition = m_audioBuffer.m_writePointer;

    SampleOffset targetWritePosition =
        m_audioBuffer.getBufferIndex(currentAudioPosition, (int)(44100 * 0.1));
    SampleOffset maxWrite = m_audioBuffer.offsetDelta(writePosition, targetWritePosition);

    SampleOffset currentLead = m_audioBuffer.offsetDelta(currentAudioPosition, writePosition);
    SampleOffset newLead = m_audioBuffer.offsetDelta(currentAudioPosition, targetWritePosition);

    if (currentLead > newLead) {
        maxWrite = 0;
    }

    int16_t *samples = new int16_t[maxWrite];
    const int readSamples = m_simulator.readAudioOutput(maxWrite, samples);

    for (SampleOffset i = 0; i < (SampleOffset)readSamples && i < maxWrite; ++i) {
        const int16_t sample = samples[i];
        if (m_oscillatorSampleOffset % 4 == 0) {
            m_oscCluster->getAudioWaveformOscilloscope()->addDataPoint(
                m_oscillatorSampleOffset,
                sample / (float)(INT16_MAX));
        }

        m_audioBuffer.writeSample(sample, m_audioBuffer.m_writePointer, (int)i);

        m_oscillatorSampleOffset = (m_oscillatorSampleOffset + 1) % (44100 / 10);
    }

    delete[] samples;

    if (readSamples > 0) {
        SampleOffset size0, size1;
        void *data0, *data1;
        m_audioSource->LockBufferSegment(
            m_audioBuffer.m_writePointer, readSamples, &data0, &size0, &data1, &size1);

        m_audioBuffer.copyBuffer(
            reinterpret_cast<int16_t *>(data0), m_audioBuffer.m_writePointer, size0);
        m_audioBuffer.copyBuffer(
            reinterpret_cast<int16_t *>(data1),
            m_audioBuffer.getBufferIndex(m_audioBuffer.m_writePointer, size0),
            size1);

        m_audioSource->UnlockBufferSegments(data0, size0, data1, size1);
        m_audioBuffer.commitBlock(readSamples);
    }

    m_performanceCluster->addAudioLatencySample(
        m_audioBuffer.offsetDelta(m_audioSource->GetCurrentPosition(), m_audioBuffer.m_writePointer) / (44100 * 0.1));
    m_performanceCluster->addInputBufferUsageSample(
        (double)m_simulator.getSynthesizerInputLatency() / m_simulator.getSynthesizerInputLatencyTarget());
}

void EngineSimApplication::render() {
    SimulationObject::ViewParameters view;
    view.Layer0 = 0;
    view.Layer1 = 3;

    for (SimulationObject *object : m_objects) {
        object->generateGeometry();
        object->render(&view);
    }

    m_uiManager.render();
}

float EngineSimApplication::pixelsToUnits(float pixels) const {
    const float f = m_displayHeight / m_gameWindowHeight;
    return pixels * f;
}

float EngineSimApplication::unitsToPixels(float units) const {
    const float f = m_gameWindowHeight / m_displayHeight;
    return units * f;
}

void EngineSimApplication::run() {
    double throttle = 1.0;
    double targetThrottle = 1.0;

    double clutchPressure = 1.0;
    int lastMouseWheel = 0;

    while (true) {
        const float dt = m_engine.GetFrameLength();
        const bool fineControlMode = m_engine.IsKeyDown(ysKey::Code::Space);

        const int mouseWheel = m_engine.GetMouseWheel();
        const int mouseWheelDelta = mouseWheel - lastMouseWheel;
        lastMouseWheel = mouseWheel;

        m_gameWindowHeight = m_engine.GetGameWindow()->GetGameHeight();
        m_screenHeight = m_engine.GetGameWindow()->GetScreenHeight();
        m_screenWidth = m_engine.GetGameWindow()->GetScreenWidth();

        if (m_engine.ProcessKeyDown(ysKey::Code::Escape)) {
            break;
        }

        m_engine.StartFrame();
        if (!m_engine.IsOpen()) break;

        updateScreenSizeStability();

        if (m_engine.ProcessKeyDown(ysKey::Code::F)) {
            m_engine.GetGameWindow()->SetWindowStyle(ysWindow::WindowStyle::Fullscreen);
            m_infoCluster->setLogMessage("Entered fullscreen mode");
        }

        bool fineControlInUse = false;
        if (m_engine.IsKeyDown(ysKey::Code::Z)) {
            const double rate = fineControlMode
                ? 0.001
                : 0.01;

            Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
            audioParams.Volume = clamp(audioParams.Volume + mouseWheelDelta * rate * dt);

            m_simulator.getSynthesizer()->setAudioParameters(audioParams);
            fineControlInUse = true;

            m_infoCluster->setLogMessage("[Z] - Set volume to " + std::to_string(audioParams.Volume));
        }
        else if (m_engine.IsKeyDown(ysKey::Code::X)) {
            const double rate = fineControlMode
                ? 0.001
                : 0.01;

            Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
            audioParams.Convolution = clamp(audioParams.Convolution + mouseWheelDelta * rate * dt);

            m_simulator.getSynthesizer()->setAudioParameters(audioParams);
            fineControlInUse = true;

            m_infoCluster->setLogMessage("[X] - Set convolution level to " + std::to_string(audioParams.Convolution));
        }
        else if (m_engine.IsKeyDown(ysKey::Code::C)) {
            const double rate = fineControlMode
                ? 0.00001
                : 0.001;

            Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
            audioParams.dF_F_mix = clamp(audioParams.dF_F_mix + mouseWheelDelta * rate * dt);

            m_simulator.getSynthesizer()->setAudioParameters(audioParams);
            fineControlInUse = true;

            m_infoCluster->setLogMessage("[C] - Set high freq. gain to " + std::to_string(audioParams.dF_F_mix));
        }
        else if (m_engine.IsKeyDown(ysKey::Code::V)) {
            const double rate = fineControlMode
                ? 0.001
                : 0.01;

            Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
            audioParams.AirNoise = clamp(audioParams.AirNoise + mouseWheelDelta * rate * dt);

            m_simulator.getSynthesizer()->setAudioParameters(audioParams);
            fineControlInUse = true;

            m_infoCluster->setLogMessage("[V] - Set low freq. noise to " + std::to_string(audioParams.AirNoise));
        }
        else if (m_engine.IsKeyDown(ysKey::Code::B)) {
            const double rate = fineControlMode
                ? 0.001
                : 0.01;

            Synthesizer::AudioParameters audioParams = m_simulator.getSynthesizer()->getAudioParameters();
            audioParams.InputSampleNoise = clamp(audioParams.InputSampleNoise + mouseWheelDelta * rate * dt);

            m_simulator.getSynthesizer()->setAudioParameters(audioParams);
            fineControlInUse = true;

            m_infoCluster->setLogMessage("[B] - Set high freq. noise to " + std::to_string(audioParams.InputSampleNoise));
        }

        const double prevTargetThrottle = targetThrottle;
        targetThrottle = fineControlMode ? targetThrottle : 1.0;
        if (m_engine.IsKeyDown(ysKey::Code::Q)) {
            targetThrottle = 0.99;
        }
        else if (m_engine.IsKeyDown(ysKey::Code::W)) {
            targetThrottle = 0.9;
        }
        else if (m_engine.IsKeyDown(ysKey::Code::E)) {
            targetThrottle = 0.8;
        }
        else if (m_engine.IsKeyDown(ysKey::Code::R)) {
            targetThrottle = 0.0;
        }
        else if (fineControlMode && !fineControlInUse) {
            targetThrottle = std::fmax(0.0, std::fmin(1.0, targetThrottle - mouseWheelDelta * 0.0001));
        }

        if (prevTargetThrottle != targetThrottle) {
            m_infoCluster->setLogMessage("Throttle set to " + std::to_string(targetThrottle));
        }

        throttle = targetThrottle * 0.5 + 0.5 * throttle;

        m_iceEngine->setThrottle(throttle);

        if (m_engine.ProcessKeyDown(ysKey::Code::D)) {
            m_simulator.m_dyno.m_enabled = !m_simulator.m_dyno.m_enabled;

            const std::string msg = m_simulator.m_dyno.m_enabled
                ? "DYNOMOMETER ENABLED"
                : "DYNOMOMETER DISABLED";
            m_infoCluster->setLogMessage(msg);
        }

        if (m_simulator.m_dyno.m_enabled) {
            if (m_simulator.getFilteredDynoTorque() > units::torque(50.0, units::ft_lb)) {
                m_dynoSpeed += units::rpm(500) * dt;
            }
            else {
                m_dynoSpeed *= (1 / (1 + dt));
            }

            if ((m_dynoSpeed + units::rpm(1000)) > m_iceEngine->getRedline()) {
                m_simulator.m_dyno.m_enabled = false;
                m_dynoSpeed = units::rpm(0);
            }
        }
        else {
            m_dynoSpeed = units::rpm(0);
        }

        m_simulator.m_dyno.m_rotationSpeed = m_dynoSpeed + units::rpm(1000);

        const bool prevStarterEnabled = m_simulator.m_starterMotor.m_enabled;
        if (m_engine.IsKeyDown(ysKey::Code::S)) {
            m_simulator.m_starterMotor.m_enabled = true;
        }
        else {
            m_simulator.m_starterMotor.m_enabled = false;
        }

        if (prevStarterEnabled != m_simulator.m_starterMotor.m_enabled) {
            const std::string msg = m_simulator.m_starterMotor.m_enabled
                ? "STARTER ENABLED"
                : "STARTER DISABLED";
            m_infoCluster->setLogMessage(msg);
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::A)) {
            m_simulator.getEngine()->getIgnitionModule()->m_enabled =
                !m_simulator.getEngine()->getIgnitionModule()->m_enabled;

            const std::string msg = m_simulator.getEngine()->getIgnitionModule()->m_enabled
                ? "IGNITION ENABLED"
                : "IGNITION DISABLED";
            m_infoCluster->setLogMessage(msg);
        }

        if (m_engine.ProcessKeyDown(ysKey::Code::Up)) {
            m_simulator.getTransmission()->changeGear(m_simulator.getTransmission()->getGear() + 1);

            m_infoCluster->setLogMessage(
                "UPSHIFTED TO " + std::to_string(m_simulator.getTransmission()->getGear()));
        }
        else if (m_engine.ProcessKeyDown(ysKey::Code::Down)) {
            m_simulator.getTransmission()->changeGear(m_simulator.getTransmission()->getGear() - 1);

            m_infoCluster->setLogMessage(
                "DOWNSHIFTED TO " + std::to_string(m_simulator.getTransmission()->getGear()));
        }

        double newClutchPressure = 1.0;
        if (m_engine.IsKeyDown(ysKey::Code::Shift)) {
            newClutchPressure = 0.0;

            m_infoCluster->setLogMessage("CLUTCH DEPRESSED");
        }

        double clutchRC = 0.001;
        if (m_engine.IsKeyDown(ysKey::Code::Space)) {
            clutchRC = 1.0;
        }

        const double clutch_s = dt / (dt + clutchRC);
        clutchPressure = clutchPressure * (1 - clutch_s) + newClutchPressure * clutch_s;
        m_simulator.getTransmission()->setClutchPressure(clutchPressure);

        if (m_engine.ProcessKeyDown(ysKey::Code::M) &&
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
            process(m_engine.GetFrameLength());
        }

        m_uiManager.update(m_engine.GetFrameLength());

        renderScene();

        m_engine.EndFrame();

        if (isRecording()) {
            recordFrame();
        }
    }

    if (isRecording()) {
        stopRecording();
    }

    m_simulator.endAudioRenderingThread();
}

void EngineSimApplication::destroy() {
    m_shaderSet.Destroy();

    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryVertexBuffer);
    m_engine.GetDevice()->DestroyGPUBuffer(m_geometryIndexBuffer);

    m_assetManager.Destroy();
    m_engine.Destroy();

    m_simulator.destroy();
}

void EngineSimApplication::drawGenerated(
        const GeometryGenerator::GeometryIndices &indices,
        int layer)
{
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

        CombustionChamberObject *ccObject = new CombustionChamberObject;
        ccObject->initialize(this);
        ccObject->m_chamber = m_iceEngine->getChamber(i);
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
        cbObject->m_head = engine->getHead(i);
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
    grid.v_cells = 2;
    grid.h_cells = 3;
    Grid grid3x3;
    grid3x3.v_cells = 3;
    grid3x3.h_cells = 3;
    m_engineView->m_bounds = grid.get(windowBounds, 1, 0, 1, 1);
    m_engineView->setLocalPosition({ 0, 0 });

    m_rightGaugeCluster->m_bounds = grid.get(windowBounds, 2, 0, 1, 2);
    m_oscCluster->m_bounds = grid.get(windowBounds, 1, 1);
    m_performanceCluster->m_bounds = grid3x3.get(windowBounds, 0, 1);
    m_loadSimulationCluster->m_bounds = grid3x3.get(windowBounds, 0, 2);

    Grid grid1x3;
    grid1x3.v_cells = 3;
    grid1x3.h_cells = 1;
    m_mixerCluster->m_bounds = grid1x3.get(grid3x3.get(windowBounds, 0, 0), 0, 2);
    m_infoCluster->m_bounds = grid1x3.get(grid3x3.get(windowBounds, 0, 0), 0, 0, 1, 2);

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
    atg_dtv::Frame *frame = m_encoder.newFrame(false);
    if (frame != nullptr && m_encoder.getError() == atg_dtv::Encoder::Error::None) {
        m_engine.GetDevice()->ReadRenderTarget(m_engine.GetScreenRenderTarget(), frame->m_rgb);
    }

    m_encoder.submitFrame();
#endif /* ATG_ENGINE_SIM_VIDEO_CAPTURE */
}
