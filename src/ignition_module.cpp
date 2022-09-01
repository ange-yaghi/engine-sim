#include "../include/ignition_module.h"

#include "../include/utilities.h"
#include "../include/constants.h"
#include "../include/units.h"

#include <cmath>

IgnitionModule::IgnitionModule() {
    m_plugs = nullptr;
    m_crankshaft = nullptr;
    m_timingCurve = nullptr;
    m_cylinderCount = 0;
    m_lastCrankshaftAngle = 0.0;
    m_enabled = false;
    m_revLimitTimer = 0.0;
    m_revLimit = 0;
    m_limiterDuration = 0;
}

IgnitionModule::~IgnitionModule() {
    assert(m_plugs == nullptr);
}

void IgnitionModule::destroy() {
    delete[] m_plugs;

    m_plugs = nullptr;
    m_cylinderCount = 0;
}

void IgnitionModule::initialize(const Parameters &params) {
    m_cylinderCount = params.CylinderCount;
    m_plugs = new SparkPlug[m_cylinderCount];
    m_crankshaft = params.crankshaft;
    m_timingCurve = params.TimingCurve;
    m_revLimit = params.RevLimit;
    m_limiterDuration = params.LimiterDuration;
}

void IgnitionModule::setFiringOrder(int cylinderIndex, double angle) {
    assert(cylinderIndex < m_cylinderCount);

    m_plugs[cylinderIndex].Angle = angle;
    m_plugs[cylinderIndex].Enabled = true;
}

void IgnitionModule::reset() {
    m_lastCrankshaftAngle = m_crankshaft->getCycleAngle();
    resetIgnitionEvents();
}

void IgnitionModule::update(double dt) {
    const double cycleAngle = m_crankshaft->getCycleAngle();

    if (m_enabled && m_revLimitTimer == 0) {
        const double fourPi = 4 * constants::pi;
        const double advance = getTimingAdvance();

        for (int i = 0; i < m_cylinderCount; ++i) {
            double adjustedAngle = positiveMod(m_plugs[i].Angle - advance, fourPi);
            const double r0 = m_lastCrankshaftAngle;
            double r1 = cycleAngle;

            if (m_crankshaft->m_body.v_theta < 0) {
                if (r1 < r0) {
                    r1 += fourPi;
                    adjustedAngle += fourPi;
                }

                if (adjustedAngle >= r0 && adjustedAngle < r1) {
                    m_plugs[i].IgnitionEvent = m_plugs[i].Enabled;
                }
            }
            else {
                if (r1 > r0) {
                    r1 -= fourPi;
                    adjustedAngle -= fourPi;
                }

                if (adjustedAngle >= r1 && adjustedAngle < r0) {
                    m_plugs[i].IgnitionEvent = m_plugs[i].Enabled;
                }
            }
        }
    }

    m_revLimitTimer -= dt;
    if (std::fabs(m_crankshaft->m_body.v_theta) > m_revLimit) {
        m_revLimitTimer = m_limiterDuration;
    }

    if (m_revLimitTimer < 0) {
        m_revLimitTimer = 0;
    }

    m_lastCrankshaftAngle = cycleAngle;
}

bool IgnitionModule::getIgnitionEvent(int index) const {
    return m_plugs[index].IgnitionEvent;
}

void IgnitionModule::resetIgnitionEvents() {
    for (int i = 0; i < m_cylinderCount; ++i) {
        m_plugs[i].IgnitionEvent = false;
    }
}

double IgnitionModule::getTimingAdvance() {
    return m_timingCurve->sampleTriangle(-m_crankshaft->m_body.v_theta);
}

IgnitionModule::SparkPlug *IgnitionModule::getPlug(int i) {
    return &m_plugs[((i % m_cylinderCount) + m_cylinderCount) % m_cylinderCount];
}
