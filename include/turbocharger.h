#ifndef ATG_ENGINE_SIM_TURBOCHARGER_H
#define ATG_ENGINE_SIM_TURBOCHARGER_H

#include "part.h"

#include "bov_sound.h"
#include "gas_system.h"

#include <vector>

class ExhaustSystem;
class Intake;
class Supercharger;

// Couples an exhaust collector (turbine side) to an intake plenum
// (compressor side) through a single rotating shaft. The turbine extracts
// energy from the normal collector->atmosphere flow (replacing
// ExhaustSystem's own outlet flow while attached), so its k_flow directly
// sets exhaust backpressure -- this is also the turbo's only mechanical
// load on the crank: there's no belt/gear connection, so any parasitic
// drag it imposes on the engine shows up as pumping loss against that
// backpressure, not a direct torque subtraction.
//
// The compressor side is a real (if small) discharge volume -- the charge
// pipe between the compressor wheel and the throttle plate -- fed through
// a Moore-Greitzer-style cubic pressure-vs-flow characteristic instead of
// a single-valued function of shaft speed. That characteristic has a
// positive-slope ("unstable") region at low flow; when the throttle
// closes with the shaft still spun up, the operating point is forced onto
// it and the coupled flow/plenum-pressure state genuinely oscillates
// (surge) rather than that behavior being scripted. See process().
class Turbocharger : public Part {
    public:
        // Baseline turbine wheel size that the scale factors derived from
        // Parameters::turbineInducerDiameter/turbineExducerDiameter are
        // relative to -- also the fields' own defaults, so a turbo that
        // doesn't set them gets a scale factor of exactly 1.
        static constexpr double BaselineTurbineInducerDiameter = units::distance(50.0, units::mm);
        static constexpr double BaselineTurbineExducerDiameter = units::distance(40.0, units::mm);

        struct Parameters {
            // Shaft + wheels rotational inertia
            double turbineInertia;

            // Turbine effective flow constant (k_flow) -- this is what
            // replaces the exhaust system's own outlet_flow_rate once
            // attached, so it directly sets exhaust backpressure.
            double turbineRateK;

            // Fraction of the isentropic enthalpy drop across the turbine
            // that becomes shaft work (rest is lost to the exhaust stream).
            double turbineEfficiency = 0.68;

            // Isentropic compressor efficiency.
            double compressorEfficiency = 0.70;

            // Wastegate setpoint -- once boost (gauge pressure) reaches
            // this, the shaft is prevented from accelerating further.
            double maxBoost = units::pressure(10.0, units::psi);

            // Shaft speed (rad/s) at which the turbo would reach maxBoost,
            // absent the wastegate clamp. Used only to derive the
            // pressure-ratio-vs-speed curve below; not a hard limit itself.
            double spoolSpeed = units::rpm(120000.0);

            // Roughly how long (seconds) the shaft takes to coast down
            // under drag alone, off-throttle. Derives an internal friction
            // coefficient from the shaft's own inertia so a bigger/heavier
            // shaft naturally decays slower for the same setting.
            double spinDownTime = 0.8;

            // Volume of the charge pipe between the compressor wheel and
            // the throttle plate (no intercooler volume modeled). This is
            // what the compressor is charging and the throttle is
            // draining -- its capacitance, paired with the compressor's
            // characteristic below, is what lets surge actually oscillate
            // instead of being a fixed formula.
            double compressorDischargeVolume = units::volume(1.5, units::L);

            // Turbine wheel inducer (inlet, where exhaust gas enters the
            // wheel -- the larger of the two on a turbine) and exducer
            // (outlet, where it exits -- the smaller) diameters. Real turbo
            // spec numbers, not just labels: relative to the baseline size
            // below (which reproduces a scale factor of 1, so turbos that
            // don't set these are unaffected), a bigger exducer raises the
            // effective turbine_flow_rate (~exducer flow area, quadratic in
            // diameter) and a bigger inducer raises the effective
            // turbine_inertia (~wheel size^5, since mass scales with volume
            // and rotational inertia with mass*radius^2) on top of whatever
            // those two are separately rated for. This is what makes a
            // physically bigger wheel both flow more AND spool slower,
            // instead of those being two unrelated knobs.
            double turbineInducerDiameter = BaselineTurbineInducerDiameter;
            double turbineExducerDiameter = BaselineTurbineExducerDiameter;

            // Blow-off/flutter sample played (looped, gated by isSurging())
            // in place of the app's synthesized flutter chuff when set.
            // Purely an audio asset reference -- not used by process().
            BovSound *bovSound = nullptr;
        };

    public:
        Turbocharger();
        virtual ~Turbocharger();

        void initialize(const Parameters &params);
        virtual void destroy();

        void addExhaustSystem(ExhaustSystem *exhaust);
        void setIntake(Intake *intake) { m_intake = intake; }

        // Series-staged forced induction (a "twincharger"): when set, this
        // turbo's compressor draws from the supercharger's discharge
        // (real, RPM-varying pressure/temperature) instead of flat
        // atmosphere, so the two stages' pressure ratios multiply rather
        // than either one working in isolation. The supercharger's own
        // discharge should NOT also be attached to the Intake in this
        // configuration -- it feeds this turbo's inlet, not the plenum
        // directly. Typically paired with a clutched supercharger (see
        // Supercharger::Parameters::clutched) so the blower covers low
        // RPM and the turbo takes over once it's spooled.
        void setUpstreamSupercharger(Supercharger *supercharger) { m_upstreamSupercharger = supercharger; }

        void process(double dt);

        double getShaftSpeed() const { return m_omega; }
        double getPressureRatio() const { return m_pressureRatio; }
        double getBoostPressure() const {
            return (m_pressureRatio - 1.0) * units::pressure(1.0, units::atm);
        }

        // Raw spec numbers as given (not the derived scale factors --
        // those aren't independently meaningful outside process()).
        double getTurbineInducerDiameter() const { return m_parameters.turbineInducerDiameter; }
        double getTurbineExducerDiameter() const { return m_parameters.turbineExducerDiameter; }

        // Note: turbineRateK/turbineInertia here already have the wheel-size
        // scale factors from initialize() baked in -- not the raw spec
        // values a caller passed in originally.
        const Parameters &getParameters() const { return m_parameters; }

        inline BovSound *getBovSound() const { return m_parameters.bovSound; }

        // Undoes exactly the turbineRateK/turbineInertia wheel-scale
        // transform initialize() applies (using the diameters, which
        // initialize() never modifies, to recompute the same scale
        // factors). Re-initializing with the result reproduces the current
        // state without compounding the scale factor a second time -- what
        // a live editor needs to seed its fields from an already-running
        // turbo (see TurbochargerEditorCluster).
        Parameters recoverSpecParameters() const;

        // True when the compressor characteristic has pushed flow negative
        // this substep -- i.e. the plenum is momentarily pushing back
        // through the compressor instead of the compressor charging it.
        // This is the actual surge event, not a heuristic guess at one.
        bool isSurging() const { return m_compressorFlowRate < 0.0; }

        // Real (finite, stateful) discharge volume between the compressor
        // wheel and the throttle plate -- Intake::process() draws directly
        // from this when a turbo is attached, instead of a fixed-pressure
        // reservoir, so the throttle plate actually depletes it.
        GasSystem *getCompressorDischargeSystem() { return &m_compressorDischarge; }
        double getCompressorDischargePressure() const;
        double getCompressorDischargeTemperature() const;

    protected:
        Parameters m_parameters;

        std::vector<ExhaustSystem *> m_exhaustSystems;
        Intake *m_intake = nullptr;
        Supercharger *m_upstreamSupercharger = nullptr;

        double m_omega = 0.0;
        double m_pressureRatio = 1.0;
        double m_pressureRatioGain = 0.0;
        double m_frictionCoefficient = 0.0;

        // mol/s the compressor characteristic delivered last substep;
        // negative means backflow (surge). Fed back into the
        // characteristic next substep as the flow coefficient.
        double m_compressorFlowRate = 0.0;
        double m_compressorDischargeCrossSection = 0.0;

        GasSystem m_turbineOutlet;
        GasSystem m_compressorSource;
        GasSystem m_compressorDischarge;
};

#endif /* ATG_ENGINE_SIM_TURBOCHARGER_H */
