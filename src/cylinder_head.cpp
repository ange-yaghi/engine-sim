#include "../include/cylinder_head.h"

#include "../include/cylinder_bank.h"
#include "../include/valvetrain.h"

#include <assert.h>

CylinderHead::CylinderHead() {
    m_exhaustSystems = nullptr;
    m_intakes = nullptr;
    m_soundAttenuation = nullptr;

    m_flipDisplay = false;

    m_bank = nullptr;
    m_valvetrain = nullptr;

    m_exhaustPortFlow = nullptr;
    m_intakePortFlow = nullptr;

    m_intakeRunnerVolume = 0.0;
    m_intakeRunnerCrossSectionArea = 0.0;
    m_exhaustRunnerVolume = 0.0;
    m_exhaustRunnerCrossSectionArea = 0.0;
    m_combustionChamberVolume = 0.0;
}

CylinderHead::~CylinderHead() {
    /* void */
}

void CylinderHead::initialize(const Parameters &params) {
    m_exhaustSystems = new ExhaustSystem *[params.Bank->getCylinderCount()];
    m_intakes = new Intake *[params.Bank->getCylinderCount()];
    m_soundAttenuation = new double[params.Bank->getCylinderCount()];

    m_bank = params.Bank;
    m_valvetrain = params.Valvetrain;
    m_exhaustPortFlow = params.ExhaustPortFlow;
    m_intakePortFlow = params.IntakePortFlow;
    m_combustionChamberVolume = params.CombustionChamberVolume;
    m_flipDisplay = params.FlipDisplay;

    m_intakeRunnerVolume = params.IntakeRunnerVolume;
    m_intakeRunnerCrossSectionArea = params.IntakeRunnerCrossSectionArea;
    m_exhaustRunnerVolume = params.ExhaustRunnerVolume;
    m_exhaustRunnerCrossSectionArea = params.ExhaustRunnerCrossSectionArea;

    memset(m_exhaustSystems, 0, sizeof(ExhaustSystem *) * params.Bank->getCylinderCount());
    memset(m_intakes, 0, sizeof(Intake *) * params.Bank->getCylinderCount());

    for (int i = 0; i < params.Bank->getCylinderCount(); ++i) {
        m_soundAttenuation[i] = 1.0;
    }
}

void CylinderHead::destroy() {
    if (m_exhaustSystems != nullptr) delete[] m_exhaustSystems;
    if (m_intakes != nullptr) delete[] m_intakes;
    if (m_soundAttenuation != nullptr) delete[] m_soundAttenuation;

    m_exhaustSystems = nullptr;
    m_intakes = nullptr;
    m_soundAttenuation = nullptr;
}

double CylinderHead::intakeFlowRate(int cylinder) const {
    return m_intakePortFlow->sampleTriangle(
            intakeValveLift(cylinder));
}

double CylinderHead::exhaustFlowRate(int cylinder) const {
    return m_exhaustPortFlow->sampleTriangle(
            exhaustValveLift(cylinder));
}

double CylinderHead::intakeValveLift(int cylinder) const {
    return m_valvetrain->intakeValveLift(cylinder);
}

double CylinderHead::exhaustValveLift(int cylinder) const {
    return m_valvetrain->exhaustValveLift(cylinder);
}

void CylinderHead::setAllExhaustSystems(ExhaustSystem *system) {
    for (int i = 0; i < m_bank->getCylinderCount(); ++i) {
        m_exhaustSystems[i] = system;
    }
}

void CylinderHead::setExhaustSystem(int i, ExhaustSystem *system) {
    m_exhaustSystems[i] = system;
}

void CylinderHead::setSoundAttenuation(int i, double soundAttenuation) {
    m_soundAttenuation[i] = soundAttenuation;
}

void CylinderHead::setAllIntakes(Intake *intake) {
    for (int i = 0; i < m_bank->getCylinderCount(); ++i) {
        m_intakes[i] = intake;
    }
}

void CylinderHead::setIntake(int i, Intake *intake) {
    m_intakes[i] = intake;
}

Camshaft *CylinderHead::getExhaustCamshaft() {
    return m_valvetrain->getActiveExhaustCamshaft();
}

Camshaft *CylinderHead::getIntakeCamshaft() {
    return m_valvetrain->getActiveIntakeCamshaft();
}
