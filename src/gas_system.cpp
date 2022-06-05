#include "../include/gas_system.h"

#include "../include/constants.h"

#include <cmath>

void GasSystem::start() {
    next = state;
}

void GasSystem::end() {
    state = next;

    state.E_k = std::fmax(state.E_k, 0.0);
    state.n_mol = std::fmax(state.n_mol, 0.0);
    state.V = std::fmax(state.V, 0.0);
}

void GasSystem::initialize(double P, double V, double T, const Mix &mix) {
    state.n_mol = P * V / (Constants::R * T);
    state.V = V;
    state.E_k = T * (0.5 * degreesOfFreedom * state.n_mol * Constants::R);
    state.mix = mix;
}

void GasSystem::setVolume(double V) {
    return changeVolume(V - next.V);
}

void GasSystem::setN(double n) {
    next.n_mol = n;
    next.E_k = kineticEnergy(n);
}

void GasSystem::changeVolume(double dV) {
    const double V = this->volume();
    const double L = std::pow(V + dV, 1 / 3.0);
    const double surfaceArea = (L * L);
    const double dL = -dV / surfaceArea;
    const double W = dL * pressure() * surfaceArea;

    next.V += dV;
    next.E_k += W;
}

void GasSystem::changePressure(double dP) {
    next.E_k += dP * volume() * degreesOfFreedom * 0.5;
}

void GasSystem::changeTemperature(double dT) {
    next.E_k += dT * 0.5 * degreesOfFreedom * n() * Constants::R;
}

void GasSystem::changeEnergy(double dE) {
    next.E_k += dE;
}

void GasSystem::changeTemperature(double dT, double n) {
    next.E_k += dT * 0.5 * degreesOfFreedom * n * Constants::R;
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

    next.n_mol += dn;

    // Adjust mix
    const double new_system_n_fuel = system_n_fuel - a_n_fuel;
    const double new_system_n_o2 = system_n_o2 - a_n_o2;
    const double new_system_n_inert = system_n_inert + products_n;
    const double new_system_n = system_n + dn;

    if (new_system_n != 0) {
        next.mix.p_fuel = new_system_n_fuel / new_system_n;
        next.mix.p_inert = new_system_n_inert / new_system_n;
        next.mix.p_o2 = new_system_n_o2 / new_system_n;
    }
    else {
        next.mix.p_fuel = next.mix.p_inert = next.mix.p_o2 = 0;
    }

    return a_n_fuel;
}

double GasSystem::flow(double dn, GasSystem *target) {
    double flow = 0;
    if (dn >= 0) {
        flow = loseN(dn);
        target->gainN(dn, kineticEnergyPerMol(), state.mix);
    }
    else {
        flow = gainN(-dn, target->kineticEnergyPerMol(), target->state.mix);
        target->loseN(-dn);
    }

    return flow;
}

double GasSystem::flow(double dn, double T_env, const Mix &mix) {
    if (dn >= 0) {
        return loseN(dn);
    }
    else {
        const double E_k_per_mol = 0.5 * T_env * Constants::R * degreesOfFreedom;
        return gainN(-dn, E_k_per_mol, mix);
    }
}

double GasSystem::loseN(double dn) {
    next.E_k -= kineticEnergy(dn);
    next.n_mol -= dn;

    return dn;
}

double GasSystem::gainN(double dn, double E_k_per_mol, const Mix &mix) {
    const double next_n = next.n_mol + dn;
    const double current_n = next.n_mol;

    next.E_k += dn * E_k_per_mol;
    next.n_mol = next_n;

    if (next_n != 0) {
        next.mix.p_fuel = (next.mix.p_fuel * current_n + dn * mix.p_fuel) / next_n;
        next.mix.p_inert = (next.mix.p_inert * current_n + dn * mix.p_inert) / next_n;
        next.mix.p_o2 = (next.mix.p_o2 * current_n + dn * mix.p_o2) / next_n;
    }
    else {
        next.mix.p_fuel = next.mix.p_inert = next.mix.p_o2 = 0;
    }

    return -dn;
}

double GasSystem::flow(double k_flow, double dt, GasSystem *target) {
    const double flowRate =
        std::fmin(
            std::sqrt(std::abs(pressure() - target->pressure())) * k_flow * dt,
            std::abs(pressureEquilibriumMaxFlow(target)));
    const double flowDirection = (pressure() < target->pressure())
        ? -1.0
        : 1.0;

    return flow(flowDirection * flowRate, target);
}

double GasSystem::flow(double k_flow, double dt, double P_env, double T_env, const Mix &mix) {
    const double flowRate =
        std::fmin(
            std::sqrt(std::abs(pressure() - P_env)) * k_flow * dt,
            std::abs(pressureEquilibriumMaxFlow(P_env, T_env)));
    const double flowDirection = (pressure() < P_env)
        ? -1.0
        : 1.0;

    return flow(flowDirection * flowRate, T_env, mix);
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
        return  (b->volume() * kineticEnergy() - volume() * b->kineticEnergy()) /
                (b->volume() * kineticEnergyPerMol() + volume() * kineticEnergyPerMol());
    }
    else {
        return  (b->volume() * kineticEnergy() - volume() * b->kineticEnergy()) /
                (b->volume() * b->kineticEnergyPerMol() + volume() * b->kineticEnergyPerMol());
    }
}

double GasSystem::pressureEquilibriumMaxFlow(double P_env, double T_env) const {
    if (pressure() > P_env) {
        return -(P_env * (0.5 * degreesOfFreedom * volume()) - kineticEnergy()) / kineticEnergyPerMol();
    }
    else {
        const double E_k_per_mol_env = 0.5 * T_env * Constants::R * degreesOfFreedom;
        return -(P_env * (0.5 * degreesOfFreedom * volume()) - kineticEnergy()) / E_k_per_mol_env;
    }
}

double GasSystem::n() const {
    return state.n_mol;
}

double GasSystem::n(double V) const {
    return (V / volume()) * n();
}

double GasSystem::kineticEnergy() const {
    return state.E_k;
}

double GasSystem::kineticEnergy(double n) const {
    return (kineticEnergy() / this->n()) * n;
}

double GasSystem::pressure() const {
    const double volume = this->volume();
    return (volume != 0)
        ? kineticEnergy() / (0.5 * degreesOfFreedom * volume)
        : 0;
}

double GasSystem::temperature() const {
    return kineticEnergy() / (0.5 * degreesOfFreedom * n() * Constants::R);
}

double GasSystem::volume() const {
    return state.V;
}

double GasSystem::n_inert() const {
    return state.mix.p_inert * n();
}

double GasSystem::n_fuel() const {
    return state.mix.p_fuel * n();
}

double GasSystem::n_o2() const {
    return state.mix.p_o2 * n();
}
