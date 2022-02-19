#include "../include/cylinder_head.h"

#include "../include/cylinder_bank.h"

#include <assert.h>

CylinderHead::CylinderHead() {
    m_bank = nullptr;
    m_exhaustCamshaft = nullptr;
    m_intakeCamshaft = nullptr;

    m_exhaustPortFlow = nullptr;
    m_intakePortFlow = nullptr;
}

CylinderHead::~CylinderHead() {
    /* void */
}

void CylinderHead::initialize(const Parameters &params) {
    m_bank = params.Bank;
    m_exhaustCamshaft = params.ExhaustCam;
    m_intakeCamshaft = params.IntakeCam;
    m_exhaustPortFlow = params.ExhaustPortFlow;
    m_intakePortFlow = params.IntakePortFlow;
}

void CylinderHead::destroy() {
    /* void */
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
    return m_intakeCamshaft->valveLift(cylinder);
}

double CylinderHead::exhaustValveLift(int cylinder) const {
    return m_exhaustCamshaft->valveLift(cylinder);
}
