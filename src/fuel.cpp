#include "../include/fuel.h"

Fuel::Fuel() {
    m_molecularMass = 0.0;
    m_energyDensity = 0.0;
    m_density = 0.0;
}

Fuel::~Fuel() {
    /* void */
}

void Fuel::initialize(const Parameters &params) {
    m_molecularMass = params.MolecularMass;
    m_energyDensity = params.EnergyDensity;
    m_density = params.Density;
}
