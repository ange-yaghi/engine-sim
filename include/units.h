#ifndef ATG_ENGINE_SIM_UNITS_H
#define ATG_ENGINE_SIM_UNITS_H

#include "constants.h"

namespace units {
    // Force
    static constexpr double N = 1.0;

    static constexpr double lbf = N * 4.44822;

    // Mass
    static constexpr double kg = 1.0;
    static constexpr double g = kg / 1000.0;

    static constexpr double lb = 0.45359237 * kg;

    // Distance
    static constexpr double m = 1.0;
    static constexpr double cm = m / 100.0;
    static constexpr double mm = m / 1000.0;
    static constexpr double km = m * 1000.0;

    static constexpr double inch = cm * 2.54;
    static constexpr double foot = inch * 12.0;
    static constexpr double thou = inch / 1000.0;

    static constexpr double mile = m * 1609.344;

    // Time
    static constexpr double sec = 1.0;
    static constexpr double minute = 60 * sec;
    static constexpr double hour = 60 * minute;

    // Torque
    static constexpr double Nm = N * m;
    static constexpr double ft_lb = foot * lbf;

    // Power
    static constexpr double W = Nm / sec;
    static constexpr double kW = W * 1000.0;
    static constexpr double hp = 745.699872 * W;

    // Volume
    static constexpr double m3 = 1.0;
    static constexpr double cc = cm * cm * cm;
    static constexpr double mL = cc;
    static constexpr double L = mL * 1000.0;
    static constexpr double cubic_feet = foot * foot * foot;
    static constexpr double cubic_inches = inch * inch * inch;
    static constexpr double gal = 3.785411784 * L;

    // Molecular
    static constexpr double mol = 1.0;
    static constexpr double kmol = mol / 1000.0;
    static constexpr double mmol = mol / 1000000.0;
    static constexpr double lbmol = mol * 453.59237;

    // Flow-rate (moles)
    static constexpr double mol_per_sec = mol / sec;
    static constexpr double scfm = 0.002641 * lbmol / minute;

    // Area
    static constexpr double m2 = 1.0;
    static constexpr double cm2 = cm * cm;

    // Pressure
    static constexpr double Pa = 1.0;
    static constexpr double kPa = Pa * 1000.0;
    static constexpr double MPa = Pa * 1000000.0;
    static constexpr double atm = 101.325 * kPa;

    static constexpr double mbar = Pa * 100.0;
    static constexpr double bar = mbar * 1000.0;

    static constexpr double psi = lbf / (inch * inch);
    static constexpr double psig = psi;
    static constexpr double inHg = Pa * 3386.3886666666713;
    static constexpr double inH2O = inHg * 0.0734824;

    // Temperature
    static constexpr double K = 1.0;
    static constexpr double K0 = 273.15;
    static constexpr double C = K;
    static constexpr double F = (5.0 / 9.0) * K;
    static constexpr double F0 = -459.67;

    // Energy
    static constexpr double J = 1.0;
    static constexpr double kJ = J * 1000;
    static constexpr double MJ = J * 1000000;

    // Angles
    static constexpr double rad = 1.0;
    static constexpr double deg = rad * (constants::pi / 180);

    // Conversions
    inline constexpr double distance(double v, double unit) {
        return v * unit;
    }

    inline constexpr double area(double v, double unit) {
        return v * unit;
    }

    inline constexpr double torque(double v, double unit) {
        return v * unit;
    }

    inline constexpr double rpm(double rpm) {
        return rpm * 0.104719755;
    }

    inline constexpr double toRpm(double rad_s) {
        return rad_s / 0.104719755;
    }

    inline constexpr double pressure(double v, double unit) {
        return v * unit;
    }

    inline constexpr double psia(double p) {
        return units::pressure(p, units::psig) - units::pressure(1.0, units::atm);
    }

    inline constexpr double toPsia(double p) {
        return (p + units::pressure(1.0, units::atm)) / units::psig;
    }

    inline constexpr double mass(double v, double unit) {
        return v * unit;
    }

    inline constexpr double force(double v, double unit) {
        return v * unit;
    }

    inline constexpr double volume(double v, double unit) {
        return v * unit;
    }

    inline constexpr double flow(double v, double unit) {
        return v * unit;
    }

    inline constexpr double convert(double v, double unit0, double unit1) {
        return v * (unit0 / unit1);
    }

    inline constexpr double convert(double v, double unit) {
        return v / unit;
    }

    inline constexpr double celcius(double T_C) {
        return T_C * C + K0;
    }

    inline constexpr double kelvin(double T) {
        return T * K;
    }

    inline constexpr double fahrenheit(double T_F) {
        return F * (T_F - F0);
    }

    inline constexpr double toAbsoluteFahrenheit(double T) {
        return T / F;
    }

    inline constexpr double angle(double v, double unit) {
        return v * unit;
    }

    inline constexpr double energy(double v, double unit) {
        return v * unit;
    }

    // Physical Constants
    constexpr double AirMolecularMass = units::mass(28.97, units::g) / units::mol;
};

#endif /* ATG_ENGINE_SIM_UNITS_H */
