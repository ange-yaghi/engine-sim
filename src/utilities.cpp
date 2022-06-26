#include "../include/utilities.h"

#include <cmath>

double modularDistance(double a0, double b0, double mod) {
    double a, b;
    if (a0 < b0) {
        a = a0;
        b = b0;
    }
    else {
        a = b0;
        b = a0;
    }

    return std::fmin(b - a, a + mod - b);
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

double clamp(double x, double x0, double x1) {
    if (x <= x0) return x0;
    else if (x >= x1) return x1;
    else return x;
}
