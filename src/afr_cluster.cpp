#include "../include/afr_cluster.h"

#include "../include/units.h"
#include "../include/gauge.h"
#include "../include/constants.h"
#include "../include/engine_sim_application.h"

#include <sstream>

AfrCluster::AfrCluster() {
    m_simulator = nullptr;
    m_intakeAfrGauge = nullptr;
    m_exhaustAfrGauge = nullptr;
}

AfrCluster::~AfrCluster() {
    /* void */
}

void AfrCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_intakeAfrGauge = addElement<LabeledGauge>();
    m_exhaustAfrGauge = addElement<LabeledGauge>();

    const float shortenAngle = (float)units::angle(1.0, units::deg);

    m_intakeAfrGauge->m_title = "IN. AFR";
    m_intakeAfrGauge->m_unit = "";
    m_intakeAfrGauge->m_precision = 1;
    m_intakeAfrGauge->setLocalPosition({ 0, 0 });
    m_intakeAfrGauge->m_gauge->m_min = 0;
    m_intakeAfrGauge->m_gauge->m_max = 50;
    m_intakeAfrGauge->m_gauge->m_minorStep = 1;
    m_intakeAfrGauge->m_gauge->m_majorStep = 5;
    m_intakeAfrGauge->m_gauge->m_maxMinorTick = 7000;
    m_intakeAfrGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_intakeAfrGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_intakeAfrGauge->m_gauge->m_needleWidth = 4.0;
    m_intakeAfrGauge->m_gauge->m_gamma = 1.0f;
    m_intakeAfrGauge->m_gauge->m_needleKs = 1000.0f;
    m_intakeAfrGauge->m_gauge->m_needleKd = 20.0f;
    m_intakeAfrGauge->m_gauge->setBandCount(0);

    m_exhaustAfrGauge->m_title = "EX. O2";
    m_exhaustAfrGauge->m_unit = "";
    m_exhaustAfrGauge->m_precision = 1;
    m_exhaustAfrGauge->setLocalPosition({ 0, 0 });
    m_exhaustAfrGauge->m_gauge->m_min = 0;
    m_exhaustAfrGauge->m_gauge->m_max = 100;
    m_exhaustAfrGauge->m_gauge->m_minorStep = 5;
    m_exhaustAfrGauge->m_gauge->m_majorStep = 10;
    m_exhaustAfrGauge->m_gauge->m_maxMinorTick = 200;
    m_exhaustAfrGauge->m_gauge->m_thetaMin = constants::pi * 1.2;
    m_exhaustAfrGauge->m_gauge->m_thetaMax = -0.2 * constants::pi;
    m_exhaustAfrGauge->m_gauge->m_needleWidth = 4.0;
    m_exhaustAfrGauge->m_gauge->m_gamma = 1.0f;
    m_exhaustAfrGauge->m_gauge->m_needleKs = 1000.0f;
    m_exhaustAfrGauge->m_gauge->m_needleKd = 20.0f;
    m_exhaustAfrGauge->m_gauge->setBandCount(0);
}

void AfrCluster::destroy() {
    UiElement::destroy();
}

void AfrCluster::update(float dt) {
    UiElement::update(dt);
}

void AfrCluster::render() {
    Engine *engine = m_simulator->getEngine();

    const Bounds top = m_bounds.verticalSplit(0.5f, 1.0f);
    const Bounds bottom = m_bounds.verticalSplit(0.0f, 0.5f);

    m_intakeAfrGauge->m_bounds = top;
    m_intakeAfrGauge->m_gauge->m_value = engine->getIntakeAfr();

    m_exhaustAfrGauge->m_bounds = bottom;
    m_exhaustAfrGauge->m_gauge->m_value = engine->getExhaustO2() * 100;

    UiElement::render();
}
