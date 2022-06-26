#ifndef ATG_ENGINE_SIM_GAS_SYSTEM_H
#define ATG_ENGINE_SIM_GAS_SYSTEM_H

#include "constants.h"
#include "units.h"

#include <cfloat>
#include <cmath>

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
            double momentum[2] = { 0.0, 0.0 };

            Mix mix;
        };

        struct FlowParameters {
            double k_flow;
            double dt;
            double direction_x, direction_y;
            double crossSectionArea_0, crossSectionArea_1;
            GasSystem *system_0, *system_1;
        };

        class FlowState {
        public:
            struct FlowFraction {
                double mass;
                double momentum[2];
                double E_k_mol;
            };

        public:
            FlowFraction fractions[3];

            inline void changeVelocity(int f, double v_x, double v_y);
            inline void inelasticCollision(int f0, int f1);
        };

    public:
        GasSystem() { /* void */ }
        ~GasSystem() { /* void */ }

        void initialize(double P, double V, double T, const Mix &mix = {}, int degreesOfFreedom = 5);

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
        static double flowConstant(double flowRate, double P, double pressureDrop, double T, double hcr);
        static double k_28inH2O(double flowRateScfm);
        static double k_carb(double flowRateScfm);
        static double flowRate(
            double k_flow,
            double P0,
            double P1,
            double T0,
            double T1,
            double hcr,
            double chokedFlowLimit,
            double chokedFlowRateCached);
        double flow(double dn, double E_k_per_mol, const Mix &mix = {});
        double loseN(double dn);
        double gainN(double dn, double E_k_per_mol, const Mix &mix = {});
        void dissipateVelocity(double dt, double timeConstant);

        static double flow(const FlowParameters &params);
        double flow(double k_flow, double dt, double P_env, double T_env, const Mix &mix = {});

        double pressureEquilibriumMaxFlow(const GasSystem *b) const;
        double pressureEquilibriumMaxFlow(double P_env, double T_env) const;

        inline static constexpr double kineticEnergyPerMol(double T, int degreesOfFreedom);
        inline static constexpr double heatCapacityRatio(int degreesOfFreedom);
        inline static double chokedFlowLimit(int degreesOfFreedom);
        inline static double chokedFlowRate(int degreesOfFreedom);

        inline double approximateDensity() const;
        inline int degreesOfFreedom() const { return m_degreesOfFreedom; }
        inline double n() const;
        inline double n(double V) const;
        inline double kineticEnergy() const;
        inline double kineticEnergy(double n) const;
        inline double kineticEnergyPerMol() const { return kineticEnergy(1.0); }
        inline double totalEnergy() const;
        inline double dynamicPressure(double dx, double dy) const;
        inline double mass() const;
        inline double pressure() const;
        inline double temperature() const;
        inline double velocity_x() const;
        inline double velocity_y() const;
        inline double volume() const;
        inline double volume(double n) const;
        inline double n_fuel() const;
        inline double n_inert() const;
        inline double n_o2() const;
        inline double heatCapacityRatio() const;
        inline Mix mix() const { return m_state.mix; }

    protected:
        State m_state;
        State m_next;

        int m_degreesOfFreedom = 5;

        double m_chokedFlowLimit;
        double m_chokedFlowFactorCached;
};

inline constexpr double GasSystem::kineticEnergyPerMol(double T, int degreesOfFreedom) {
    return 0.5 * T * constants::R * degreesOfFreedom;
}

inline constexpr double GasSystem::heatCapacityRatio(int degreesOfFreedom) {
    return 1.0 + (2.0 / degreesOfFreedom);
}

inline double GasSystem::chokedFlowLimit(int degreesOfFreedom) {
    const double hcr = heatCapacityRatio(degreesOfFreedom);
    return std::pow((2.0 / (hcr + 1)), hcr / (hcr - 1));
}

inline double GasSystem::chokedFlowRate(int degreesOfFreedom) {
    const double hcr = heatCapacityRatio(degreesOfFreedom);
    double flowRate =
        std::sqrt(hcr) * std::pow(2 / (hcr + 1), (hcr + 1) / (2 * (hcr - 1)));

    return flowRate;
}

inline double GasSystem::approximateDensity() const {
    return (units::AirMolecularMass * n()) / volume();
}

inline double GasSystem::n() const {
    return m_state.n_mol;
}

inline double GasSystem::n(double V) const {
    return (V / volume()) * n();
}

inline double GasSystem::kineticEnergy() const {
    return m_state.E_k;
}

inline double GasSystem::kineticEnergy(double n) const {
    return (kineticEnergy() / this->n()) * n;
}

inline double GasSystem::totalEnergy() const {
    if (n() == 0) return 0;

    const double invMass = 1 / mass();
    const double v_x = m_state.momentum[0] * invMass;
    const double v_y = m_state.momentum[1] * invMass;
    const double v_squared = v_x * v_x + v_y * v_y;

    return kineticEnergy() + 0.5 * mass() * v_squared;
}

inline double GasSystem::dynamicPressure(double dx, double dy) const {
    if (n() == 0 || kineticEnergy() == 0) return 0;

    const double inverseMass = 1 / this->mass();
    const double v = inverseMass * (dx * m_state.momentum[0] + dy * m_state.momentum[1]);

    if (v <= 0) {
        return 0;
    }

    const double hcr = heatCapacityRatio();
    const double staticPressure = pressure();
    const double density = approximateDensity();
    const double c = std::sqrt(staticPressure * hcr / density);
    const double machNumber = v / c;

    return
        staticPressure
        * (std::pow(1 + ((hcr - 1) / 2) * machNumber * machNumber, hcr / (hcr - 1)) - 1);
}

inline double GasSystem::mass() const {
    return units::AirMolecularMass * n();
}

inline double GasSystem::pressure() const {
    const double volume = this->volume();
    return (volume != 0)
        ? kineticEnergy() / (0.5 * m_degreesOfFreedom * volume)
        : 0;
}

inline double GasSystem::temperature() const {
    if (n() == 0) return 0;
    else return kineticEnergy() / (0.5 * m_degreesOfFreedom * n() * constants::R);
}

inline double GasSystem::velocity_x() const {
    if (n() == 0) return 0;
    else return m_state.momentum[0] / mass();
}

inline double GasSystem::velocity_y() const {
    if (n() == 0) return 0;
    else return m_state.momentum[1] / mass();
}


inline double GasSystem::volume() const {
    return m_state.V;
}

inline double GasSystem::volume(double n) const {
    return n * this->n() / volume();
}

inline double GasSystem::n_fuel() const {
    return m_state.mix.p_fuel * n();
}

inline double GasSystem::n_inert() const {
    return m_state.mix.p_inert * n();
}

inline double GasSystem::n_o2() const {
    return m_state.mix.p_o2 * n();
}

inline double GasSystem::heatCapacityRatio() const {
    return heatCapacityRatio(m_degreesOfFreedom);
}

inline void GasSystem::FlowState::changeVelocity(int f, double v_x, double v_y) {
    FlowFraction &frac = fractions[f];

    const double vx_0 = frac.momentum[0] / frac.mass;
    const double vy_0 = frac.momentum[1] / frac.mass;

    const double E_k_current = 0.5 * frac.mass * (vx_0 * vx_0 + vy_0 * vy_0);
    const double E_k_final = 0.5 * frac.mass * (v_x * v_x + v_y * v_y);

    frac.E_k_mol -= (E_k_final - E_k_current);
}

inline void GasSystem::FlowState::inelasticCollision(int f0, int f1) {
    FlowFraction &frac0 = fractions[f0];
    FlowFraction &frac1 = fractions[f1];

    const double vx_0 = frac0.momentum[0] / frac0.mass;
    const double vy_0 = frac0.momentum[1] / frac0.mass;

    const double vx_1 = frac1.momentum[0] / frac1.mass;
    const double vy_1 = frac1.momentum[1] / frac1.mass;

    const double vf_x = (frac0.momentum[0] + frac1.momentum[0]) / (frac0.mass + frac1.mass);
    const double vf_y = (frac0.momentum[1] + frac1.momentum[1]) / (frac0.mass + frac1.mass);

    const double E_k_current_0 = 0.5 * frac0.mass * (vx_0 * vx_0 + vy_0 * vy_0);
    const double E_k_current_1 = 0.5 * frac1.mass * (vx_1 * vx_1 + vy_1 * vy_1);
    const double E_k_final = 0.5 * (frac0.mass + frac1.mass) * (vf_x * vf_x + vf_y * vf_y);
    const double E_k_residual = E_k_final - (E_k_current_0 + E_k_current_1);

    const double s = frac0.mass / (frac0.mass + frac1.mass);
    frac0.E_k_mol -= E_k_residual * s;
    frac1.E_k_mol -= E_k_residual * (1 - s);

    frac0.momentum[0] = vf_x * frac0.mass;
    frac0.momentum[1] = vf_y * frac0.mass;
    frac1.momentum[0] = vf_x * frac1.mass;
    frac1.momentum[1] = vf_y * frac1.mass;
}

#endif /* ATG_ENGINE_SIM_GAS_SYSTEM_H */
