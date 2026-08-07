#ifndef ATG_ENGINE_SIM_SUPERCHARGER_H
#define ATG_ENGINE_SIM_SUPERCHARGER_H

#include "scs.h"

#include "gas_system.h"
#include "units.h"

class Crankshaft;

// A belt/gear-driven forced-induction unit: unlike Turbocharger (exhaust-
// driven, zero mechanical connection to the crank -- see its header
// comment), a supercharger's compressor shaft is rigidly coupled to the
// crankshaft at a fixed ratio, and the power to compress the charge is
// drawn directly off the crank as a torque load. That load is what makes
// this a ForceGenerator rather than a passive Part: apply() writes a real
// drag torque onto the driving crankshaft's rigid body every physics step,
// computed from whatever the gas-dynamics side actually did last substep
// (see process()) -- the same one-step lag CombustionChamber's piston
// force already has relative to combustion state, not a new pattern.
//
// Three types share this file because they differ only in how process()
// gets from "shaft speed" to "mass flow and compression power", not in
// the drive coupling or crank-load mechanism:
//
//   - Roots / TwinScrew (positive displacement): flow is metered by
//     shaft speed and swept volume, essentially independent of downstream
//     pressure -- a PD blower doesn't "choose" its pressure ratio, it's
//     dictated entirely by how much backpressure the plenum already
//     holds. This is why PD superchargers deliver strong boost/torque
//     low in the rev range: flow (and hence load) scales with RPM
//     directly rather than RPM^2. TwinScrew differs from Roots only in
//     its efficiency/leakage defaults (see supercharger_node.h /
//     objects.mr) -- its internal compression before port opening is a
//     materially more efficient process than Roots' sudden-backflow
//     mixing event, not a different flow model.
//   - Centrifugal (dynamic compressor): flow and pressure ratio instead
//     follow a speed-squared characteristic just like Turbocharger's
//     compressor stage (in fact this reuses that same math), fed through
//     the real GasSystem::flow() primitive into a finite discharge
//     volume. Because the impeller is kinematically slaved to crank RPM
//     (no free-spinning shaft inertia to integrate, unlike a turbo), boost
//     still ramps in hard at high RPM and does very little at idle --
//     the defining trait that separates a centrifugal blower from a PD
//     unit at the same peak rating, even though neither one "spools" in
//     the turbo sense.
class Supercharger : public atg_scs::ForceGenerator {
    public:
        enum class Type {
            Roots,
            TwinScrew,
            Centrifugal
        };

        struct Parameters {
            Type type = Type::Roots;

            // Fixed belt/gear ratio -- supercharger shaft revolutions per
            // crank revolution. For a centrifugal unit this already
            // includes any internal step-up gearbox.
            double driveRatio = 2.5;

            // Belt/bearing/gear losses between the crank and the
            // compressor shaft -- scales the crank-referred load above
            // the pure gas-compression power.
            double mechanicalEfficiency = 0.90;

            // --- Positive displacement (Roots/TwinScrew) sizing ---

            // Swept volume per supercharger shaft revolution.
            double displacement = units::volume(2.3, units::L);

            // Volumetric efficiency at zero boost.
            double volumetricEfficiency = 0.85;

            // How much volumetric efficiency is lost per atmosphere of
            // boost -- internal leakback growing with the pressure the
            // trapped charge has to push against. Roots (no internal
            // compression) leaks back a lot more than TwinScrew for the
            // same boost target; see the type-specific presets in
            // objects.mr.
            double leakageCoefficient = 0.10;

            // --- Centrifugal sizing ---

            // Shaft speed (rad/s, post-drive-ratio) at which the
            // compressor characteristic reaches max_boost.
            double maxImpellerSpeed = units::rpm(50000.0);

            // Effective flow constant (k_flow) for the compressor's fill
            // of its own discharge volume -- how quickly it can charge
            // the plenum once it has head to do so. Centrifugal only.
            double compressorFlowRateK = 0.0;

            // --- Shared ---

            // Isentropic/adiabatic compression efficiency. Same role
            // across all three types, but realistic values differ:
            // Roots ~0.55-0.65 (compression is an irreversible backflow
            // event at port opening), TwinScrew ~0.75-0.85 (internal
            // compression approximates the real pressure ratio),
            // Centrifugal ~0.70-0.80.
            double compressorEfficiency = 0.60;

            // Bypass/relief setpoint (gauge pressure) -- excess flow is
            // recirculated (PD types) or the characteristic is capped
            // (centrifugal), the same role a turbo's wastegate plays.
            double maxBoost = units::pressure(10.0, units::psi);

            // Charge pipe/plenum capacitance between the compressor
            // discharge and the throttle plate.
            double compressorDischargeVolume = units::volume(2.0, units::L);

            // Most superchargers are permanently belt-driven -- the
            // default (false) matches every standalone unit, spinning
            // continuously at a fixed ratio with no engagement logic. A
            // twincharger's supercharger stage is different: a magnetic
            // clutch plus a bypass valve engage it only low in the rev
            // range, handing off to a turbo stage (see Turbocharger::
            // setUpstreamSupercharger()) once it's spooled. Set clutched
            // to enable that handoff instead of always-on drive.
            bool clutched = false;

            // Hysteresis band (crank RPM) for the clutch: engages once
            // RPM falls below clutchEngageRpm, disengages once it rises
            // above clutchDisengageRpm. The gap between them prevents
            // rapid re-engagement chatter right at the handoff point.
            // Ignored unless clutched is set.
            double clutchEngageRpm = units::rpm(3500.0);
            double clutchDisengageRpm = units::rpm(4500.0);
        };

    public:
        Supercharger();
        virtual ~Supercharger();

        void initialize(const Parameters &params);
        void destroy();

        // The crankshaft this supercharger's belt/gear is driven from --
        // also what apply() writes the reaction load torque onto.
        void setCrankshaft(Crankshaft *crankshaft) { m_crankshaft = crankshaft; }

        void process(double dt);

        virtual void apply(atg_scs::SystemState *state) override;

        double getShaftSpeed() const { return m_omega; }
        bool isClutchEngaged() const { return m_clutchEngaged; }
        double getPressureRatio() const { return m_pressureRatio; }
        double getBoostPressure() const {
            return (m_pressureRatio - 1.0) * units::pressure(1.0, units::atm);
        }

        // Crank-referred load torque from the last process() call -- the
        // magnitude apply() subtracts from whichever way the crank is
        // currently turning.
        double getLoadTorque() const { return m_loadTorque; }

        const Parameters &getParameters() const { return m_parameters; }

        // Real (finite, stateful) discharge volume between the
        // compressor and the throttle plate -- Intake::process() draws
        // directly from this when a supercharger is attached, same as it
        // does for a Turbocharger's compressor discharge.
        GasSystem *getCompressorDischargeSystem() { return &m_compressorDischarge; }
        double getCompressorDischargePressure() const;
        double getCompressorDischargeTemperature() const;

    protected:
        double processPositiveDisplacement(double dt, double omega);
        double processCentrifugal(double dt, double omega);

        Parameters m_parameters;
        Crankshaft *m_crankshaft = nullptr;

        double m_omega = 0.0;
        double m_pressureRatio = 1.0;
        double m_loadTorque = 0.0;
        bool m_clutchEngaged = true;

        double m_compressorDischargeCrossSection = 0.0;

        GasSystem m_inlet;
        GasSystem m_compressorSource;
        GasSystem m_compressorDischarge;
};

#endif /* ATG_ENGINE_SIM_SUPERCHARGER_H */
