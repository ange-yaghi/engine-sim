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
    m_app->getShaders()->ResetBaseColor();
    m_app->getShaders()->SetObjectTransform(ysMath::LoadIdentity());
}

void SimulationObject::setTransform(
    atg_scs::RigidBody *rigidBody,
    float scale,
    float lx,
    float ly,
    float angle)
{
    double p_x, p_y;
    rigidBody->localToWorld(lx, ly, &p_x, &p_y);

    const ysMatrix rot = ysMath::RotationTransform(
            ysMath::Constants::ZAxis,
            rigidBody->theta + angle);
    const ysMatrix trans = ysMath::TranslationTransform(
            ysMath::LoadVector((float)p_x, (float)p_y, 0.0f));
    const ysMatrix scaleTransform = ysMath::ScaleTransform(ysMath::LoadScalar(scale));

    m_app->getShaders()->SetObjectTransform(
        ysMath::MatMult(ysMath::MatMult(trans, rot), scaleTransform));
}

ysVector SimulationObject::tintByLayer(const ysVector &col, int layers) const {
    ysVector result = col;
    for (int i = 0; i < layers; ++i) {
        result = ysMath::Add(
            ysMath::Mul(result, ysMath::LoadScalar(0.1f)),
            ysMath::Mul(m_app->getBackgroundColor(), ysMath::LoadScalar(0.9f))
        );
    }

    return result;
}
