#include "../include/engine_load.h"

EngineLoad::EngineLoad() {
    m_torque = 0;
    m_crankshaft = nullptr;
}

EngineLoad::~EngineLoad() {
    /* void */
}

void EngineLoad::apply(atg_scs::SystemState *system) {
    system->t[m_crankshaft->m_body.index] += m_torque;
}
