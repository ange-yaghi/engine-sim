#include "../include/gas_system.h"

#include "../include/units.h"
#include "../include/utilities.h"

#include <cmath>
#include <cassert>

void GasSystem::setGeometry(double width, double height, double dx, double dy) {
    m_width = width;
    m_height = height;
    m_dx = dx;
    m_dy = dy;
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
        flowRate /= std::sqrt(constants::R * T_0);
    }
    else {
        const double s = std::pow(p_ratio, 1 / hcr);

        flowRate = (2 * hcr) / (hcr - 1);
        flowRate *= s * (s - p_ratio);
        flowRate = std::sqrt(std::fmax(flowRate, 0.0) / (constants::R * T_0));
    }

    flowRate *= direction * p_0;

    return flowRate * k_flow;
}

double GasSystem::loseN(double dn, double E_k_per_mol) {
    m_state.E_k -= E_k_per_mol * dn;
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

void GasSystem::dissipateExcessVelocity() {
    const double v_x = velocity_x();
    const double v_y = velocity_y();
    const double v_squared = v_x * v_x + v_y * v_y;
    const double c = this->c();
    const double c_squared = c * c;

    if (c_squared >= v_squared || v_squared == 0) {
        return;
    }

    const double k_squared = c_squared / v_squared;
    const double k = std::sqrt(k_squared);

    m_state.momentum[0] *= k;
    m_state.momentum[1] *= k;

    m_state.E_k += 0.5 * mass() * (v_squared - c_squared);

    if (m_state.E_k < 0) m_state.E_k = 0;

    if (std::isnan(m_state.momentum[0]) || std::isnan(m_state.E_k)) {
        int a = 0;
    }
}

void GasSystem::updateVelocity(double dt, double beta) {
    if (n() == 0) return;

    const double depth = volume() / (m_width * m_height);
    
    double d_momentum_x = 0;
    double d_momentum_y = 0;

    const double p0 = dynamicPressure(m_dx, m_dy);
    const double p1 = dynamicPressure(-m_dx, -m_dy);
    const double p2 = dynamicPressure(m_dy, m_dx);
    const double p3 = dynamicPressure(-m_dy, -m_dx);

    const double p_sa_0 = p0 * (m_height * depth);
    const double p_sa_1 = p1 * (m_height * depth);
    const double p_sa_2 = p2 * (m_width * depth);
    const double p_sa_3 = p3 * (m_width * depth);

    d_momentum_x += p_sa_0 * m_dx;
    d_momentum_y += p_sa_0 * m_dy;

    d_momentum_x -= p_sa_1 * m_dx;
    d_momentum_y -= p_sa_1 * m_dy;

    d_momentum_x += p_sa_2 * m_dy;
    d_momentum_y += p_sa_2 * m_dx;

    d_momentum_x -= p_sa_3 * m_dy;
    d_momentum_y -= p_sa_3 * m_dx;

    const double m = mass();
    const double inv_m = 1 / m;
    const double v0_x = m_state.momentum[0] * inv_m;
    const double v0_y = m_state.momentum[1] * inv_m;

    m_state.momentum[0] -= d_momentum_x * dt * beta;
    m_state.momentum[1] -= d_momentum_y * dt * beta;

    const double v1_x = m_state.momentum[0] * inv_m;
    const double v1_y = m_state.momentum[1] * inv_m;

    m_state.E_k -= 0.5 * m * (v1_x * v1_x - v0_x * v0_x);
    m_state.E_k -= 0.5 * m * (v1_y * v1_y - v0_y * v0_y);

    if (m_state.E_k < 0) m_state.E_k = 0;
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

    const double maxFlow = source->pressureEquilibriumMaxFlow(sink);
    flow = clamp(flow, 0.0, 0.9 * source->n());
    //flow = clamp(flow, 0.0, maxFlow);

    const double fraction = flow / source->n();
    const double fractionVolume = fraction * source->volume();
    const double fractionMass = fraction * source->mass();
    const double remainingMass = (1 - fraction) * source->mass();

    if (flow != 0) {
        // - Stage 1
        // Fraction flows from source to sink.

        const double E_k_bulk_src0 = source->bulkKineticEnergy();
        const double E_k_bulk_sink0 = sink->bulkKineticEnergy();

        const double s0 = source->totalEnergy() + sink->totalEnergy();

        const double E_k_per_mol = source->kineticEnergyPerMol();
        sink->gainN(flow, E_k_per_mol, source->mix());
        source->loseN(flow, E_k_per_mol);

        const double s1 = source->totalEnergy() + sink->totalEnergy();

        const double dp_x = source->m_state.momentum[0] * fraction;
        const double dp_y = source->m_state.momentum[1] * fraction;
        source->m_state.momentum[0] -= dp_x;
        source->m_state.momentum[1] -= dp_y;

        sink->m_state.momentum[0] += dp_x;
        sink->m_state.momentum[1] += dp_y;

        const double E_k_bulk_src1 = source->bulkKineticEnergy();
        const double E_k_bulk_sink1 = sink->bulkKineticEnergy();

        sink->m_state.E_k -= ((E_k_bulk_src1 + E_k_bulk_sink1) - (E_k_bulk_src0 + E_k_bulk_sink0));
    }
    
    const double sourceMass = source->mass();
    const double sinkMass = sink->mass();

    const double c_source = source->c();
    const double c_sink = sink->c();

    const double sourceInitialMomentum_x = source->m_state.momentum[0];
    const double sourceInitialMomentum_y = source->m_state.momentum[1];

    const double sinkInitialMomentum_x = sink->m_state.momentum[0];
    const double sinkInitialMomentum_y = sink->m_state.momentum[1];

    // Momentum in fraction

    if (sinkCrossSection != 0) {
        const double sinkFractionVelocity =
            clamp((fractionVolume / sinkCrossSection) / params.dt, 0.0, c_sink);
        const double sinkFractionVelocity_squared = sinkFractionVelocity * sinkFractionVelocity;
        const double sinkFractionVelocity_x = sinkFractionVelocity * dx;
        const double sinkFractionVelocity_y = sinkFractionVelocity * dy;
        const double sinkFractionMomentum_x = sinkFractionVelocity_x * fractionMass;
        const double sinkFractionMomentum_y = sinkFractionVelocity_y * fractionMass;

        sink->m_state.momentum[0] += sinkFractionMomentum_x;
        sink->m_state.momentum[1] += sinkFractionMomentum_y;
    }

    if (sourceCrossSection != 0 && sourceMass != 0) {
        const double sourceFractionVelocity =
            clamp((fractionVolume / sourceCrossSection) / params.dt, 0.0, c_source);
        const double sourceFractionVelocity_squared = sourceFractionVelocity * sourceFractionVelocity;
        const double sourceFractionVelocity_x = sourceFractionVelocity * dx;
        const double sourceFractionVelocity_y = sourceFractionVelocity * dy;
        const double sourceFractionMomentum_x = sourceFractionVelocity_x * fractionMass;
        const double sourceFractionMomentum_y = sourceFractionVelocity_y * fractionMass;

        source->m_state.momentum[0] += sourceFractionMomentum_x;
        source->m_state.momentum[1] += sourceFractionMomentum_y;
    }

    if (sourceMass != 0) {
        const double invSourceMass = 1 / sourceMass;

        // Energy conservation
        const double sourceVelocity0_x = sourceInitialMomentum_x * invSourceMass;
        const double sourceVelocity0_y = sourceInitialMomentum_y * invSourceMass;

        const double sourceVelocity1_x = source->m_state.momentum[0] * invSourceMass;
        const double sourceVelocity1_y = source->m_state.momentum[1] * invSourceMass;

        source->m_state.E_k -=
            0.5 * sourceMass
            * (sourceVelocity1_x * sourceVelocity1_x - sourceVelocity0_x * sourceVelocity0_x);

        source->m_state.E_k -=
            0.5 * sourceMass
            * (sourceVelocity1_y * sourceVelocity1_y - sourceVelocity0_y * sourceVelocity0_y);
    }

    if (sinkMass > 0) {
        const double invSinkMass = 1 / sinkMass;

        const double sinkVelocity0_x = sinkInitialMomentum_x * invSinkMass;
        const double sinkVelocity0_y = sinkInitialMomentum_y * invSinkMass;

        const double sinkVelocity1_x = sink->m_state.momentum[0] * invSinkMass;
        const double sinkVelocity1_y = sink->m_state.momentum[1] * invSinkMass;

        sink->m_state.E_k -=
            0.5 * sinkMass
            * (sinkVelocity1_x * sinkVelocity1_x - sinkVelocity0_x * sinkVelocity0_x);

        sink->m_state.E_k -=
            0.5 * sinkMass
            * (sinkVelocity1_y * sinkVelocity1_y - sinkVelocity0_y * sinkVelocity0_y);
    }

    if (sink->m_state.E_k < 0) {
        sink->m_state.E_k = 0;
    }

    if (source->m_state.E_k < 0) {
        source->m_state.E_k = 0;
    }

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

    if (flow < 0) {
        const double bulk_E_k_0 = bulkKineticEnergy();
        gainN(-flow, kineticEnergyPerMol(T_env, m_degreesOfFreedom), mix);
        const double bulk_E_k_1 = bulkKineticEnergy();

        m_state.E_k += (bulk_E_k_1 - bulk_E_k_0);
    }
    else {
        const double starting_n = n();
        loseN(flow, kineticEnergyPerMol());

        m_state.momentum[0] -= (flow / starting_n) * m_state.momentum[0];
        m_state.momentum[1] -= (flow / starting_n) * m_state.momentum[1];
    }

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
