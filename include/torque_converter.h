#ifndef ATG_ENGINE_SIM_TORQUE_CONVERTER_H
#define ATG_ENGINE_SIM_TORQUE_CONVERTER_H

#include "scs.h"

// The fluid coupling in an automatic transmission, modelled as a force
// generator rather than a constraint.
//
// That distinction is the whole point. A ClutchConstraint is a velocity
// constraint: it drives two bodies toward the *same* speed with a capped
// torque, and because its Jacobian is [-1, +1] the torque it applies to
// either side is necessarily equal and opposite. A torque converter is
// neither of those things:
//
//   - It never tries to equalize speed. It applies a torque that the fluid
//     happens to be carrying at the current operating point, and the two
//     sides end up wherever the rest of the drivetrain puts them.
//   - It does not apply equal and opposite torque. The stator reflects the
//     fluid leaving the turbine back into the impeller's direction of
//     rotation, so the turbine sees *more* torque than the impeller gives
//     up -- the torque multiplication that a converter is named for. A
//     constraint cannot express that at all.
//
// Modelling it as a constraint is also what let it stall the engine: capped
// or not, a velocity constraint sitting between a running engine and a
// stationary car pulls the engine down to the car's speed, and the cap only
// decides how fast that happens.
//
// Sign convention: this sim's normal running direction is negative v_theta
// (see StarterMotor's default), so everything below works in magnitudes
// measured along the impeller's own direction of rotation and the direction
// is reapplied once at the end.
class TorqueConverter : public atg_scs::ForceGenerator {
    public:
        struct Parameters {
            // Torque the impeller absorbs at StallSpeed with the turbine
            // held stationary -- the classic converter rating. Absorption
            // scales with the square of impeller speed from there.
            double StallTorque;
            double StallSpeed;

            // Stator-driven torque multiplication at stall, falling linearly
            // to 1.0 at the coupling point. ~2:1 is typical for a passenger
            // car converter; loose truck/racing converters go higher.
            double StallTorqueRatio;

            // Speed ratio (turbine/impeller) at which the stator starts to
            // freewheel and the converter stops multiplying and behaves as a
            // plain fluid coupling. ~0.85-0.9 on a real unit.
            double CouplingPoint;
        };

    public:
        TorqueConverter();
        virtual ~TorqueConverter();

        void initialize(const Parameters &params);
        void connect(atg_scs::RigidBody *impeller, atg_scs::RigidBody *turbine);

        virtual void apply(atg_scs::SystemState *state) override;

        // Solves the coupling for a given pair of shaft speeds and records
        // the result in the diagnostics below. apply() is this plus writing
        // the torques into the solver; splitting it out means the coupling's
        // behaviour can be checked at a chosen operating point without
        // standing up a whole rigid body system to get there.
        void solve(double impellerOmega, double turbineOmega);

        // Fraction of full fluid capacity the converter is allowed to carry,
        // 0..1 -- see Transmission::updateTorqueConverter(), which drives it
        // from the "neutral idle" logic. Disengaged entirely (neutral, or
        // the lockup clutch carrying the load instead) is m_enabled = false.
        inline void setEngagement(double engagement) { m_engagement = engagement; }
        inline double getEngagement() const { return m_engagement; }
        inline void setEnabled(bool enabled) { m_enabled = enabled; }
        inline bool isEnabled() const { return m_enabled; }

        // Diagnostics, sampled on the last apply().
        inline double getSpeedRatio() const { return m_speedRatio; }
        inline double getTorqueRatio() const { return m_torqueRatio; }
        inline double getImpellerTorque() const { return m_impellerTorque; }
        inline double getTurbineTorque() const { return m_turbineTorque; }

    protected:
        // Stator contribution: torque multiplication as a function of speed
        // ratio, 1.0 at and above the coupling point.
        double torqueRatio(double speedRatio) const;

        atg_scs::RigidBody *m_impeller;
        atg_scs::RigidBody *m_turbine;

        double m_stallTorque;
        double m_stallSpeed;
        double m_stallTorqueRatio;
        double m_couplingPoint;

        double m_engagement;
        bool m_enabled;

        // Sign of the impeller's rotation on the last solve() -- this sim
        // runs negative, and solve() works in magnitudes along that
        // direction, so apply() needs it to put the torques back the right
        // way round.
        double m_direction;
        double m_speedRatio;
        double m_torqueRatio;
        double m_impellerTorque;
        double m_turbineTorque;
};

#endif /* ATG_ENGINE_SIM_TORQUE_CONVERTER_H */
