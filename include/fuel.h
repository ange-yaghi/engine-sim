#ifndef ATG_ENGINE_FUEL_H
#define ATG_ENGINE_FUEL_H

#include "units.h"

class Fuel {
    public:
        struct Parameters {
            double MolecularMass =
                units::mass(100.0, units::g);
            double EnergyDensity =
                units::energy(48.1, units::kJ) / units::mass(1.0, units::g);
            double Density =
                units::mass(0.755, units::kg) / units::volume(1.0, units::L);
        };

        Fuel();
        ~Fuel();

        void initialize(const Parameters &params);

        inline double getMolecularMass() const { return m_molecularMass; }
        inline double getEnergyDensity() const { return m_energyDensity; }
        inline double getDensity() const { return m_density; }

    protected:
        double m_molecularMass;
        double m_energyDensity;
        double m_density;
};

#endif /* ATG_ENGINE_FUEL_H */
