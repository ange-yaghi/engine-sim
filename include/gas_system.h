#ifndef ATG_ENGINE_SIM_GAS_SYSTEM_H
#define ATG_ENGINE_SIM_GAS_SYSTEM_H

class GasSystem {
    public:
        struct State {
            double n_mol = 0.0;
            double E_k = 0.0;
            double V = 0.0;
            double afr = 0.0;
        };

    public:
        void initialize(double P, double V, double T);

        void start();
        void end();

        void setVolume(double V);
        void setN(double n);

        void changeVolume(double dV);
        void changePressure(double pressure);
        void changeTemperature(double dT);
        void changeTemperature(double dT, double n);

        double flow(double dn, GasSystem *target);
        double flow(double dn, double T_env);
        double loseN(double dn);
        double gainN(double dn, double E_k_per_mol);

        double flow(double k_flow, double dt, GasSystem *target);
        double flow(double k_flow, double dt, double P_env, double T_env);

        double pressureEquilibriumMaxFlow(const GasSystem *b) const;
        double pressureEquilibriumMaxFlow(double P_env, double T_env) const;

        double n() const;
        double n(double V) const;
        double kineticEnergy() const;
        double kineticEnergy(double n) const;
        double kineticEnergyPerMol() const { return kineticEnergy(1.0); }
        double pressure() const;
        double temperature() const;
        double volume() const;

        int degreesOfFreedom = 5;
        double specificGravity = 1.0;

    protected:
        State state, next;
};

#endif /* ATG_ENGINE_SIM_GAS_SYSTEM_H */
