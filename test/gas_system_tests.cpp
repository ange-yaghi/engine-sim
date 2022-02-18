#include <gtest/gtest.h>

#include "../include/gas_system.h"
#include "../include/units.h"

TEST(GasSystemTests, GasSystemSanity) {
    GasSystem system;
    system.initialize(0.0, 0.0, 0.0);
}

TEST(GasSystemTests, AdiabaticEnergyConservation) {
    const double pistonArea = units::area(1.0, units::cm2);
    const double vesselHeight = units::distance(1.0, units::cm);
    const double compression = vesselHeight * 0.5;
    const int steps = 10000;

    GasSystem system;
    system.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1.0, units::cc),
        units::celcius(25.0)
    );

    const double initialSystemEnergy = system.kineticEnergy();
    const double initialMolecules = system.n();

    double W = 0.0;
    double currentPistonHeight = vesselHeight;
    for (int i = 1; i <= steps; ++i) {
        const double newPistonHeight = vesselHeight - (compression / steps) * i;
        const double dH = (currentPistonHeight - newPistonHeight);
        const double F = system.pressure() * pistonArea;
        W += F * dH;

        system.start();
        system.changeVolume((newPistonHeight - currentPistonHeight) * pistonArea);
        system.end();

        currentPistonHeight = newPistonHeight;
    }

    const double finalSystemEnergy = system.kineticEnergy();
    const double finalMolecules = system.n();

    EXPECT_NEAR(finalMolecules, initialMolecules, 1E-6);
    EXPECT_NEAR(finalSystemEnergy - initialSystemEnergy, W, 1E-4);
}

TEST(GasSystemTests, PressureEqualizationEnergyConservation) {
    GasSystem system1, system2;
    system1.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1.0, units::cc),
        units::celcius(25.0)
    );

    system2.initialize(
        units::pressure(2.0, units::atm),
        units::volume(1.0, units::cc),
        units::celcius(25.0)
    );

    const double initialSystemEnergy = system1.kineticEnergy() + system2.kineticEnergy();
    const double initialMolecules = system1.n() + system2.n();

    const double dt = 1.0;
    const double flow_k = 0.000001;
    const int steps = 100;
    for (int i = 1; i <= steps; ++i) {
        system1.start();
        system2.start();

        system1.flow(flow_k, dt, &system2);

        system1.end();
        system2.end();
    }

    const double finalSystemEnergy = system1.kineticEnergy() + system2.kineticEnergy();
    const double finalMolecules = system1.n() + system2.n();

    EXPECT_NEAR(finalMolecules, initialMolecules, 1E-6);
    EXPECT_NEAR(finalSystemEnergy, initialSystemEnergy, 1E-4);
}

TEST(GasSystemTests, PressureEquilibriumMaxFlow) {
    GasSystem system1, system2;
    system1.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1.0, units::cc),
        units::celcius(25.0)
    );

    system2.initialize(
        units::pressure(2.0, units::atm),
        units::volume(1.0, units::cc),
        units::celcius(25.0)
    );

    const double maxFlow = system1.pressureEquilibriumMaxFlow(&system2);

    system1.start();
    system2.start();

    system1.flow(maxFlow, &system2);

    system1.end();
    system2.end();

    EXPECT_NEAR(system1.pressure(), system2.pressure(), 1E-6);
}

TEST(GasSystemTests, PressureEquilibriumMaxFlowInfinite) {
    GasSystem system1;
    system1.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1.0, units::cc),
        units::celcius(25.0)
    );

    const double P_env = units::pressure(2.0, units::atm);
    const double T_env = units::celcius(25.0);

    const double maxFlow = system1.pressureEquilibriumMaxFlow(P_env, T_env);

    system1.start();
    system1.flow(maxFlow, T_env);
    system1.end();

    EXPECT_NEAR(system1.pressure(), P_env, 1E-6);
}
