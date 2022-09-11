#include "../include/cylinder_bank.h"

#include "../include/constants.h"

#include <cmath>

CylinderBank::CylinderBank() {
    m_angle = 0.0;
    m_bore = 0.0;
    m_deckHeight = 0.0;
    m_cylinderCount = 0;
    m_displayDepth = 0.4;
    m_index = -1;

    m_dx = m_dy = 0;
    m_x = m_y = 0;
}

CylinderBank::~CylinderBank() {
    /* void */
}

void CylinderBank::initialize(const Parameters &params) {
    m_angle = params.angle;
    m_bore = params.bore;
    m_deckHeight = params.deckHeight;
    m_cylinderCount = params.cylinderCount;

    m_dx = std::cos(m_angle + constants::pi / 2);
    m_dy = std::sin(m_angle + constants::pi / 2);

    m_x = params.positionX;
    m_y = params.positionY;

    m_displayDepth = params.displayDepth;

    m_index = params.index;
}

void CylinderBank::destroy() {
    /* void */
}

void CylinderBank::getPositionAboveDeck(double h, double *x, double *y) const {
    *x = m_dx * (m_deckHeight + h) + m_x;
    *y = m_dy * (m_deckHeight + h) + m_y;
}

double CylinderBank::boreSurfaceArea() const {
    return constants::pi * m_bore * m_bore / 4.0;
}
