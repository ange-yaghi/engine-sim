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
