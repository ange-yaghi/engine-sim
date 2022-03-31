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

void GasSystem::initialize(double P, double V, double T) {
    state.n_mol = P * V / (Constants::R * T);
    state.V = V;
    state.E_k = T * (0.5 * degreesOfFreedom * state.n_mol * Constants::R);
    state.afr = 0.0;
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

void GasSystem::changeTemperature(double dT, double n) {
    next.E_k += dT * 0.5 * degreesOfFreedom * n * Constants::R;
}

double GasSystem::flow(double dn, GasSystem *target) {
    double flow = 0;
    if (dn >= 0) {
        flow = loseN(dn);
        target->gainN(dn, kineticEnergyPerMol());
    }
    else {
        flow = gainN(-dn, target->kineticEnergyPerMol());
        target->loseN(-dn);
    }

    return flow;
}

double GasSystem::flow(double dn, double T_env) {
    if (dn >= 0) {
        return loseN(dn);
    }
    else {
        const double E_k_per_mol = 0.5 * T_env * Constants::R * degreesOfFreedom;
        return gainN(-dn, E_k_per_mol);
    }
}

double GasSystem::loseN(double dn) {
    next.E_k -= kineticEnergy(dn);
    next.n_mol -= dn;

    return -dn;
}

double GasSystem::gainN(double dn, double E_k_per_mol) {
    next.E_k += dn * E_k_per_mol;
    next.n_mol += dn;

    return dn;
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

double GasSystem::flow(double k_flow, double dt, double P_env, double T_env) {
    const double flowRate =
        std::fmin(
            std::sqrt(std::abs(pressure() - P_env)) * k_flow * dt,
            std::abs(pressureEquilibriumMaxFlow(P_env, T_env)));
    const double flowDirection = (pressure() < P_env)
        ? -1.0
        : 1.0;

    return flow(flowDirection * flowRate, T_env);
}

double GasSystem::pressureEquilibriumMaxFlow(const GasSystem *b) const {
    return
        (b->volume() * kineticEnergy() - volume() * b->kineticEnergy()) /
        (b->volume() * b->kineticEnergyPerMol() + volume() * kineticEnergyPerMol());
}

double GasSystem::pressureEquilibriumMaxFlow(double P_env, double T_env) const {
    const double E_k_per_mol_env = 0.5 * T_env * Constants::R * degreesOfFreedom;
    return
        (kineticEnergy() - 0.5 * degreesOfFreedom * P_env * volume()) /
        E_k_per_mol_env;
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
