#include "../include/camshaft.h"

#include "../include/crankshaft.h"
#include "../include/constants.h"
#include "../include/units.h"

#include <cmath>
#include <assert.h>

Camshaft::Camshaft() {
    m_crankshaft = nullptr;
    m_lobeAngles = nullptr;
    m_lobes = 0;
}

Camshaft::~Camshaft() {
    assert(m_lobeAngles == nullptr);
}

void Camshaft::initialize(const Parameters &params) {
    m_lobeAngles = new double[params.Lobes];
    m_lobes = params.Lobes;
    m_crankshaft = params.Crankshaft;
}

void Camshaft::destroy() {
    delete[] m_lobeAngles;
    m_lobes = 0;
}

double Camshaft::valveLift(int lobe) const {
    return sampleLobe(getAngle() - m_lobeAngles[lobe]);
}

double Camshaft::sampleLobe(double theta) const {
    return m_lobeProfile->sampleTriangle(theta);
}

double Camshaft::getAngle() const {
    const double angle = std::fmodf(m_crankshaft->getAngle() * 0.5, 2 * Constants::pi);
    if (angle < 0) {
        return angle + 2 * Constants::pi;
    }
    else {
        return angle;
    }
}
