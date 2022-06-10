#include "../include/gas_system.h"

#include "../include/units.h"

#include <cmath>
#include <cassert>

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
    state.n_mol = P * V / (constants::R * T);
    state.V = V;
    state.E_k = T * (0.5 * degreesOfFreedom * state.n_mol * constants::R);
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
    next.E_k += dT * 0.5 * degreesOfFreedom * n() * constants::R;
}

void GasSystem::changeEnergy(double dE) {
    next.E_k += dE;
}

void GasSystem::changeMix(const Mix &mix) {
    next.mix = mix;
}

void GasSystem::injectFuel(double n) {
    const double n_fuel = this->n_fuel() + n;
    const double p_fuel = n_fuel / this->n();
    next.mix.p_fuel = p_fuel;
}

void GasSystem::changeTemperature(double dT, double n) {
    next.E_k += dT * 0.5 * degreesOfFreedom * n * constants::R;
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

double GasSystem::flowConstant(
    double flowRate,
    double P,
    double pressureDrop,
    double T)
{
    // Calculate the specific gravity (density) of air
    // at standard conditions
    // PV = nRT
    // density (per m3) = (P / RT) * mol_mass
    constexpr double standard_temp = units::fahrenheit(70);
    constexpr double standard_pressure = units::pressure(1.0, units::atm);
    constexpr double reference_sg =
        (standard_pressure * units::AirMolecularMass) / (constants::R * standard_temp);
    const double sg =
        (P * units::AirMolecularMass) / (constants::R * T);
    const double norm_sg = sg / reference_sg;
    if (norm_sg == 0) return 0;

    const double P0 = P;
    const double P1 = P - pressureDrop;

    const double T_R = units::toAbsoluteFahrenheit(T);
    const double P0_imp = units::toPsia(P0);
    const double P1_imp = units::toPsia(P1);

    // C_v = flow coefficient
    double C_v;
    const double Q_g = units::convert(flowRate, units::scfm);
    if (P0 > 1.4 * P1) {
        // Critical flow
        C_v = Q_g * std::sqrt(norm_sg * T_R) / (816 * P0_imp);
    }
    else {
        C_v = (Q_g / 962) * std::sqrt((norm_sg * T_R) / (P0_imp * P0_imp - P1_imp * P1_imp));
    }

    return C_v;
}

double GasSystem::k_28inH2O(double flowRateScfm) {
    return flowConstant(
        units::flow(flowRateScfm, units::scfm),
        units::pressure(1.0, units::atm),
        units::pressure(28.0, units::inH2O),
        units::celcius(25)
    );
}

double GasSystem::k_carb(double flowRateScfm) {
    return flowConstant(
        units::flow(flowRateScfm, units::scfm),
        units::pressure(1.0, units::atm),
        units::pressure(1.5, units::inHg),
        units::celcius(25)
    );
}

double GasSystem::flowRate(
    double k_flow,
    double P0_,
    double P1_,
    double T0,
    double T1,
    double sg0,
    double sg1)
{
    // Calculate the specific gravity (density) of air
    // at standard conditions
    // PV = nRT
    // density (per m3) = (P / RT) * mol_mass
    constexpr double standard_temp = units::fahrenheit(70);
    constexpr double standard_pressure = units::pressure(1.0, units::atm);
    constexpr double reference_sg =
        (standard_pressure * units::AirMolecularMass) / (constants::R * standard_temp);

    double direction;
    double T;
    double P0, P1;
    double sg;
    if (P0_ > P1_) {
        direction = 1.0;
        T = T0;
        P0 = P0_;
        P1 = P1_;
        sg = sg0;
    }
    else {
        direction = -1.0;
        T = T1;
        P0 = P1_;
        P1 = P0_;
        sg = sg1;
    }

    const double norm_sg = sg / reference_sg;
    if (norm_sg == 0) return 0;

    const double T_R = units::toAbsoluteFahrenheit(T);
    const double P0_imp = units::toPsia(P0);
    const double P1_imp = units::toPsia(P1);

    // Q_g = gas flow [SCFM]
    double Q_g;
    if (P0 > 1.9 * P1) {
        // Critical flow
        Q_g = (k_flow * 816 * P0_imp) / std::sqrt(T_R * norm_sg);
    }
    else {
        Q_g = k_flow * 962 * std::sqrt((P0_imp * P0_imp - P1_imp * P1_imp) / (T_R * norm_sg));
    }

    return units::flow(Q_g, units::scfm) * direction;
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
    next.E_k -= kineticEnergy(dn);
    next.n_mol -= dn;

    assert(next.n_mol >= 0);

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
    const double maxFlow = pressureEquilibriumMaxFlow(target);
    double flow = dt * flowRate(
        k_flow,
        pressure(),
        target->pressure(),
        temperature(),
        target->temperature(),
        approximateDensity(),
        target->approximateDensity());

    if (std::abs(flow) > std::abs(maxFlow)) {
        flow = maxFlow;
    }

    this->flow(flow, target->kineticEnergyPerMol(), target->mix());
    target->flow(-flow, kineticEnergyPerMol(), mix());

    return flow;
}

double GasSystem::flow(double k_flow, double dt, double P_env, double T_env, const Mix &mix) {
    constexpr double standardPressure = units::pressure(1.0, units::atm);
    constexpr double standardTemp = units::celcius(25.0);
    constexpr double airDensity =
        units::AirMolecularMass * (standardPressure * units::volume(1.0, units::m3))
        / (constants::R * standardTemp);

    const double maxFlow = pressureEquilibriumMaxFlow(P_env, T_env);
    double flow = dt * flowRate(
        k_flow,
        pressure(),
        P_env,
        temperature(),
        T_env,
        approximateDensity(),
        airDensity);

    if (std::abs(flow) > std::abs(maxFlow)) {
        flow = maxFlow;
    }

    this->flow(flow, kineticEnergyPerMol(T_env, degreesOfFreedom), mix);

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
        const double E_k_per_mol_env = 0.5 * T_env * constants::R * degreesOfFreedom;
        return -(P_env * (0.5 * degreesOfFreedom * volume()) - kineticEnergy()) / E_k_per_mol_env;
    }
}

/*
double GasSystem::approximateDensity() const {
    return (units::AirMolecularMass * n()) / volume();
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
    return kineticEnergy() / (0.5 * degreesOfFreedom * n() * constants::R);
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
*/