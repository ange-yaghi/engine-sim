#include "../include/gas_system.h"

#include "../include/units.h"
#include "../include/utilities.h"

#include <cmath>
#include <cassert>

void GasSystem::start() {
    //m_next = m_state;
}

void GasSystem::end() {
    //m_state = m_next;

    m_state.E_k = std::fmax(m_state.E_k, 0.0);
    m_state.n_mol = std::fmax(m_state.n_mol, 0.0);
    m_state.V = std::fmax(m_state.V, 0.0);
}

void GasSystem::initialize(double P, double V, double T, const Mix &mix, int degreesOfFreedom) {
    m_degreesOfFreedom = degreesOfFreedom;
    m_state.n_mol = P * V / (constants::R * T);
    m_state.V = V;
    m_state.E_k = T * (0.5 * degreesOfFreedom * m_state.n_mol * constants::R);
    m_state.mix = mix;
    m_state.momentum[0] = m_state.momentum[1] = 0;

    const double hcr = heatCapacityRatio();
    m_chokedFlowLimit = chokedFlowLimit(degreesOfFreedom);
    m_chokedFlowFactorCached = chokedFlowRate(degreesOfFreedom);
}

void GasSystem::reset(double P, double T, const Mix &mix) {
    m_state.n_mol = P * volume() / (constants::R * T);
    m_state.E_k = T * (0.5 * m_degreesOfFreedom * m_state.n_mol * constants::R);
    m_state.mix = mix;
    m_state.momentum[0] = m_state.momentum[1] = 0;
}

void GasSystem::setVolume(double V) {
    return changeVolume(V - m_state.V);
}

void GasSystem::setN(double n) {
    m_state.E_k = kineticEnergy(n);
    m_state.n_mol = n;
}

void GasSystem::changeVolume(double dV) {
    const double V = this->volume();
    const double L = std::pow(V + dV, 1 / 3.0);
    const double surfaceArea = (L * L);
    const double dL = -dV / surfaceArea;
    const double W = dL * pressure() * surfaceArea;

    m_state.V += dV;
    m_state.E_k += W;
}

void GasSystem::changePressure(double dP) {
    m_state.E_k += dP * volume() * m_degreesOfFreedom * 0.5;
}

void GasSystem::changeTemperature(double dT) {
    m_state.E_k += dT * 0.5 * m_degreesOfFreedom * n() * constants::R;
}

void GasSystem::changeEnergy(double dE) {
    m_state.E_k += dE;
}

void GasSystem::changeMix(const Mix &mix) {
    m_state.mix = mix;
}

void GasSystem::injectFuel(double n) {
    const double n_fuel = this->n_fuel() + n;
    const double p_fuel = n_fuel / this->n();
    m_state.mix.p_fuel = p_fuel;
}

void GasSystem::changeTemperature(double dT, double n) {
    m_state.E_k += dT * 0.5 * m_degreesOfFreedom * n * constants::R;
}

double GasSystem::react(double n, const Mix &mix) {
    const double l_n_fuel = mix.p_fuel * n;
    const double l_n_o2 = mix.p_o2 * n;

    const double system_n_fuel = n_fuel();
    const double system_n_o2 = n_o2();
    const double system_n_inert = n_inert();
    const double system_n = this->n();

    // Assuming the following reaction:
    // 25[O2] + 2[C8H16] -> 16[CO2] + 18[H2O]
    constexpr double ideal_o2_ratio = 25.0 / 2;
    constexpr double ideal_fuel_ratio = 2.0 / 25;
    constexpr double output_input_ratio = (16.0 + 18.0) / (25 + 2);

    const double ideal_fuel_n = ideal_fuel_ratio * l_n_o2;
    const double ideal_o2_n = ideal_o2_ratio * l_n_fuel;
    
    const double a_n_fuel = std::fmin(
        std::fmin(system_n_fuel, l_n_fuel),
        ideal_fuel_n);
    const double a_n_o2 = std::fmin(
        std::fmin(system_n_o2, l_n_o2),
        ideal_o2_n);

    const double reactants_n = a_n_fuel + a_n_o2;
    const double products_n = output_input_ratio * reactants_n;
    const double dn = products_n - reactants_n;

    m_state.n_mol += dn;

    // Adjust mix
    const double new_system_n_fuel = system_n_fuel - a_n_fuel;
    const double new_system_n_o2 = system_n_o2 - a_n_o2;
    const double new_system_n_inert = system_n_inert + products_n;
    const double new_system_n = system_n + dn;

    if (new_system_n != 0) {
        m_state.mix.p_fuel = new_system_n_fuel / new_system_n;
        m_state.mix.p_inert = new_system_n_inert / new_system_n;
        m_state.mix.p_o2 = new_system_n_o2 / new_system_n;
    }
    else {
        m_state.mix.p_fuel = m_state.mix.p_inert = m_state.mix.p_o2 = 0;
    }

    return a_n_fuel;
}

double GasSystem::flowConstant(
    double targetFlowRate,
    double P,
    double pressureDrop,
    double T,
    double hcr)
{
    const double T_0 = T;
    const double p_0 = P, p_T = P - pressureDrop; // p_0 = upstream pressure

    const double chokedFlowLimit =
        std::pow((2.0 / (hcr + 1)), hcr / (hcr - 1));
    const double p_ratio = p_T / p_0;

    double flowRate = 0;
    if (p_ratio <= chokedFlowLimit) {
        // Choked flow
        flowRate = std::sqrt(hcr);
        flowRate *= std::pow(2 / (hcr + 1), (hcr + 1) / (2 * (hcr - 1)));
    }
    else {
        flowRate = (2 * hcr) / (hcr - 1);
        flowRate *= (1 - std::pow(p_ratio, (hcr - 1) / hcr));
        flowRate = std::sqrt(flowRate);
        flowRate *= std::pow(p_ratio, 1 / hcr);
    }

    flowRate *= p_0 / std::sqrt(constants::R * T_0);

    return targetFlowRate / flowRate;
}

double GasSystem::k_28inH2O(double flowRateScfm) {
    return flowConstant(
        units::flow(flowRateScfm, units::scfm),
        units::pressure(1.0, units::atm),
        units::pressure(28.0, units::inH2O),
        units::celcius(25),
        heatCapacityRatio(5)
    );
}

double GasSystem::k_carb(double flowRateScfm) {
    return flowConstant(
        units::flow(flowRateScfm, units::scfm),
        units::pressure(1.0, units::atm),
        units::pressure(1.5, units::inHg),
        units::celcius(25),
        heatCapacityRatio(5)
    );
}

double GasSystem::flowRate(
    double k_flow,
    double P0,
    double P1,
    double T0,
    double T1,
    double hcr,
    double chokedFlowLimit,
    double chokedFlowRateCached)
{
    if (k_flow == 0) return 0;

    double direction;
    double T_0;
    double p_0, p_T; // p_0 = upstream pressure
    if (P0 > P1) {
        direction = 1.0;
        T_0 = T0;
        p_0 = P0;
        p_T = P1;
    }
    else {
        direction = -1.0;
        T_0 = T1;
        p_0 = P1;
        p_T = P0;
    }

    const double p_ratio = p_T / p_0;
    double flowRate = 0;
    if (p_ratio <= chokedFlowLimit) {
        // Choked flow
        flowRate = chokedFlowRateCached;
    }
    else {
        flowRate = (2 * hcr) / (hcr - 1);
        flowRate *= (1 - std::pow(p_ratio, (hcr - 1) / hcr));
        flowRate = std::sqrt(std::fmax(flowRate, 0.0));
        flowRate *= std::pow(p_ratio, 1 / hcr);
    }

    flowRate *= direction * p_0;
    flowRate /= std::sqrt(constants::R * T_0);

    return flowRate * k_flow;
}

double GasSystem::flow(double dn, double E_k_per_mol, const Mix &mix) {
    if (dn >= 0) {
        return loseN(dn);
    }
    else {
        return gainN(-dn, E_k_per_mol, mix);
    }
}

double GasSystem::loseN(double dn) {
    m_state.E_k -= kineticEnergy(dn);
    m_state.n_mol -= dn;

    if (m_state.n_mol < 0) {
        m_state.n_mol = 0;
    }

    return dn;
}

double GasSystem::gainN(double dn, double E_k_per_mol, const Mix &mix) {
    const double next_n = m_state.n_mol + dn;
    const double current_n = m_state.n_mol;

    m_state.E_k += dn * E_k_per_mol;
    m_state.n_mol = next_n;

    if (next_n != 0) {
        m_state.mix.p_fuel = (m_state.mix.p_fuel * current_n + dn * mix.p_fuel) / next_n;
        m_state.mix.p_inert = (m_state.mix.p_inert * current_n + dn * mix.p_inert) / next_n;
        m_state.mix.p_o2 = (m_state.mix.p_o2 * current_n + dn * mix.p_o2) / next_n;
    }
    else {
        m_state.mix.p_fuel = m_state.mix.p_inert = m_state.mix.p_o2 = 0;
    }

    return -dn;
}

void GasSystem::velocityWall(
    double dt,
    double timeConstant,
    double dx,
    double dy)
{
    if (n() == 0) return;
    if (dx * m_state.momentum[0] + dy * m_state.momentum[1] < 0) return;

    const double p_dx = -dy;
    const double p_dy = dx;

    const double invMass = 1.0 / mass();
    const double velocity_x = m_state.momentum[0] * invMass;
    const double velocity_y = m_state.momentum[1] * invMass;
    const double velocity_squared =
        velocity_x * velocity_x + velocity_y * velocity_y;

    const double momentum_dir = dx * m_state.momentum[0] + dy * m_state.momentum[1];
    const double momentum_pdir = p_dx * m_state.momentum[0] + p_dy * m_state.momentum[1];

    const double s = dt / (dt + timeConstant);
    const double momentum_dx = dx * m_state.momentum[0] * (1 - s);
    const double momentum_dy = dy * m_state.momentum[1] * (1 - s);
    
    m_state.momentum[0] = momentum_dx * dx + (momentum_pdir * p_dx);
    m_state.momentum[1] = momentum_dy * dy + (momentum_pdir * p_dy);

    const double newVelocity_x = m_state.momentum[0] * invMass;
    const double newVelocity_y = m_state.momentum[1] * invMass;
    const double newVelocity_squared =
        newVelocity_x * newVelocity_x + newVelocity_y * newVelocity_y;

    const double dE_k = 0.5 * mass() * (velocity_squared - newVelocity_squared);
    m_state.E_k += dE_k;
}

void GasSystem::dissipateVelocity(double dt, double timeConstant) {
    if (n() == 0) return;

    const double invMass = 1.0 / mass();
    const double velocity_x = m_state.momentum[0] * invMass;
    const double velocity_y = m_state.momentum[1] * invMass;
    const double velocity_squared =
        velocity_x * velocity_x + velocity_y * velocity_y;

    const double s = dt / (dt + timeConstant);
    m_state.momentum[0] = m_state.momentum[0] * (1 - s);
    m_state.momentum[1] = m_state.momentum[1] * (1 - s);

    const double newVelocity_x = m_state.momentum[0] * invMass;
    const double newVelocity_y = m_state.momentum[1] * invMass;
    const double newVelocity_squared =
        newVelocity_x * newVelocity_x + newVelocity_y * newVelocity_y;

    const double dE_k = 0.5 * mass() * (velocity_squared - newVelocity_squared);
    m_state.E_k += dE_k;
}

double GasSystem::flow(const FlowParameters &params) {
    GasSystem *source = nullptr, *sink = nullptr;
    double sourcePressure = 0, sinkPressure = 0;
    double dx, dy;
    double sourceCrossSection = 0, sinkCrossSection = 0;
    double direction = 0;

    const double P_0 =
        params.system_0->pressure()
        + params.system_0->dynamicPressure(params.direction_x, params.direction_y);
    const double P_1 =
        params.system_1->pressure()
        + params.system_1->dynamicPressure(-params.direction_x, -params.direction_y);

    if (P_0 > P_1) {
        dx = params.direction_x;
        dy = params.direction_y;
        source = params.system_0;
        sink = params.system_1;
        sourcePressure = P_0;
        sinkPressure = P_1;
        sourceCrossSection = params.crossSectionArea_0;
        sinkCrossSection = params.crossSectionArea_1;
        direction = 1.0;
    }
    else {
        dx = -params.direction_x;
        dy = -params.direction_y;
        source = params.system_1;
        sink = params.system_0;
        sourcePressure = P_1;
        sinkPressure = P_0;
        sourceCrossSection = params.crossSectionArea_1;
        sinkCrossSection = params.crossSectionArea_0;
        direction = -1.0;
    }

    double flow = params.dt * flowRate(
        params.k_flow,
        sourcePressure,
        sinkPressure,
        source->temperature(),
        sink->temperature(),
        source->heatCapacityRatio(),
        source->m_chokedFlowLimit,
        source->m_chokedFlowFactorCached);

    flow = clamp(flow, 0.0, source->n());

    /*
    const double fraction = flow / source->n();
    const double fractionVolume = fraction * source->volume();
    const double invMass = 1 / source->mass();
    const double fractionMass = fraction * source->mass();

    const double sourceVelocity_x_0 = invMass * source->m_state.momentum[0];
    const double sourceVelocity_y_0 = invMass * source->m_state.momentum[1];
    const double sourceVelocity_0_squared =
        sourceVelocity_x_0 * sourceVelocity_x_0 + sourceVelocity_y_0 * sourceVelocity_y_0;

    const double sourceFlowVelocity_x_0 = dx * (fractionVolume / sourceCrossSection) / params.dt;
    const double sourceFlowVelocity_y_0 = dy * (fractionVolume / sourceCrossSection) / params.dt;
    const double sourceFlowVelocity_0_squared =
        sourceFlowVelocity_x_0 * sourceFlowVelocity_x_0 + sourceFlowVelocity_y_0 * sourceFlowVelocity_y_0;

    const double sinkVelocity = (fractionVolume / sinkCrossSection) / params.dt;
    const double sinkVelocity_squared = sinkVelocity * sinkVelocity;
    const double sinkVelocity_x = sinkVelocity * dx;
    const double sinkVelocity_y = sinkVelocity * dy;

    // Energy needed to correct source flow velocity
    source->m_next.E_k -=
        0.5 * fractionMass * (sourceFlowVelocity_0_squared - sourceVelocity_0_squared);
    source->m_next.momentum[0] += fractionMass * (sourceFlowVelocity_x_0 - sourceVelocity_x_0);
    source->m_next.momentum[1] += fractionMass * (sourceFlowVelocity_y_0 - sourceVelocity_y_0);

    // Energy correction to sink
    sink->m_next.E_k -=
        0.5 * fractionMass * (sinkVelocity_squared - sourceFlowVelocity_0_squared);
    sink->m_next.momentum[0] += fractionMass * sinkVelocity_x;
    sink->m_next.momentum[1] += fractionMass * sinkVelocity_y;

    source->loseN(flow);
    sink->gainN(flow, source->kineticEnergyPerMol(), source->mix());
    */

    // - Stage 1
    // The flow fraction is accelerated to the flow velocity appropriate for the
    // source vessel using the vessel's molecular kinetic energy.
    /*
    const double sourceVelocity_x = invMass * source->m_state.momentum[0];
    const double sourceVelocity_y = invMass * source->m_state.momentum[1];
    const double sourceVelocity_squared =
        sourceVelocity_x * sourceVelocity_x + sourceVelocity_y * sourceVelocity_y;

    const double sourceFlowVelocity = (fractionVolume / sourceCrossSection) / params.dt;
    const double sourceFlowVelocity_x = dx * sourceFlowVelocity;
    const double sourceFlowVelocity_y = dy * sourceFlowVelocity;
    const double sourceFlowVelocity_squared = sourceFlowVelocity * sourceFlowVelocity;

    const double E_k_sourceFlowVelocity =
        0.5 * fractionMass * (sourceFlowVelocity_squared - sourceVelocity_squared);

    source->m_next.E_k -= E_k_sourceFlowVelocity;
    */

    /*
    double flowVelocity = 0;
    double flowVelocity_x = 0;
    double flowVelocity_y = 0;

    // - Stage 1
    // The flow fraction is accelerated to the flow velocity appropriate for the
    // source vessel. This is modeled as an inelastic collision between the flow
    // fraction and the remaining gas in the source vessel. 
    flowVelocity = (fractionVolume / sourceCrossSection) / params.dt;
    flowVelocity_x = dx * flowVelocity;
    flowVelocity_y = dy * flowVelocity;
    const double flowVelocity_squared = flowVelocity * flowVelocity;
    const double sourceFlowMomentum_x = sourceFlowVelocity_x * fractionMass;
    const double sourceFlowMomentum_y = sourceFlowVelocity_y * fractionMass;

    const double sourceRemainderMass = (1 - fraction) * source->mass();
    const double sourceRemainderMomentum_x_0 = (1 - fraction) * source->m_state.momentum[0];
    const double sourceRemainderMomentum_y_0 = (1 - fraction) * source->m_state.momentum[1];

    const double totalMomentum_x = sourceFlowVelocity_x * source->mass();
    const double totalMomentum_y = sourceFlowVelocity_y * source->mass();

    const double sourceRemainderMomentum_x_1 =
        totalMomentum_x - sourceFlowMomentum_x;
    const double sourceRemainderMomentum_y_1 =
        totalMomentum_y - sourceFlowMomentum_y;

    source->m_next.momentum[0] += sourceRemainderMomentum_x_1 - sourceRemainderMomentum_x_0;
    source->m_next.momentum[1] += sourceRemainderMomentum_y_1 - sourceRemainderMomentum_y_0;



    // - Stage 2
    // Mass transfer between source and sink vessel.
    source->loseN(flow);
    sink->gainN(flow, source->kineticEnergyPerMol(), source->mix());

    source->m_next.momentum[0] -= source->m_state.momentum[0] * fraction;
    source->m_next.momentum[1] -= source->m_state.momentum[1] * fraction;

    // - Stage 3
    // The flow fraction is accelerated to the flow velocity appropriate for the
    // sink vessel using the source vessel's molecular kinetic energy.
    const double sinkFractionVelocity = (fractionVolume / sinkCrossSection) / params.dt;
    const double sinkFractionVelocity_squared = sinkFractionVelocity * sinkFractionVelocity;
    const double sinkFractionVelocity_x = sinkFractionVelocity * dx;
    const double sinkFractionVelocity_y = sinkFractionVelocity * dy;

    const double E_k_fractionSourceFlow =
        0.5 * fractionMass * sourceFlowVelocity_squared;
    const double E_k_sinkFractionVelocity =
        0.5 * fractionMass * sinkFractionVelocity_squared;

    source->m_next.E_k -= (E_k_sinkFractionVelocity - E_k_fractionSourceFlow);

    // - Stage 4
    // Flow fraction collides with gas in sink vessel. Momentum is conserved
    // and residual energy is converted to molecular kinetic energy.
    const double fractionMomentum_x = sinkFractionVelocity_x * fractionMass;
    const double fractionMomentum_y = sinkFractionVelocity_y * fractionMass;

    const double newSinkMass = sink->mass() + fractionMass;
    const double newSinkMomentum_x = sink->m_state.momentum[0] + fractionMomentum_x;
    const double newSinkMomentum_y = sink->m_state.momentum[1] + fractionMomentum_y;

    const double newSinkVelocity_x = newSinkMomentum_x / newSinkMass;
    const double newSinkVelocity_y = newSinkMomentum_y / newSinkMass;
    const double newSinkVelocity_squared =
        newSinkVelocity_x * newSinkVelocity_x + newSinkVelocity_y * newSinkVelocity_y;

    const double sinkVelocity_x = sink->m_state.momentum[0] / sink->mass();
    const double sinkVelocity_y = sink->m_state.momentum[1] / sink->mass();
    const double sinkVelocity_squared =
        sinkVelocity_x * sinkVelocity_x + sinkVelocity_y * sinkVelocity_y;

    const double E_k_sinkVelocity =
        0.5 * sink->mass() * sinkVelocity_squared;
    const double E_k_newSinkVelocity =
        0.5 * newSinkMass * newSinkVelocity_squared;
    const double E_k_sinkVelocityResidual =
        (E_k_sinkVelocity + E_k_sinkFractionVelocity) - E_k_newSinkVelocity;

    sink->m_next.E_k += E_k_sinkVelocityResidual;
    sink->m_next.momentum[0] += fractionMomentum_x;
    sink->m_next.momentum[1] += fractionMomentum_y;
    */

/*
    FlowState state;
    state.fractions[0].E_k_mol = source->kineticEnergy();
    state.fractions[0].mass = source->mass();
    state.fractions[0].momentum[0] = source->m_state.momentum[0];
    state.fractions[0].momentum[1] = source->m_state.momentum[1];

    state.fractions[1].E_k_mol = 0;
    state.fractions[1].mass = 0;
    state.fractions[1].momentum[0] = 0;
    state.fractions[1].momentum[1] = 0;

    state.fractions[2].E_k_mol = sink->kineticEnergy();
    state.fractions[2].mass = sink->mass();
    state.fractions[2].momentum[0] = sink->m_state.momentum[0];
    state.fractions[2].momentum[1] = sink->m_state.momentum[1];

    const double sourceFlow = (fractionVolume / sourceCrossSection) / params.dt;
    flowVelocity_x = dx * flowVelocity;
    flowVelocity_y = dy * flowVelocity;
    state.changeVelocity
    */

    // distance_moved = (1 - s) * sourceFlowDistance
    // velocity = (1 - s) * sourceFlowDistance / dt
    // avg_velocity = integrate(velocity, 0, 1)
    //              = (sourceFlowDistance / dt) * integrate(1 - s, 0, 1)
    //              = (sourceFlowDistance / dt) * (s - 0.5 * s^2)[0, 1]
    //              = (sourceFlowDistance / dt) * 0.5

    //const double sourceFlowDistance = (fractionVolume / sourceCrossSection);
    //const double sourceTotalDepth = (source->volume() / sourceCrossSection);
    //const double sourceFlowVelocity_x = dx * sourceFlowVelocity;
    //const double sourceFlowVelocity_y = dy * sourceFlowVelocity;
    //const double sourceFlowVelocity_squared = sourceFlowVelocity * sourceFlowVelocity;

    const double fraction = flow / source->n();
    const double fractionVolume = fraction * source->volume();
    const double invMass = 1 / source->mass();
    const double fractionMass = fraction * source->mass();
    const double remainingMass = (1 - fraction) * source->mass();
    const double sinkMass = sink->mass();

    const double c_source = source->c();
    const double c_sink = sink->c();

    const double acc_s = params.dt / (params.accelerationTimeConstant + params.dt);

    if (flow != 0) {
        // - Stage 1
        // Fraction flows from source to sink.

        // Convert fraction kinetic energy to molecular kinetic energy for easier
        // accounting later
        const double fractionInitialVelocity_x = fraction * source->m_state.momentum[0] / fractionMass;
        const double fractionInitialVelocity_y = fraction * source->m_state.momentum[1] / fractionMass;
        const double fractionInitialVelocity_squared =
            fractionInitialVelocity_x * fractionInitialVelocity_x
            + fractionInitialVelocity_y * fractionInitialVelocity_y;
        sink->m_state.E_k += 0.5 * fractionMass * fractionInitialVelocity_squared;

        source->m_state.momentum[0] -= fraction * source->m_state.momentum[0];
        source->m_state.momentum[1] -= fraction * source->m_state.momentum[1];

        source->loseN(flow);
        sink->gainN(flow, source->kineticEnergyPerMol(), source->mix());
    }

    // - Stage 2
    // Remaining source mass accelerates to fill missing space converting some
    // molecular kinetic energy.
    const double sourceVelocity =
        0.5 * clamp((fractionVolume / sourceCrossSection) / params.dt, 0.0, c_source);
    const double sourceVelocity_x = dx * sourceVelocity;
    const double sourceVelocity_y = dy * sourceVelocity;
    const double sourceVelocity_squared = sourceVelocity * sourceVelocity;
    const double sourceMomentum_x = sourceVelocity_x * remainingMass;
    const double sourceMomentum_y = sourceVelocity_y * remainingMass;

    const double initialVelocity_x = source->m_state.momentum[0] / remainingMass;
    const double initialVelocity_y = source->m_state.momentum[1] / remainingMass;
    const double initialVelocity_squared =
        initialVelocity_x * initialVelocity_x + initialVelocity_y * initialVelocity_y;

    const double finalVelocity_x = acc_s * sourceVelocity_x + (1 - acc_s) * initialVelocity_x;
    const double finalVelocity_y = acc_s * sourceVelocity_y + (1 - acc_s) * initialVelocity_y;
    const double finalVelocity_squared =
        finalVelocity_x * finalVelocity_x + finalVelocity_y * finalVelocity_y;

    const double E_k_sourceInitial = 0.5 * remainingMass * initialVelocity_squared;
    const double E_k_sourceFinal = 0.5 * remainingMass * finalVelocity_squared;

    source->m_state.E_k -= (E_k_sourceFinal - E_k_sourceInitial);
    source->m_state.momentum[0] = finalVelocity_x * remainingMass;
    source->m_state.momentum[1] = finalVelocity_y * remainingMass;

    // - Stage 3
    // Moving airflow collides inelastically with air mass in sink vessel.
    // Excess energy is converted to molecular kinetic energy.
    const double sinkFractionVelocity =
        0.5 * clamp((fractionVolume / sinkCrossSection) / params.dt, 0.0, c_sink);
    const double sinkFractionVelocity_squared = sinkFractionVelocity * sinkFractionVelocity;
    const double sinkFractionVelocity_x = sinkFractionVelocity * dx;
    const double sinkFractionVelocity_y = sinkFractionVelocity * dy;
    const double sinkFractionMomentum_x = sinkFractionVelocity_x * fractionMass;
    const double sinkFractionMomentum_y = sinkFractionVelocity_y * fractionMass;

    /*
    const double sinkMomentum_x = sink->m_state.momentum[0];
    const double sinkMomentum_y = sink->m_state.momentum[1];
    const double sinkVelocity_x = sinkMomentum_x / sinkMass;
    const double sinkVelocity_y = sinkMomentum_y / sinkMass;
    const double sinkVelocity_squared =
        sinkVelocity_x * sinkVelocity_x + sinkVelocity_y * sinkVelocity_y;

    const double finalSinkVelocity_x =
        (sinkFractionMomentum_x + sinkMomentum_x) / (fractionMass + sinkMass);
    const double finalSinkVelocity_y =
        (sinkFractionMomentum_y + sinkMomentum_y) / (fractionMass + sinkMass);
    const double finalSinkVelocity_squared =
        finalSinkVelocity_x * finalSinkVelocity_x + finalSinkVelocity_y * finalSinkVelocity_y;
        */
    const double sinkMomentum_x = sink->m_state.momentum[0];
    const double sinkMomentum_y = sink->m_state.momentum[1];
    const double sinkVelocity_x = sinkMomentum_x / sinkMass;
    const double sinkVelocity_y = sinkMomentum_y / sinkMass;
    const double sinkVelocity_squared =
        sinkVelocity_x * sinkVelocity_x + sinkVelocity_y * sinkVelocity_y;

    const double finalSinkVelocity_x = acc_s * sinkFractionVelocity_x + (1 - acc_s) * sinkVelocity_x;
    const double finalSinkVelocity_y = acc_s * sinkFractionVelocity_y + (1 - acc_s) * sinkVelocity_y;
    const double finalSinkVelocity_squared =
        finalSinkVelocity_x * finalSinkVelocity_x + finalSinkVelocity_y * finalSinkVelocity_y;

    const double E_k_initialSink = 0.5 * sinkMass * sinkVelocity_squared;
    const double E_k_finalSink = 0.5 * (fractionMass + sinkMass) * finalSinkVelocity_squared;

    sink->m_state.momentum[0] = finalSinkVelocity_x * (fractionMass + sinkMass);
    sink->m_state.momentum[1] = finalSinkVelocity_y * (fractionMass + sinkMass);
    sink->m_state.E_k -= (E_k_finalSink - E_k_initialSink);

    return flow * direction;
}

double GasSystem::flow(double k_flow, double dt, double P_env, double T_env, const Mix &mix) {
    const double maxFlow = pressureEquilibriumMaxFlow(P_env, T_env);
    double flow = dt * flowRate(
        k_flow,
        pressure(),
        P_env,
        temperature(),
        T_env,
        heatCapacityRatio(),
        m_chokedFlowLimit,
        m_chokedFlowFactorCached);

    if (std::abs(flow) > std::abs(maxFlow)) {
        flow = maxFlow;
    }

    this->flow(flow, kineticEnergyPerMol(T_env, m_degreesOfFreedom), mix);

    return flow;
}

double GasSystem::pressureEquilibriumMaxFlow(const GasSystem *b) const {
    // pressure_a = (kineticEnergy() + n * b->kineticEnergyPerMol()) / (0.5 * degreesOfFreedom * volume())
    // pressure_b = (b->kineticEnergy() - n *  / (0.5 * b->degreesOfFreedom * b->volume())
    // pressure_a = pressure_b

    // E_a = kineticEnergy()
    // E_b = b->kineticEnergy()
    // D_a = E_a / n()
    // D_b = E_b / b->n()
    // Q_a = 1 / (0.5 * degreesOfFreedom * volume())
    // Q_b = 1 / (0.5 * b->degreesOfFreedom * b->volume())
    // pressure_a = Q_a * (E_a + dn * D_b)
    // pressure_b = Q_b * (E_b - dn * D_b)

    if (pressure() > b->pressure()) {
        const double maxFlow =
                (b->volume() * kineticEnergy() - volume() * b->kineticEnergy()) /
                (b->volume() * kineticEnergyPerMol() + volume() * kineticEnergyPerMol());
        return std::fmax(0.0, std::fmin(maxFlow, n()));
    }
    else {
        const double maxFlow =
                (b->volume() * kineticEnergy() - volume() * b->kineticEnergy()) /
                (b->volume() * b->kineticEnergyPerMol() + volume() * b->kineticEnergyPerMol());
        return std::fmin(0.0, std::fmax(maxFlow, -b->n()));
    }
}

double GasSystem::pressureEquilibriumMaxFlow(double P_env, double T_env) const {
    if (pressure() > P_env) {
        return -(P_env * (0.5 * m_degreesOfFreedom * volume()) - kineticEnergy()) / kineticEnergyPerMol();
    }
    else {
        const double E_k_per_mol_env = 0.5 * T_env * constants::R * m_degreesOfFreedom;
        return -(P_env * (0.5 * m_degreesOfFreedom * volume()) - kineticEnergy()) / E_k_per_mol_env;
    }
}
