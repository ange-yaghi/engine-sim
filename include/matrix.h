#ifndef ENGINE_SIM_MATRIX_H
#define ENGINE_SIM_MATRIX_H

class Matrix {
public:
    Matrix();
    Matrix(int width, int height, double value = 0.0f);
    ~Matrix();

    void initialize(int width, int height, double value = 0.0f);
    void destroy();
    void set(int column, int row, double value);
    double get(int column, int row);

    void set(Matrix *reference);

    void multiply(Matrix &b, Matrix *target);
    void subtract(Matrix &b, Matrix *target);
    void negate(Matrix *target);
    void transpose(Matrix *target);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

protected:
    double **m_matrix;
    double *m_data;
    int m_width;
    int m_height;
};

#endif /* ENGINE_SIM_MATRIX_H */
