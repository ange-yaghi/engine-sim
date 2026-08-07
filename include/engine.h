#ifndef ATG_ENGINE_SIM_ENGINE_H
#define ATG_ENGINE_SIM_ENGINE_H

#include "part.h"

#include "piston.h"
#include "connecting_rod.h"
#include "crankshaft.h"
#include "cylinder_bank.h"
#include "cylinder_head.h"
#include "exhaust_system.h"
#include "ignition_module.h"
#include "intake.h"
#include "combustion_chamber.h"
#include "units.h"
#include "throttle.h"
#include "turbocharger.h"
#include "supercharger.h"

#include <string>

class Simulator;
class Vehicle;
class Transmission;
class Engine : public Part {
    public:
        struct Parameters {
            int cylinderBanks;
            int cylinderCount;
            int crankshaftCount;
            int exhaustSystemCount;
            int intakeCount;
            int turbochargerCount;
            int superchargerCount;

            std::string name;

            double starterTorque = units::torque(90.0, units::ft_lb);
            double starterSpeed = units::rpm(200);
            double redline = units::rpm(6500);
            double dynoMinSpeed = units::rpm(1000);
            double dynoMaxSpeed = units::rpm(6500);
            double dynoHoldStep = units::rpm(100);

            Throttle *throttle;

            double initialSimulationFrequency;
            double initialHighFrequencyGain;
            double initialNoise;
            double initialJitter;

            // Off-throttle exhaust pop/burble tuning -- see
            // Engine::updateAntiLag(). Disabled by default so nothing
            // changes for an engine file that doesn't ask for it (the UI
            // toggle still works either way). The three "amount" knobs
            // default to the values this system always used before they
            // existed, so antiLagEnabledByDefault: true with everything
            // else left alone reproduces the original one-size-fits-all
            // tune; set them explicitly to give a specific engine its own
            // pop character (e.g. a factory M-car crackle vs. a
            // comfort-tuned six that's nearly silent on lift-off) instead
            // of sharing one fixed, race-anti-lag-style tune.
            bool antiLagEnabledByDefault = false;
            double antiLagMaxRetard = units::angle(55.0, units::deg);
            double antiLagDumpRateK = 6.0;
            double antiLagPopRate = 6.0;
        };

    public:
        Engine();
        virtual ~Engine();

        void initialize(const Parameters &params);
        virtual void destroy();

        std::string getName() const { return m_name; }

        virtual Crankshaft *getOutputCrankshaft() const;
        virtual void setSpeedControl(double s);
        virtual double getSpeedControl();
        virtual void setThrottle(double throttle);
        virtual double getThrottle() const;
        virtual double getThrottlePlateAngle() const;
        virtual void calculateDisplacement();
        double getDisplacement() const { return m_displacement; }
        virtual double getIntakeFlowRate() const;
        virtual void update(double dt);

        virtual double getManifoldPressure() const;
        virtual double getIntakeAfr() const;
        virtual double getExhaustO2() const;
        virtual double getRpm() const;
        virtual double getSpeed() const;
        virtual bool isSpinningCw() const;

        virtual void resetFuelConsumption();
        virtual double getTotalFuelMassConsumed() const;
        double getTotalVolumeFuelConsumed() const;

        // Anti-lag: UI-toggleable, no-op without a turbocharger. Off-throttle
        // (plate mostly closed) gets heavy ignition retard plus a raw
        // fuel/air charge dumped into each header that pops off on its own;
        // still-on-throttle ("rolling") gets a lighter, continuous retard to
        // keep the turbine fed through a lift/shift without giving up much
        // drive torque. See Engine::updateAntiLag().
        void setAntiLagEnabled(bool enabled) { m_antiLagEnabled = enabled; }
        bool getAntiLagEnabled() const { return m_antiLagEnabled; }
        bool hasTurbocharger() const { return m_turbochargerCount > 0; }
        bool hasSupercharger() const { return m_superchargerCount > 0; }
        bool isAntiLagActive() const { return m_antiLagActive; }

        inline double getStarterTorque() const { return m_starterTorque; }
        inline double getStarterSpeed() const { return m_starterSpeed; }
        inline double getRedline() const { return m_redline; }
        inline double getDynoMinSpeed() const { return m_dynoMinSpeed; }
        inline double getDynoMaxSpeed() const { return m_dynoMaxSpeed; }
        inline double getDynoHoldStep() const { return m_dynoHoldStep; }

        int getCylinderBankCount() const { return m_cylinderBankCount; }
        int getCylinderCount() const { return m_cylinderCount; }
        int getCrankshaftCount() const { return m_crankshaftCount; }
        int getExhaustSystemCount() const { return m_exhaustSystemCount; }
        int getIntakeCount() const { return m_intakeCount; }
        int getTurbochargerCount() const { return m_turbochargerCount; }
        int getSuperchargerCount() const { return m_superchargerCount; }
        int getMaxDepth() const;

        Crankshaft *getCrankshaft(int i) const { return &m_crankshafts[i]; }
        CylinderBank *getCylinderBank(int i) const { return &m_cylinderBanks[i]; }
        CylinderHead *getHead(int i) const { return &m_heads[i]; }
        Piston *getPiston(int i) const { return &m_pistons[i]; }
        ConnectingRod *getConnectingRod(int i) const { return &m_connectingRods[i]; }
        IgnitionModule *getIgnitionModule() { return &m_ignitionModule; }
        ExhaustSystem *getExhaustSystem(int i) const { return &m_exhaustSystems[i]; }
        Intake *getIntake(int i) const { return &m_intakes[i]; }
        Turbocharger *getTurbocharger(int i) const { return &m_turbochargers[i]; }
        Supercharger *getSupercharger(int i) const { return &m_superchargers[i]; }
        CombustionChamber *getChamber(int i) const { return &m_combustionChambers[i]; }
        Fuel *getFuel() { return &m_fuel; }

        double getSimulationFrequency() const { return m_initialSimulationFrequency; }
        double getInitialHighFrequencyGain() const { return m_initialHighFrequencyGain; }
        double getInitialNoise() const { return m_initialNoise; }
        double getInitialJitter() const { return m_initialJitter; }

        virtual Simulator *createSimulator(Vehicle *vehicle, Transmission *transmission);

    protected:
        void updateAntiLag(double dt);

        bool m_antiLagEnabled = false;
        bool m_antiLagActive = false;

        // 0..1 envelope: snaps toward 1 almost immediately off-throttle,
        // but decays back toward 0 over time rather than instantly once
        // back on the gas. This *is* "rolling" anti-lag -- the built-up
        // charge/retard carries over and fades as throttle is reapplied,
        // instead of a hard cutoff dropping exhaust energy (and turbine
        // speed) the instant the driver tips back in.
        double m_antiLagLevel = 0.0;

        // Per-engine pop character -- see Parameters::antiLagMaxRetard etc.
        double m_antiLagMaxRetard = units::angle(55.0, units::deg);
        double m_antiLagDumpRateK = 6.0;
        double m_antiLagPopRate = 6.0;

        std::string m_name;

        Crankshaft *m_crankshafts;
        int m_crankshaftCount;

        CylinderBank *m_cylinderBanks;
        CylinderHead *m_heads;
        int m_cylinderBankCount;

        Piston *m_pistons;
        ConnectingRod *m_connectingRods;
        CombustionChamber *m_combustionChambers;
        int m_cylinderCount;

        double m_starterTorque;
        double m_starterSpeed;
        double m_redline;
        double m_dynoMinSpeed;
        double m_dynoMaxSpeed;
        double m_dynoHoldStep;

        double m_initialSimulationFrequency;
        double m_initialHighFrequencyGain;
        double m_initialNoise;
        double m_initialJitter;

        ExhaustSystem *m_exhaustSystems;
        int m_exhaustSystemCount;

        Intake *m_intakes;
        int m_intakeCount;

        Turbocharger *m_turbochargers;
        int m_turbochargerCount;

        Supercharger *m_superchargers;
        int m_superchargerCount;

        IgnitionModule m_ignitionModule;
        Fuel m_fuel;

        Throttle *m_throttle;

        double m_throttleValue;
        double m_displacement;
};

#endif /* ATG_ENGINE_SIM_ENGINE_H */
