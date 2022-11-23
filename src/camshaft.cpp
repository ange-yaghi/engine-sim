#include "../include/camshaft.h"

#include "../include/crankshaft.h"
#include "../include/constants.h"
#include "../include/units.h"

#include <cmath>
#include <assert.h>

Camshaft::Camshaft() {
    m_crankshaft = nullptr;
    m_lobeAngles = nullptr;
    m_lobeProfile = nullptr;
    m_lobes = 0;
    m_advance = 0;
    m_baseRadius = 0;
}

Camshaft::~Camshaft() {
    assert(m_lobeAngles == nullptr);
}

void Camshaft::initialize(const Parameters &params) {
    m_lobeAngles = new double[params.lobes];
    memset(m_lobeAngles, 0, sizeof(double) * params.lobes);

    m_lobes = params.lobes;
    m_crankshaft = params.crankshaft;
    m_lobeProfile = params.lobeProfile;
    m_advance = params.advance;
    m_baseRadius = params.baseRadius;
}

void Camshaft::destroy() {
    delete[] m_lobeAngles;
    m_lobeAngles = nullptr;

    m_lobes = 0;
}

double Camshaft::valveLift(int lobe) const {
    return sampleLobe(getAngle() + m_lobeAngles[lobe]);
}

double Camshaft::sampleLobe(double theta) const {
    double clampedTheta = std::fmod(theta, 2 * constants::pi);
    if (clampedTheta < 0) clampedTheta += 2 * constants::pi;
    if (clampedTheta >= constants::pi) clampedTheta -= 2 * constants::pi;

    return m_lobeProfile->sampleTriangle(clampedTheta);
}

double Camshaft::getAngle() const {
    const double angle =
        std::fmod((m_crankshaft->getAngle() + m_advance) * 0.5, 2 * constants::pi);
    return (angle < 0)
        ?  angle + 2 * constants::pi
        :  angle;
}
