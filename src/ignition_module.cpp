#include "../include/ignition_module.h"

#include "../include/utilities.h"
#include "../include/constants.h"
#include "../include/units.h"

#include <algorithm>
#include <cmath>

// A fixed cut fraction is just a torque multiplier, not a real limiter --
// if the engine has enough torque margin left even with that fraction of
// cylinders cut, nothing stops RPM from climbing indefinitely past
// revLimit (a 0.3 "soft" cut can still let a torquey engine run away to
// well past redline in neutral, where there's essentially no load to
// fight). This is how far above revLimit (as a fraction of revLimit) both
// the cut-fraction ramp and the timing-retard ramp finish reaching their
// full effect -- a closed-loop backstop under either soft mechanism, not
// a per-engine tune, same spirit as Transmission's launch-clutch overrun
// band. Kept tight (a real ECU generally holds RPM within a couple hundred
// RPM of the limiter, not 10-15% over it) -- 0.15 initially let a
// torquey V8 settle nearly 1200rpm above its configured rev_limit before
// either ramp finished engaging.
static constexpr double kSoftLimiterOverrunBand = 0.05;

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
    m_limiterCutFraction = 1.0;
    m_limiterCutAccumulator = 0.0;
    m_limiterMaxRetard = 0.0;
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
    m_cylinderCount = params.cylinderCount;
    m_plugs = new SparkPlug[m_cylinderCount];
    m_crankshaft = params.crankshaft;
    m_timingCurve = params.timingCurve;
    m_revLimit = params.revLimit;
    m_limiterDuration = params.limiterDuration;
    m_limiterCutFraction = params.limiterCutFraction;
    m_limiterMaxRetard = params.limiterMaxRetard;
}

void IgnitionModule::setFiringOrder(int cylinderIndex, double angle) {
    assert(cylinderIndex < m_cylinderCount);

    m_plugs[cylinderIndex].angle = angle;
    m_plugs[cylinderIndex].enabled = true;
}

void IgnitionModule::reset() {
    m_lastCrankshaftAngle = m_crankshaft->getCycleAngle();
    resetIgnitionEvents();
}

void IgnitionModule::update(double dt) {
    const double cycleAngle = m_crankshaft->getCycleAngle();

    // Limiting is now a per-event decision (see the cut-fraction loop
    // below) rather than an all-or-nothing skip of this whole block, so
    // the gate here only needs m_enabled/m_shiftCut -- m_revLimitTimer no
    // longer disqualifies the block wholesale.
    if (m_enabled && !m_shiftCut) {
        const double fourPi = 4 * constants::pi;
        const double advance = getTimingAdvance();
        const bool limiting = m_revLimitTimer > 0.0;

        // limiterCutFraction of 0.0 is a deliberate "no limiter" escape
        // hatch and stays exempt from the ramp below (it should never cut,
        // full stop); anything above that is a real request to limit, so
        // it gets the closed-loop backstop -- the further rpm has climbed
        // past revLimit, the harder the cut, up to a full 1.0 by
        // kSoftLimiterOverrunBand over. This is what keeps a soft cut
        // fraction from just being a flat torque multiplier a torquey
        // engine can grind straight through.
        double effectiveCutFraction = m_limiterCutFraction;
        if (limiting && m_limiterCutFraction > 0.0 && m_revLimit > 0.0) {
            const double rpm = std::fabs(m_crankshaft->m_body.v_theta);
            const double overrunBand = m_revLimit * kSoftLimiterOverrunBand;
            const double overrun = std::clamp((rpm - m_revLimit) / overrunBand, 0.0, 1.0);
            effectiveCutFraction = m_limiterCutFraction + overrun * (1.0 - m_limiterCutFraction);
        }

        for (int i = 0; i < m_cylinderCount; ++i) {
            double adjustedAngle = positiveMod(m_plugs[i].angle - advance, fourPi);
            const double r0 = m_lastCrankshaftAngle;
            double r1 = cycleAngle;

            bool firing = false;
            if (m_crankshaft->m_body.v_theta < 0) {
                if (r1 < r0) {
                    r1 += fourPi;
                    adjustedAngle += fourPi;
                }

                firing = adjustedAngle >= r0 && adjustedAngle < r1;
            }
            else {
                if (r1 > r0) {
                    r1 -= fourPi;
                    adjustedAngle -= fourPi;
                }

                firing = adjustedAngle >= r1 && adjustedAngle < r0;
            }

            if (!firing) continue;

            // Bresenham-style accumulator: spreads the cut evenly across
            // ignition events (round-robin over whichever cylinder happens
            // to fire next) instead of clumping it, so a fraction like 0.3
            // reads as "roughly every third firing," not "cylinder 1 dies
            // for a while, the rest run fine." At the default fraction of
            // 1.0 this cuts every single event, same as the old
            // whole-block skip it replaces.
            bool cut = false;
            if (limiting && effectiveCutFraction > 0.0) {
                m_limiterCutAccumulator += effectiveCutFraction;
                if (m_limiterCutAccumulator >= 1.0) {
                    m_limiterCutAccumulator -= 1.0;
                    cut = true;
                }
            }

            m_plugs[i].ignitionEvent = m_plugs[i].enabled && !cut;
        }
    }

    m_revLimitTimer -= dt;
    if (std::fabs(m_crankshaft->m_body.v_theta) > m_revLimit) {
        m_revLimitTimer = m_limiterDuration;
    }
    else if (m_revLimitTimer <= 0.0) {
        // Fresh start the next time the limiter engages, rather than
        // carrying a stale fractional cut count across two unrelated
        // limiter events that might be seconds apart.
        m_limiterCutAccumulator = 0.0;
    }

    if (m_revLimitTimer < 0) {
        m_revLimitTimer = 0;
    }

    m_lastCrankshaftAngle = cycleAngle;
}

bool IgnitionModule::getIgnitionEvent(int index) const {
    return m_plugs[index].ignitionEvent;
}

void IgnitionModule::resetIgnitionEvents() {
    for (int i = 0; i < m_cylinderCount; ++i) {
        m_plugs[i].ignitionEvent = false;
    }
}

double IgnitionModule::getTimingAdvance() {
    // Timing curves are keyed by engine speed magnitude (rpm), same as a
    // real tachometer -- rotation direction is an arbitrary per-engine
    // artifact of crank throw/tdc setup, not something a timing table
    // should care about. Sampling by raw -v_theta instead of |v_theta|
    // silently clamped every engine that happens to run with positive
    // v_theta to its curve's 0rpm sample for the entire rev range (spark
    // timing frozen at idle advance, never actually advancing).
    const double rpm = std::fabs(m_crankshaft->m_body.v_theta);
    double advance = m_timingCurve->sampleTriangle(rpm) - m_timingRetard;

    // Soft-limiter retard: a smooth, continuous function of instantaneous
    // rpm, not gated by m_revLimitTimer/limiterDuration the way the cut
    // fraction is -- there's no chatter risk to debounce against since
    // nothing here is an on/off decision, just a continuously varying
    // number. Every cylinder still fires every cycle; it just fires later
    // and weaker the further over revLimit the engine has climbed, fully
    // engaged (limiterMaxRetard) by kSoftLimiterOverrunBand over -- same
    // overrun band the cut-fraction ramp in update() uses, so both
    // mechanisms (if both are configured) finish escalating together.
    if (m_limiterMaxRetard > 0.0 && m_revLimit > 0.0 && rpm > m_revLimit) {
        const double overrunBand = m_revLimit * kSoftLimiterOverrunBand;
        const double overrun = std::clamp((rpm - m_revLimit) / overrunBand, 0.0, 1.0);
        advance -= overrun * m_limiterMaxRetard;
    }

    return advance;
}

IgnitionModule::SparkPlug *IgnitionModule::getPlug(int i) {
    return &m_plugs[((i % m_cylinderCount) + m_cylinderCount) % m_cylinderCount];
}
