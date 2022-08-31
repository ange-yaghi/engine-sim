#include "../include/utilities.h"

#include <cmath>

double modularDistance(double a, double b, double mod) {
    return (a < b) ? std::fmin(b - a, a + mod - b) : std::fmin(a - b, b + mod - a);
}

double positiveMod(double x, double mod) {
    if (x < 0) {
        x = std::ceil(-x / mod) * mod + x;
    }

    return std::fmod(x, mod);
}

double erfApproximation(double x) {
    const double a1 = 0.278393;
    const double a2 = 0.230389;
    const double a3 = 0.000972;
    const double a4 = 0.078108;

    const double x2 = x * x;
    const double x3 = x2 * x;
    const double x4 = x3 * x;

    const double q = 1 / (1 + a1 * x + a2 * x2 + a3 * x3 + a4 * x4);
    const double q2 = q * q;
    const double q4 = q2 * q2;

    return 1 - q4;
}
