#include "../include/torque_converter.h"

#include "../include/units.h"

#include <algorithm>
#include <cmath>

// Fraction of the normal fluid capacity that acts on overrun -- turbine
// spinning faster than the impeller, i.e. coasting in gear. The stator
// freewheels and the turbine ends up pumping backwards through blading that
// isn't shaped for it, so the coupling is far weaker in this direction. This
// is why an automatic with the converter unlocked has almost no engine
// braking, and why the symmetric +/- capacity the old constraint-based model
// used was wrong in both directions at once.
static constexpr double kOverrunCouplingFactor = 0.25;

TorqueConverter::TorqueConverter() {
    m_impeller = nullptr;
    m_turbine = nullptr;

    m_stallTorque = units::torque(300.0, units::ft_lb);
    m_stallSpeed = units::rpm(2500);
    m_stallTorqueRatio = 2.0;
    m_couplingPoint = 0.87;

    m_engagement = 1.0;
    m_enabled = false;

    m_direction = 1.0;
    m_speedRatio = 0.0;
    m_torqueRatio = 1.0;
    m_impellerTorque = 0.0;
    m_turbineTorque = 0.0;
}

TorqueConverter::~TorqueConverter() {
    /* void */
}

void TorqueConverter::initialize(const Parameters &params) {
    m_stallTorque = params.StallTorque;
    m_stallSpeed = params.StallSpeed;
    m_stallTorqueRatio = std::max(1.0, params.StallTorqueRatio);
    m_couplingPoint = std::clamp(params.CouplingPoint, 0.1, 1.0);
}

void TorqueConverter::connect(atg_scs::RigidBody *impeller, atg_scs::RigidBody *turbine) {
    m_impeller = impeller;
    m_turbine = turbine;
}

double TorqueConverter::torqueRatio(double speedRatio) const {
    if (speedRatio >= m_couplingPoint) return 1.0;

    return 1.0
        + (m_stallTorqueRatio - 1.0) * (1.0 - speedRatio / m_couplingPoint);
}

void TorqueConverter::apply(atg_scs::SystemState *state) {
    if (!m_enabled || m_impeller == nullptr || m_turbine == nullptr) {
        m_impellerTorque = m_turbineTorque = 0.0;
        return;
    }

    if (m_impeller->index < 0 || m_turbine->index < 0) {
        m_impellerTorque = m_turbineTorque = 0.0;
        return;
    }

    solve(state->v_theta[m_impeller->index], state->v_theta[m_turbine->index]);

    // Positive m_impellerTorque opposes impeller rotation (a load on the
    // engine); positive m_turbineTorque drives the turbine the way the
    // impeller is already turning.
    state->t[m_impeller->index] += -m_direction * m_impellerTorque;
    state->t[m_turbine->index] += m_direction * m_turbineTorque;
}

void TorqueConverter::solve(double impellerOmega, double turbineOmega) {
    m_impellerTorque = m_turbineTorque = 0.0;

    // Work in the impeller's own direction of rotation so the rest of this
    // is plain magnitudes; m_direction goes back on in apply().
    m_direction = (impellerOmega < 0.0) ? -1.0 : 1.0;
    const double impeller = std::abs(impellerOmega);
    const double turbine = turbineOmega * m_direction;

    if (impeller < 1e-6 && turbine < 1e-6) {
        m_speedRatio = 0.0;
        m_torqueRatio = 1.0;
        return;
    }

    // Absorbed torque, on both branches, follows the same law a real
    // converter is specced by: proportional to the square of pump speed, and
    // falling to nothing as the two sides approach the same speed. The
    // (1 - SR^2) term is the part the old model was missing entirely -- it
    // held full capacity right up to lockup, so the coupling kept pulling at
    // full strength no matter how close the turbine already was.
    if (turbine <= impeller) {
        // Impeller drives: the normal case. Stator is working, so the
        // turbine gets more torque out than the impeller puts in.
        const double speedRatio = (impeller > 1e-6)
            ? std::clamp(turbine / impeller, 0.0, 1.0)
            : 0.0;
        const double speedScale = impeller / m_stallSpeed;

        m_speedRatio = speedRatio;
        m_torqueRatio = torqueRatio(speedRatio);
        m_impellerTorque = m_stallTorque
            * speedScale * speedScale
            * (1.0 - speedRatio * speedRatio)
            * m_engagement;
        m_turbineTorque = m_impellerTorque * m_torqueRatio;

        // Power out is TR * SR * power in, and TR * SR peaks at the coupling
        // point (<= 1) for any stall ratio >= 1, so this never manufactures
        // energy -- the balance is the heat a converter actually makes.
    }
    else {
        // Overrun: the turbine is driving. The stator freewheels, so no
        // multiplication, and the coupling is much weaker.
        // Same law, just referenced to the turbine now that it's the one
        // pumping -- so the ratio that matters is impeller-over-turbine.
        const double drivenRatio = std::clamp(impeller / turbine, 0.0, 1.0);
        const double speedScale = turbine / m_stallSpeed;

        // Reported speed ratio stays turbine-over-impeller by convention, so
        // a reading above 1 is what overrun looks like from outside.
        m_speedRatio = turbine / std::max(impeller, 1e-6);
        m_torqueRatio = 1.0;

        const double capacity = kOverrunCouplingFactor * m_stallTorque
            * speedScale * speedScale
            * (1.0 - drivenRatio * drivenRatio)
            * m_engagement;

        // Negative == the coupling is pushing the engine along and dragging
        // the vehicle back, rather than the other way around.
        m_impellerTorque = -capacity;
        m_turbineTorque = -capacity;
    }
}
