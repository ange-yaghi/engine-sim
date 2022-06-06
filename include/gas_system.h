#ifndef ATG_ENGINE_SIM_GAS_SYSTEM_H
#define ATG_ENGINE_SIM_GAS_SYSTEM_H

#include "constants.h"

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

        double flow(double k_flow, double dt, GasSystem *target);
        double flow(double k_flow, double dt, double P_env, double T_env, const Mix &mix = {});

        double pressureEquilibriumMaxFlow(const GasSystem *b) const;
        double pressureEquilibriumMaxFlow(double P_env, double T_env) const;

        inline static constexpr double kineticEnergyPerMol(double T, int degreesOfFreedom);

        double approximateDensity() const;
        double n() const;
        double n(double V) const;
        double kineticEnergy() const;
        double kineticEnergy(double n) const;
        double kineticEnergyPerMol() const { return kineticEnergy(1.0); }
        double pressure() const;
        double temperature() const;
        double volume() const;
        double n_fuel() const;
        double n_inert() const;
        double n_o2() const;
        inline Mix mix() const { return state.mix; }

        int degreesOfFreedom = 5;
        double specificGravity = 1.0;

    protected:
        State state, next;
};

inline constexpr double GasSystem::kineticEnergyPerMol(double T, int degreesOfFreedom) {
    return 0.5 * T * constants::R * degreesOfFreedom;
}

#endif /* ATG_ENGINE_SIM_GAS_SYSTEM_H */
