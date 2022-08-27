#include "../include/throttle.h"

Throttle::Throttle() {
    m_speedControl = 0;
}

Throttle::~Throttle() {
    /* void */
}

void Throttle::setSpeedControl(double s) {
    m_speedControl = s;
}

void Throttle::update(double dt, Engine *engine) {
    /* void */
}
