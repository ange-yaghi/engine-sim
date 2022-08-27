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

#include <string>

class Engine : public Part {
    public:
        struct Parameters {
            int CylinderBanks;
            int CylinderCount;
            int CrankshaftCount;
            int ExhaustSystemCount;
            int IntakeCount;

            std::string Name;

            double StarterTorque = units::torque(90.0, units::ft_lb);
            double StarterSpeed = units::rpm(200);
            double Redline = units::rpm(6500);

            Throttle *throttle;

            double initialSimulationFrequency;
            double initialHighFrequencyGain;
            double initialNoise;
            double initialJitter;
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

        inline double getStarterTorque() const { return m_starterTorque; }
        inline double getStarterSpeed() const { return m_starterSpeed; }
        inline double getRedline() const { return m_redline; }

        int getCylinderBankCount() const { return m_cylinderBankCount; }
        int getCylinderCount() const { return m_cylinderCount; }
        int getCrankshaftCount() const { return m_crankshaftCount; }
        int getExhaustSystemCount() const { return m_exhaustSystemCount; }
        int getIntakeCount() const { return m_intakeCount; }
        int getMaxDepth() const;

        Crankshaft *getCrankshaft(int i) const { return &m_crankshafts[i]; }
        CylinderBank *getCylinderBank(int i) const { return &m_cylinderBanks[i]; }
        CylinderHead *getHead(int i) const { return &m_heads[i]; }
        Piston *getPiston(int i) const { return &m_pistons[i]; }
        ConnectingRod *getConnectingRod(int i) const { return &m_connectingRods[i]; }
        IgnitionModule *getIgnitionModule() { return &m_ignitionModule; }
        ExhaustSystem *getExhaustSystem(int i) const { return &m_exhaustSystems[i]; }
        Intake *getIntake(int i) const { return &m_intakes[i]; }
        CombustionChamber *getChamber(int i) const { return &m_combustionChambers[i]; }
        Fuel *getFuel() { return &m_fuel; }

        double getSimulationFrequency() const { return m_initialSimulationFrequency; }
        double getInitialHighFrequencyGain() const { return m_initialHighFrequencyGain; }
        double getInitialNoise() const { return m_initialNoise; }
        double getInitialJitter() const { return m_initialJitter; }

    protected:
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

        double m_initialSimulationFrequency;
        double m_initialHighFrequencyGain;
        double m_initialNoise;
        double m_initialJitter;

        ExhaustSystem *m_exhaustSystems;
        int m_exhaustSystemCount;

        Intake *m_intakes;
        int m_intakeCount;

        IgnitionModule m_ignitionModule;
        Fuel m_fuel;

        Throttle *m_throttle;

        double m_throttleValue;
        double m_displacement;
};

#endif /* ATG_ENGINE_SIM_ENGINE_H */
