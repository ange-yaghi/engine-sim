#include "../include/ui_utilities.h"

ysVector mix(const ysVector &c1, const ysVector &c2, float s) {
    return ysMath::Add(
            ysMath::Mul(c1, ysMath::LoadScalar(1 - s)),
            ysMath::Mul(c2, ysMath::LoadScalar(s)));
}
