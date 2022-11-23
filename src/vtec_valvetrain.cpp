#include "../include/vtec_valvetrain.h"

#include "../include/engine.h"

VtecValvetrain::VtecValvetrain() {
    m_intakeCamshaft = nullptr;
    m_exhaustCamshaft = nullptr;

    m_vtecIntakeCamshaft = nullptr;
    m_vtecExhaustCamshaft = nullptr;

    m_engine = nullptr;

    m_minRpm = 0.0;
    m_minSpeed = 0.0;
    m_minThrottlePosition = 0.0;
    m_manifoldVacuum = 0.0;
}

VtecValvetrain::~VtecValvetrain() {
    /* void */
}

void VtecValvetrain::initialize(const Parameters &parameters) {
    m_intakeCamshaft = parameters.intakeCamshaft;
    m_exhaustCamshaft = parameters.exhaustCamshaft;
    m_vtecIntakeCamshaft = parameters.vtecIntakeCamshaft;
    m_vtecExhaustCamshaft = parameters.vtexExhaustCamshaft;

    m_minRpm = parameters.minRpm;
    m_minSpeed = parameters.minSpeed;
    m_minThrottlePosition = parameters.minThrottlePosition;
    m_manifoldVacuum = parameters.manifoldVacuum;
    m_engine = parameters.engine;
}

double VtecValvetrain::intakeValveLift(int cylinder) {
    return isVtecEnabled()
        ? m_vtecIntakeCamshaft->valveLift(cylinder)
        : m_intakeCamshaft->valveLift(cylinder);
}

double VtecValvetrain::exhaustValveLift(int cylinder) {
    return isVtecEnabled()
        ? m_vtecExhaustCamshaft->valveLift(cylinder)
        : m_exhaustCamshaft->valveLift(cylinder);
}

Camshaft *VtecValvetrain::getActiveIntakeCamshaft() {
    return isVtecEnabled()
        ? m_vtecIntakeCamshaft
        : m_intakeCamshaft;
}

Camshaft *VtecValvetrain::getActiveExhaustCamshaft() {
    return isVtecEnabled()
        ? m_vtecExhaustCamshaft
        : m_exhaustCamshaft;
}

bool VtecValvetrain::isVtecEnabled() const {
    return
        m_engine->getManifoldPressure() > m_manifoldVacuum
        && m_engine->getSpeed() > m_minRpm
        && (1 - m_engine->getThrottle()) > m_minThrottlePosition;
}
