#ifndef ATG_ENGINE_SIM_COMBUSTION_CHAMBER_H
#define ATG_ENGINE_SIM_COMBUSTION_CHAMBER_H

#include "scs.h"

#include "piston.h"
#include "gas_system.h"
#include "cylinder_head.h"
#include "units.h"
#include "fuel.h"

class Engine;
class CombustionChamber : public atg_scs::ForceGenerator {
    public:
        struct Parameters {
            Piston *Piston;
            CylinderHead *Head;
            Fuel *Fuel;
            Function *MeanPistonSpeedToTurbulence;

            double StartingPressure;
            double StartingTemperature;
            double CrankcasePressure;
        };

        struct FlameEvent {
            double lit_n;
            double total_n;
            double percentageLit;
            double efficiency;
            double flameSpeed;

            double lastVolume;
            double travel_x;
            double travel_y;
            GasSystem::Mix globalMix;
        };

        struct FrictionModelParams {
            double frictionCoeff = 0.06;
            double breakawayFriction = units::force(50, units::N);
            double breakawayFrictionVelocity = units::distance(0.1, units::m);
            double viscousFrictionCoefficient = units::force(20, units::N);
        };

    public:
        CombustionChamber();
        virtual ~CombustionChamber();

        void initialize(const Parameters &params);
        void setEngine(Engine *engine) { m_engine = engine; }
        virtual void apply(atg_scs::SystemState *system);

        CylinderHead *getCylinderHead() const { return m_head; }
        Piston *getPiston() const { return m_piston; }

        double getFrictionForce() const;
        double getVolume() const;
        double pistonSpeed() const;
        double calculateMeanPistonSpeed() const;
        double calculateFiringPressure() const;

        bool isLit() const { return m_lit; }
        bool popLitLastFrame();

        void ignite();
        void start();
        void update(double dt);
        void flow(double dt);
        void end();

        double lastEventAfr() const;

        double getLastIterationExhaustFlow() const { return m_exhaustFlow; }

        void resetLastTimestepExhaustFlow() { m_lastTimestepTotalExhaustFlow = 0; }
        double getLastTimestepExhaustFlow() const { return m_lastTimestepTotalExhaustFlow; }

        Function *m_meanPistonSpeedToTurbulence;
        GasSystem m_system;
        FlameEvent m_flameEvent;
        bool m_lit;

        FrictionModelParams m_frictionModel;

        double m_peakTemperature;
        double m_nBurntFuel;

    protected:
        double calculateFrictionForce(double v) const;
        void updateCycleStates();

        double m_intakeFlowRate;
        double m_exhaustFlowRate;

        double m_lastTimestepTotalExhaustFlow;
        double m_exhaustFlow;

        double m_crankcasePressure;

        double *m_pressure;
        double *m_pistonSpeed;
        static constexpr int StateSamples = 256;

        bool m_litLastFrame;

        Piston *m_piston;
        CylinderHead *m_head;
        Engine *m_engine;

        Fuel *m_fuel;
};

#endif /* ATG_ENGINE_SIM_COMBUSTION_CHAMBER_H */
