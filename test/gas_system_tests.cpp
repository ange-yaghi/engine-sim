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
        units::celcius(2500.0)
    );

    system2.initialize(
        units::pressure(2.0, units::atm),
        units::volume(1.0, units::cc),
        units::celcius(25.0)
    );

    const double maxFlowIn = system1.pressureEquilibriumMaxFlow(&system2);

    system1.start();
    system2.start();

    system1.flow(maxFlowIn, system2.kineticEnergyPerMol(), system2.mix());
    system2.flow(-maxFlowIn, system1.kineticEnergyPerMol(), system1.mix());

    system1.end();
    system2.end();

    EXPECT_NEAR(system1.pressure(), system2.pressure(), 1E-6);

    system1.start();
    system1.changePressure(units::pressure(100.0, units::atm));
    system1.end();

    const double maxFlowOut = system1.pressureEquilibriumMaxFlow(&system2);

    system1.start();
    system2.start();

    system1.flow(maxFlowOut, system2.kineticEnergyPerMol(), system2.mix());
    system2.flow(-maxFlowOut, system1.kineticEnergyPerMol(), system1.mix());

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

    constexpr double P_env = units::pressure(2.0, units::atm);
    constexpr double T_env = units::celcius(25.0);

    const double maxFlow = system1.pressureEquilibriumMaxFlow(P_env, T_env);
    const double E_k_per_mol = GasSystem::kineticEnergyPerMol(T_env, system1.degreesOfFreedom());

    system1.start();
    system1.flow(maxFlow, E_k_per_mol);
    system1.end();

    EXPECT_NEAR(system1.pressure(), P_env, 1E-6);
}

TEST(GasSystemTests, PressureEquilibriumMaxFlowInfiniteOverpressure) {
    GasSystem system1;
    system1.initialize(
        units::pressure(100.0, units::atm),
        units::volume(1.0, units::m3),
        units::celcius(2500.0)
    );

    constexpr double P_env = units::pressure(2.0, units::atm);
    constexpr double T_env = units::celcius(25.0);

    const double maxFlow = system1.pressureEquilibriumMaxFlow(P_env, T_env);

    system1.start();
    system1.flow(maxFlow, T_env);
    system1.end();

    EXPECT_NEAR(system1.pressure(), P_env, 1E-6);
}

TEST(GasSystemTests, FlowVariableVolume) {
    GasSystem system1;
    system1.initialize(
        units::pressure(100.0, units::atm),
        units::volume(1.0, units::m3),
        units::celcius(25.0)
    );

    constexpr double P_env = units::pressure(2.0, units::atm);
    constexpr double T_env = units::celcius(25.0);

    const double maxFlow = system1.pressureEquilibriumMaxFlow(P_env, T_env);

    constexpr double dV = units::volume(1000000.0, units::cc) / 100;
    for (int i = 0; i < 100; ++i) {
        system1.start();
        const double flowRate0 = system1.flow(0.01, 1/60.0, units::pressure(0.1, units::atm), units::celcius(25.0));
        const double flowRate1 = system1.flow(0.01, 1 / 60.0, units::pressure(0.2, units::atm), units::celcius(25.0));
        system1.changeVolume(-dV);
        system1.changeTemperature(100);
        system1.end();

        std::cerr << flowRate0 << ", " << flowRate1 << "\n";
    }
}

TEST(GasSystemTests, PowerStrokeTest) {
    GasSystem system1;
    system1.initialize(
        units::pressure(100.0, units::atm),
        units::volume(1.0, units::m3),
        units::celcius(2000.0)
    );

    constexpr double dV = units::volume(1000000.0, units::cc) / 100;
    for (int i = 0; i < 100; ++i) {
        system1.start();
        const double flowRate0 = system1.flow(1.0, 1 / 60.0, units::pressure(1.0, units::atm), units::celcius(25.0));
        if (flowRate0 < 0) {
            int a = 0;
        }

        //system1.changeVolume(-dV);
        system1.end();

        std::cerr << i << ", " << flowRate0 << ", " << system1.pressure() << "\n";
    }
}

TEST(GasSystemTests, FlowLimit) {
    GasSystem system1;
    system1.initialize(
        units::pressure(100.0, units::atm),
        units::volume(1.0, units::m3),
        units::celcius(2000.0)
    );

    constexpr double P_env = units::pressure(1.0, units::atm);
    constexpr double T_env = units::celcius(25.0);
    const double maxFlow = system1.pressureEquilibriumMaxFlow(P_env, T_env);

    system1.start();
    system1.flow(15.0, 10.0, P_env, T_env);
    system1.end();

    EXPECT_NEAR(system1.pressure(), P_env, 1E-6);
}

TEST(GasSystemTests, IdealGasLaw) {
    GasSystem system1;
    system1.initialize(
        units::pressure(100.0, units::atm),
        units::volume(1.0, units::m3),
        units::celcius(2000.0)
    );

    const double PV = system1.pressure() * system1.volume();
    const double nRT = system1.n() * constants::R * system1.temperature();

    EXPECT_NEAR(PV, nRT, 1E-6);
}

TEST(GasSystemTests, CompositionSanityCheck) {
    GasSystem::Mix a, b;
    a.p_fuel = 1.0;
    a.p_inert = 1.0;
    a.p_o2 = 1.0;

    b.p_fuel = 0.0;
    b.p_inert = 0.0;
    b.p_o2 = 0.0;

    GasSystem system1;
    system1.initialize(
        units::pressure(100.0, units::atm),
        units::volume(100.0, units::m3),
        units::celcius(2000.0),
        a
    );

    GasSystem system2;
    system2.initialize(
        units::pressure(1.0, units::atm),
        units::volume(1.0, units::m3),
        units::celcius(2000.0),
        b
    );

    const double PV = system1.pressure() * system1.volume();
    const double nRT = system1.n() * constants::R * system1.temperature();

    for (int i = 0; i < 200; ++i) {
        system1.start();
        system2.start();
        const double flowRate0 = system1.flow(
            1.0,
            1 / 60.0,
            &system2);
        system1.end();
        system2.end();

        std::cerr << i << ", " << flowRate0 << ", " << system1.pressure() << "\n";
    }

    EXPECT_NEAR(system2.mix().p_fuel, 1.0, 2E-2);
    EXPECT_NEAR(system2.mix().p_inert, 1.0, 2E-2);
    EXPECT_NEAR(system2.mix().p_o2, 1.0, 2E-2);
}

TEST(GasSystemTests, ChokedFlowTest) {
    GasSystem system1;
    system1.initialize(
        units::pressure(2.5, units::atm),
        units::volume(1.0, units::m3),
        units::celcius(2000.0)
    );

    const double flow_k = GasSystem::flowConstant(
        units::flow(400, units::scfm),
        units::pressure(2.5, units::atm),
        units::pressure(1.5, units::atm),
        units::celcius(2000.0),
        GasSystem::heatCapacityRatio(5)
    );

    const double chokedFlow =
        system1.flowRate(
            flow_k,
            system1.pressure(),
            units::pressure(1.0, units::atm),
            system1.temperature(),
            units::celcius(25),
            GasSystem::heatCapacityRatio(5),
            GasSystem::chokedFlowLimit(5),
            GasSystem::chokedFlowRate(5));
    const double noncriticalFlow =
        system1.flowRate(
            flow_k,
            system1.pressure(),
            units::pressure(2.0, units::atm),
            system1.temperature(),
            units::celcius(25),
            GasSystem::heatCapacityRatio(5),
            GasSystem::chokedFlowLimit(5),
            GasSystem::chokedFlowRate(5));

    const double chokedFlowScfm = units::convert(chokedFlow, units::scfm);
    const double noncriticalFlowScfm = units::convert(noncriticalFlow, units::scfm);
}

TEST(GasSystemTests, CfmConversions) {
    constexpr double standardPressure = units::pressure(1.0, units::atm);
    constexpr double standardTemp = units::celcius(25.0);
    constexpr double airDensity =
        units::AirMolecularMass * (standardPressure * units::volume(1.0, units::m3))
        / (constants::R * standardTemp);

    const double flow_28 = GasSystem::k_28inH2O(300);
    
    const double flowRate = GasSystem::flowRate(
        flow_28,
        units::pressure(1.0, units::atm),
        units::pressure(1.0, units::atm) - units::pressure(41.0, units::inH2O),
        units::celcius(25.0),
        units::celcius(25.0),
        GasSystem::heatCapacityRatio(5),
        GasSystem::chokedFlowLimit(5),
        GasSystem::chokedFlowRate(5));

    const double flowRateCfm = units::convert(flowRate, units::scfm);
}

TEST(GasSystemTests, FlowRateConstant) {
    const double flow_k = GasSystem::flowConstant(
        units::flow(400, units::scfm),
        units::pressure(2.5, units::atm),
        units::pressure(0.5, units::atm),
        units::celcius(2000.0),
        GasSystem::heatCapacityRatio(5)
    );

    const double flowRate =
        GasSystem::flowRate(
            flow_k,
            units::pressure(2.5, units::atm),
            units::pressure(2.5 - 0.5, units::atm),
            units::celcius(2000.0),
            units::celcius(25),
            GasSystem::heatCapacityRatio(5),
            GasSystem::chokedFlowLimit(5),
            GasSystem::chokedFlowRate(5));

    EXPECT_NEAR(flowRate, units::flow(400, units::scfm), 1E-6);
}
