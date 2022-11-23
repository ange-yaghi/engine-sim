#ifndef ATG_ENGINE_FUEL_H
#define ATG_ENGINE_FUEL_H

#include "units.h"

#include "function.h"

#include <string>

class Fuel {
    public:
        struct Parameters {
            std::string name = "Gasoline";
            double molecularMass =
                units::mass(100.0, units::g);
            double energyDensity =
                units::energy(48.1, units::kJ) / units::mass(1.0, units::g);
            double density =
                units::mass(0.755, units::kg) / units::volume(1.0, units::L);
            double molecularAfr = 25 / 2.0;
            double burningEfficiencyRandomness = 0.5;
            double lowEfficiencyAttenuation = 0.6;
            double maxBurningEfficiency = 0.8;
            double maxTurbulenceEffect = 2.0;
            double maxDilutionEffect = 50.0;
            Function *turbulenceToFlameSpeedRatio = nullptr;
        };

        Fuel();
        ~Fuel();

        void initialize(const Parameters &params);

        inline double getMolecularMass() const { return m_molecularMass; }
        inline double getEnergyDensity() const { return m_energyDensity; }
        inline double getDensity() const { return m_density; }
        inline double getBurningEfficiencyRandomness() const { return m_burningEfficiencyRandomness; }
        inline double getLowEfficiencyAttenuation() const { return m_lowEfficiencyAttenuation;  }
        inline double getMaxBurningEfficiency() const { return m_maxBurningEfficiency; }
        inline double getMaxTurbulenceEffect() const { return m_maxTurbulenceEffect; }
        inline double getMaxDilutionEffect() const { return m_maxDilutionEffect; }

        double flameSpeed(
            double turbulence,
            double molecularAfr,
            double T,
            double P,
            double firingPressure,
            double motoringPressure) const;
        virtual double laminarBurningVelocity(double molecularAfr, double T, double P) const;

        double getMolecularAfr() const { return m_molecularAfr; }

    protected:
        std::string m_name;
        double m_molecularMass;
        double m_energyDensity;
        double m_density;
        double m_molecularAfr;
        double m_maxBurningEfficiency;
        double m_burningEfficiencyRandomness;
        double m_lowEfficiencyAttenuation;
        double m_maxTurbulenceEffect;
        double m_maxDilutionEffect;

        Function *m_turbulenceToFlameSpeedRatio;
};

#endif /* ATG_ENGINE_FUEL_H */
