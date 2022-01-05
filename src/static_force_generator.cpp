#include "../include/static_force_generator.h"

#include <cmath>

StaticForceGenerator::StaticForceGenerator() {
    m_forceX = 0.0;
    m_forceY = 0.0;

    m_positionX = 0.0;
    m_positionY = 0.0;

    m_body = nullptr;
}

StaticForceGenerator::~StaticForceGenerator() {
    /* void */
}

void StaticForceGenerator::apply(OdeSolver::System *system) {
    system->Force_X[m_body->m_index] += m_forceX;
    system->Force_Y[m_body->m_index] += m_forceY;

    const double theta = system->Angles[m_body->m_index];
    system->Torque[m_body->m_index] +=
        std::cos(theta) * m_positionX * m_forceX +
        std::sin(theta) * m_positionY * m_forceY;
}

void StaticForceGenerator::setForce(double f_x, double f_y) {
    m_forceX = f_x;
    m_forceY = f_y;
}

void StaticForceGenerator::setPosition(double p_x, double p_y) {
    m_positionX = p_x;
    m_positionY = p_y;
}
