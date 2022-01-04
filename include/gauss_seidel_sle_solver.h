#ifndef ENGINE_SIM_GAUSS_SEIDEL_SLE_SOLVER_H
#define ENGINE_SIM_GAUSS_SEIDEL_SLE_SOLVER_H

#include "sle_solver.h"

class GaussSeidelSleSolver : public SleSolver {
public:
    GaussSeidelSleSolver();
    virtual ~GaussSeidelSleSolver();
    
    virtual bool solve(Matrix *left, Matrix *right, Matrix *result, Matrix *previous);

protected:
    double solveIteration(Matrix *left, Matrix *right, Matrix *k_next, Matrix *k);

    int m_maxIterations;
    double m_minDelta;
};

#endif /* ENGINE_SIM_GAUSS_SEIDEL_SLE_SOLVER_H */
