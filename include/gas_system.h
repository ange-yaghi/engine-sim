#ifndef ATG_ENGINE_SIM_GAS_SYSTEM_H
#define ATG_ENGINE_SIM_GAS_SYSTEM_H

#include "constants.h"
#include "units.h"

#include <cfloat>

class GasSystem {
    public:
        struct Mix {
            double p_fuel = 0.0;
            double p_inert = 1.0;
            double p_o2 = 0.0;
        };

        struct State {
            double n_mol = 0.0;
            double E_k = 0.0;
            double V = 0.0;

            Mix mix;
        };

    public:
        void initialize(double P, double V, double T, const Mix &mix = {});

        void start();
        void end();

        void setVolume(double V);
        void setN(double n);

        void changeVolume(double dV);
        void changePressure(double pressure);
        void changeTemperature(double dT);
        void changeTemperature(double dT, double n);
        void changeEnergy(double dE);
        void changeMix(const Mix &mix);
        void injectFuel(double n);

        double react(double n, const Mix &mix);
        static double flowConstant(double flowRate, double P, double pressureDrop, double T);
        static double k_28inH2O(double flowRateScfm);
        static double k_carb(double flowRateScfm);
        static double flowRate(
            double k_flow,
            double P0,
            double P1,
            double T0,
            double T1,
            double sg0,
            double sg1);
        double flow(double dn, double E_k_per_mol, const Mix &mix = {});
        double loseN(double dn);
        double gainN(double dn, double E_k_per_mol, const Mix &mix = {});

        double flow(double k_flow, double dt, GasSystem *target, double minFlow=-DBL_MAX, double maxFlow=DBL_MAX);
        double flow(double k_flow, double dt, double P_env, double T_env, const Mix &mix = {});

        double pressureEquilibriumMaxFlow(const GasSystem *b) const;
        double pressureEquilibriumMaxFlow(double P_env, double T_env) const;

        inline static constexpr double kineticEnergyPerMol(double T, int degreesOfFreedom);

        inline double approximateDensity() const;
        inline double n() const;
        inline double n(double V) const;
        inline double kineticEnergy() const;
        inline double kineticEnergy(double n) const;
        inline double kineticEnergyPerMol() const { return kineticEnergy(1.0); }
        inline double pressure() const;
        inline double temperature() const;
        inline double volume() const;
        inline double n_fuel() const;
        inline double n_inert() const;
        inline double n_o2() const;
        inline Mix mix() const { return state.mix; }

        int degreesOfFreedom = 5;
        double specificGravity = 1.0;

    protected:
        State state, next;
};

inline constexpr double GasSystem::kineticEnergyPerMol(double T, int degreesOfFreedom) {
    return 0.5 * T * constants::R * degreesOfFreedom;
}

inline double GasSystem::approximateDensity() const {
    return (units::AirMolecularMass * n()) / volume();
}

inline double GasSystem::n() const {
    return state.n_mol;
}

inline double GasSystem::n(double V) const {
    return (V / volume()) * n();
}

inline double GasSystem::kineticEnergy() const {
    return state.E_k;
}

inline double GasSystem::kineticEnergy(double n) const {
    return (kineticEnergy() / this->n()) * n;
}

inline double GasSystem::pressure() const {
    const double volume = this->volume();
    return (volume != 0)
        ? kineticEnergy() / (0.5 * degreesOfFreedom * volume)
        : 0;
}

inline double GasSystem::temperature() const {
    return kineticEnergy() / (0.5 * degreesOfFreedom * n() * constants::R);
}

inline double GasSystem::volume() const {
    return state.V;
}

inline double GasSystem::n_fuel() const {
    return state.mix.p_fuel * n();
}

inline double GasSystem::n_inert() const {
    return state.mix.p_inert * n();
}

inline double GasSystem::n_o2() const {
    return state.mix.p_o2 * n();
}

#endif /* ATG_ENGINE_SIM_GAS_SYSTEM_H */
