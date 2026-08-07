#include "../include/intake.h"

#include "../include/turbocharger.h"
#include "../include/supercharger.h"
#include "../include/units.h"

#include <algorithm>
#include <cmath>

Intake::Intake() {
    m_inputFlowK = 0;
    m_idleFlowK = 0;
    m_flow = 0;
    m_throttle = 1.0;
    m_idleThrottlePlatePosition = 0.0;
    m_crossSectionArea = 0.0;
    m_flowRate = 0;
    m_totalFuelInjected = 0;
    m_molecularAfr = 0;
    m_runnerLength = 0;
    m_audioVolume = 0.0;
    m_impulseResponse = nullptr;
    m_index = 0;
}

Intake::~Intake() {
    /* void */
}

void Intake::initialize(Parameters &params) {
    const double width = std::sqrt(params.CrossSectionArea);
    m_system.initialize(
        units::pressure(1.0, units::atm),
        params.volume,
        units::celcius(25.0));
    m_system.setGeometry(
        width,
        params.volume / params.CrossSectionArea,
        1.0,
        0.0);

    m_atmosphere.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1000.0, units::m3),
        units::celcius(25.0));
    m_atmosphere.setGeometry(
        units::distance(100.0, units::m),
        units::distance(100.0, units::m),
        1.0,
        0.0);

    m_inputFlowK = params.InputFlowK;
    m_molecularAfr = params.MolecularAfr;
    m_idleFlowK = params.IdleFlowK;
    m_idleThrottlePlatePosition = params.IdleThrottlePlatePosition;
    m_runnerLength = params.RunnerLength;
    m_crossSectionArea = params.CrossSectionArea;
    m_velocityDecay = params.VelocityDecay;
    m_runnerFlowRate = params.RunnerFlowRate;
    m_audioVolume = params.AudioVolume;
    m_impulseResponse = params.ImpulseResponse;
}

void Intake::destroy() {
    /* void */
}

void Intake::addTurbocharger(Turbocharger *turbo) {
    if (turbo != nullptr) {
        m_turbos.push_back(turbo);
    }
}

void Intake::addSupercharger(Supercharger *supercharger) {
    if (supercharger != nullptr) {
        m_superchargers.push_back(supercharger);
    }
}

void Intake::process(double dt) {
    const double ideal_afr = 0.8 * m_molecularAfr * 4;
    const double current_afr = (m_system.mix().p_o2 + m_system.mix().p_inert) / m_system.mix().p_fuel;

    const double p_air = ideal_afr / (1 + ideal_afr);
    GasSystem::Mix fuelAirMix;
    fuelAirMix.p_fuel = 1 - p_air;
    fuelAirMix.p_inert = p_air * 0.75;
    fuelAirMix.p_o2 = p_air * 0.25;

    const double idle_afr = 2.0;
    const double p_idle_air = idle_afr / (1 + idle_afr);
    GasSystem::Mix fuelMix;
    fuelMix.p_fuel = (1.0 - p_idle_air);
    fuelMix.p_inert = p_idle_air * 0.75;
    fuelMix.p_o2 = p_idle_air * 0.25;

    const double throttle = getThrottlePlatePosition();
    const double flowAttenuation = std::cos(throttle * constants::pi / 2);

    GasSystem::FlowParameters flowParams;
    flowParams.crossSectionArea_0 = units::area(10, units::m2);
    flowParams.crossSectionArea_1 = m_crossSectionArea;
    flowParams.direction_x = 0.0;
    flowParams.direction_y = -1.0;
    flowParams.dt = dt;
    flowParams.system_0 = &m_atmosphere;
    flowParams.system_1 = &m_system;

    const int turboCount = (int)m_turbos.size();
    const int superchargerCount = (int)m_superchargers.size();
    const int boostSourceCount = turboCount + superchargerCount;

    m_flow = 0.0;
    double idleCircuitFlow = 0.0;

    if (boostSourceCount == 0) {
        const double sourcePressure = units::pressure(1.0, units::atm);
        const double sourceTemperature = units::celcius(25.0);

        m_atmosphere.reset(sourcePressure, sourceTemperature, fuelAirMix);
        flowParams.k_flow = flowAttenuation * m_inputFlowK;
        m_flow = m_system.flow(flowParams);

        m_atmosphere.reset(sourcePressure, sourceTemperature, fuelMix);
        flowParams.k_flow = m_idleFlowK;
        idleCircuitFlow = m_system.flow(flowParams);
    }
    else {
        // Multiple boost sources on one intake merge into this single
        // shared plenum, same as a real twin-turbo's compressor outputs
        // Y'd together upstream of a common throttle body -- a
        // supercharger's discharge merges in exactly the same way, and a
        // turbo and supercharger can both feed the same plenum at once
        // (compound forced induction) since this loop doesn't care which
        // kind produced the discharge it's draining.
        // ponytail: splits the plenum's own InputFlowK evenly across the
        // attached sources instead of modeling the merge junction itself --
        // keeps total inlet capacity equal to what the same InputFlowK
        // already means for a single-source engine. Fine for matched
        // sources (the normal case); upgrade to per-leg flow areas if
        // someone wants mismatched sizes.
        for (Turbocharger *turbo : m_turbos) {
            const double sourcePressure = turbo->getCompressorDischargePressure();
            const double sourceTemperature = turbo->getCompressorDischargeTemperature();

            // A turbo's charge pipe is a real, small, finite volume --
            // size the scratch system to match so GasSystem::flow()'s own
            // source->n()-based flow cap actually reflects how much this
            // turbo's discharge can supply, instead of computing flow
            // against a fictional infinite source and separately
            // hard-clamping the real (tiny) discharge afterward.
            m_atmosphere.setVolume(turbo->getCompressorDischargeSystem()->volume());

            m_atmosphere.reset(sourcePressure, sourceTemperature, fuelAirMix);
            flowParams.k_flow = (flowAttenuation * m_inputFlowK) / boostSourceCount;
            const double turboFlow = m_system.flow(flowParams);
            m_flow += turboFlow;

            m_atmosphere.reset(sourcePressure, sourceTemperature, fuelMix);
            flowParams.k_flow = m_idleFlowK / boostSourceCount;
            const double turboIdleFlow = m_system.flow(flowParams);
            idleCircuitFlow += turboIdleFlow;

            // Draw down this turbo's own discharge by the air fraction of
            // what was just metered from it specifically, so the throttle
            // plate genuinely depletes the plenum the surge model is
            // fighting to keep pressurized.
            GasSystem *discharge = turbo->getCompressorDischargeSystem();
            if (discharge->n() > 0) {
                const double drawn = std::max(0.0, turboFlow) * p_air
                    + std::max(0.0, turboIdleFlow) * p_idle_air;
                discharge->loseN(std::min(drawn, 0.9 * discharge->n()), discharge->kineticEnergyPerMol());
            }
        }

        for (Supercharger *supercharger : m_superchargers) {
            const double sourcePressure = supercharger->getCompressorDischargePressure();
            const double sourceTemperature = supercharger->getCompressorDischargeTemperature();

            m_atmosphere.setVolume(supercharger->getCompressorDischargeSystem()->volume());

            m_atmosphere.reset(sourcePressure, sourceTemperature, fuelAirMix);
            flowParams.k_flow = (flowAttenuation * m_inputFlowK) / boostSourceCount;
            const double scFlow = m_system.flow(flowParams);
            m_flow += scFlow;

            m_atmosphere.reset(sourcePressure, sourceTemperature, fuelMix);
            flowParams.k_flow = m_idleFlowK / boostSourceCount;
            const double scIdleFlow = m_system.flow(flowParams);
            idleCircuitFlow += scIdleFlow;

            GasSystem *discharge = supercharger->getCompressorDischargeSystem();
            if (discharge->n() > 0) {
                const double drawn = std::max(0.0, scFlow) * p_air
                    + std::max(0.0, scIdleFlow) * p_idle_air;
                discharge->loseN(std::min(drawn, 0.9 * discharge->n()), discharge->kineticEnergyPerMol());
            }
        }
    }

    m_system.dissipateExcessVelocity();
    m_system.updateVelocity(dt, m_velocityDecay);

    if (m_flow > 0) {
        m_totalFuelInjected += fuelAirMix.p_fuel * m_flow;
    }

    if (idleCircuitFlow > 0) {
        m_totalFuelInjected += fuelMix.p_fuel * idleCircuitFlow;
    }
}
