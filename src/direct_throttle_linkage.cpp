#include "../include/direct_throttle_linkage.h"

#include "../include/engine.h"

#include <cmath>

DirectThrottleLinkage::DirectThrottleLinkage() {
    m_gamma = 1.0;
    m_throttlePosition = 1.0;
}

DirectThrottleLinkage::~DirectThrottleLinkage() {
    /* void */
}

void DirectThrottleLinkage::initialize(const Parameters &params) {
    m_gamma = params.gamma;
}

void DirectThrottleLinkage::setSpeedControl(double s) {
    Throttle::setSpeedControl(s);
    m_throttlePosition = std::pow(1 - s, m_gamma);
}

void DirectThrottleLinkage::update(double dt, Engine *engine) {
    Throttle::update(dt, engine);
    engine->setThrottle(m_throttlePosition);
}
