#ifndef ATG_ENGINE_SIM_INTAKE_H
#define ATG_ENGINE_SIM_INTAKE_H

#include "part.h"

#include "gas_system.h"
#include "impulse_response.h"

#include <vector>

class Turbocharger;
class Supercharger;

class Intake : public Part {
    friend class Engine;

    public:
        struct Parameters {
            // Plenum volume
            double volume;

            // Plenum dimensions
            double CrossSectionArea;

            // Input flow constant
            double InputFlowK;

            // Idle-circuit flow constant
            double IdleFlowK;

            // Flow rate from plenum to runner
            double RunnerFlowRate;

            // Molecular air fuel ratio (defaults to ideal for octane)
            double MolecularAfr = (25.0 / 2.0);

            // Throttle plate position at idle
            double IdleThrottlePlatePosition = 0.975;

            // Runner volume
            double RunnerLength = units::distance(4.0, units::inch);

            // Velocity decay factor
            double VelocityDecay = 0.5;

            // Intake-noise audio channel -- mirrors ExhaustSystem's
            // AudioVolume/ImpulseResponse. 0 volume (the default) makes
            // this intake inaudible regardless of ImpulseResponse.
            double AudioVolume = 0.0;
            ImpulseResponse *ImpulseResponse = nullptr;
        };

    public:
        Intake();
        virtual ~Intake();

        void initialize(Parameters &params);
        virtual void destroy();

        void process(double dt);

        // When one or more are attached, the intake draws from their
        // compressor discharges (elevated pressure/temperature) instead of
        // flat atmosphere. Multiple turbos on one intake merge into the
        // same shared plenum -- the real topology for a twin-turbo engine
        // whose compressor outputs Y together upstream of a common
        // throttle body (2JZ-GTE, RB26DETT), rather than each turbo
        // needing its own separate intake.
        void addTurbocharger(Turbocharger *turbo);

        // Same merge behavior as addTurbocharger(), for a belt/gear-driven
        // supercharger's compressor discharge instead of a turbo's. Both
        // lists can be non-empty at once (compound forced induction) --
        // they're combined into the same shared plenum either way.
        void addSupercharger(Supercharger *supercharger);

        inline double getRunnerFlowRate() const { return m_runnerFlowRate; }
        inline double getThrottlePlatePosition() const { return m_idleThrottlePlatePosition * m_throttle; }
        inline double getRunnerLength() const { return m_runnerLength; }
        inline double getPlenumCrossSectionArea() const { return m_crossSectionArea; }
        inline double getVelocityDecay() const { return m_velocityDecay; }
        inline double getAudioVolume() const { return m_audioVolume; }
        inline ImpulseResponse *getImpulseResponse() const { return m_impulseResponse; }
        inline int getIndex() const { return m_index; }

        GasSystem m_system;
        double m_throttle;

        double m_flow;
        double m_flowRate;
        double m_totalFuelInjected;

    protected:
        double m_crossSectionArea;
        double m_inputFlowK;
        double m_idleFlowK;
        double m_runnerFlowRate;
        double m_molecularAfr;
        double m_idleThrottlePlatePosition;
        double m_runnerLength;
        double m_velocityDecay;
        double m_audioVolume;
        ImpulseResponse *m_impulseResponse;
        int m_index;

        std::vector<Turbocharger *> m_turbos;
        std::vector<Supercharger *> m_superchargers;

        GasSystem m_atmosphere;
};

#endif /* ATG_ENGINE_SIM_INTAKE_H */
