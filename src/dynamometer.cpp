#include "../include/dynamometer.h"

#include "../include/units.h"

#include <cmath>

Dynamometer::Dynamometer() {
    m_constraint = nullptr;
}

Dynamometer::~Dynamometer() {
    /* void */
}

void Dynamometer::initialize(CrankshaftLoad *constraint) {
    m_constraint = constraint;
}

double Dynamometer::readTorque() const {
    return isEnabled()
        ? m_constraint->F_t[0][0]
        : 0;
}

void Dynamometer::setEnabled(bool enabled) {
    m_constraint->m_enableDyno = enabled;
}
