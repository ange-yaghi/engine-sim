#ifndef ENGINE_SIM_SLE_SOLVER_H
#define ENGINE_SIM_SLE_SOLVER_H

#include "matrix.h"

class SleSolver {
public:
    SleSolver();
    virtual ~SleSolver();

    virtual bool solve(Matrix *left, Matrix *right, Matrix *result, Matrix *previous);
};

#endif /* ENGINE_SIM_SLE_SOLVER_H */
