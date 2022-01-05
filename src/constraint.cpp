#include "../include/constraint.h"

Constraint::Constraint(int constraintCount) {
    m_index = 0;
    m_constraintCount = constraintCount;
}

Constraint::~Constraint() {
    /* void */
}

void Constraint::calculate(Output *output, int body_i, OdeSolver::System *system) {
    output->n = 0;
}
