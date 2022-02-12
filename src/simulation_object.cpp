#include "../include/simulation_object.h"

#include "../include/engine_sim_application.h"

SimulationObject::SimulationObject() {
    /* void */
}

SimulationObject::~SimulationObject() {
    /* void */
}

void SimulationObject::initialize(EngineSimApplication *app) {
    m_app = app;
}

void SimulationObject::generateGeometry() {
    /* void */
}

void SimulationObject::render(const ViewParameters *settings) {
    /* void */
}

void SimulationObject::process(float dt) {
    /* void */
}

void SimulationObject::destroy() {
    /* void */
}

void SimulationObject::resetShader() {
    m_app->getShaders()->ResetBrdfParameters();
    m_app->getShaders()->SetColorReplace(true);
    m_app->getShaders()->SetLit(false);
    m_app->getShaders()->SetFogFar(2001.0f);
    m_app->getShaders()->SetFogNear(2000.0f);
}

void SimulationObject::setTransform(atg_scs::RigidBody *rigidBody) {
    const ysMatrix rot = ysMath::RotationTransform(
            ysMath::Constants::ZAxis,
            rigidBody->theta);
    const ysMatrix trans = ysMath::TranslationTransform(
            ysMath::LoadVector(rigidBody->p_x, rigidBody->p_y, 0.0f));

    m_app->getShaders()->SetObjectTransform(ysMath::MatMult(trans, rot));
}
