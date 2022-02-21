#include "../include/piston.h"

#include "../include/connecting_rod.h"
#include "../include/crankshaft.h"

Piston::Piston() {
    m_rod = nullptr;
    m_bank = nullptr;
    m_cylinderConstraint = nullptr;
    m_cylinderIndex = -1;
    m_compressionHeight = 0.0;
    m_displacement = 0.0;
    m_wristPinLocation = 0.0;
    m_mass = 0.0;
}

Piston::~Piston() {
    /* void */
}

void Piston::initialize(const Parameters &params) {
    m_rod = params.Rod;
    m_bank = params.Bank;
    m_cylinderIndex = params.CylinderIndex;
    m_compressionHeight = params.CompressionHeight;
    m_displacement = params.Displacement;
    m_wristPinLocation = params.WristPinPosition;
    m_mass = params.Mass;
}

void Piston::destroy() {
    /* void */
}

double Piston::relativeX() const {
    return m_body.p_x - m_rod->m_crankshaft->m_p_x;
}

double Piston::relativeY() const {
    return m_body.p_y - m_rod->m_crankshaft->m_p_y;
}
