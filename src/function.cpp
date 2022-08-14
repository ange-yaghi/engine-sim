#include "../include/function.h"

#include <algorithm>
#include <string.h>
#include <assert.h>
#include <cmath>

GaussianFilter *Function::DefaultGaussianFilter = nullptr;

Function::Function() {
    m_x = m_y = nullptr;
    m_capacity = 0;
    m_size = 0;
    m_filterRadius = 0;
    m_yMin = m_yMax = 0;
    m_inputScale = 1.0;
    m_outputScale = 1.0;

    if (DefaultGaussianFilter == nullptr) {
        DefaultGaussianFilter = new GaussianFilter;
        DefaultGaussianFilter->initialize(1.0, 3.0, 1024);
    }

    m_gaussianFilter = nullptr;
}

Function::~Function() {
    assert(m_x == nullptr);
    assert(m_y == nullptr);
}

void Function::initialize(int size, double filterRadius, GaussianFilter *filter) {
    resize(size);
    m_size = 0;
    m_filterRadius = filterRadius;

    m_gaussianFilter = (filter != nullptr)
        ? filter
        : DefaultGaussianFilter;
}

void Function::resize(int newCapacity) {
    double *new_x = new double[newCapacity];
    double *new_y = new double[newCapacity];

    if (m_size > 0) {
        memcpy(new_x, m_x, sizeof(double) * m_size);
        memcpy(new_y, m_y, sizeof(double) * m_size);
    }

    delete[] m_x;
    delete[] m_y;

    m_x = new_x;
    m_y = new_y;

    m_capacity = newCapacity;
}

void Function::destroy() {
    delete[] m_x;
    delete[] m_y;

    m_x = nullptr;
    m_y = nullptr;

    m_capacity = 0;
    m_size = 0;
}

void Function::addSample(double x, double y) {
    if (m_size + 1 > m_capacity) {
        resize(m_capacity * 2 + 1);
    }

    m_yMin = std::fmin(m_yMin, y);
    m_yMax = std::fmax(m_yMax, y);

    const int closest = closestSample(x);
    if (closest == -1) {
        m_size = 1;

        m_x[0] = x;
        m_y[0] = y;
        return;
    }

    const int index = x < m_x[closest]
        ? closest
        : closest + 1;

    ++m_size;

    const size_t sizeToCopy = (size_t)m_size - index - 1;
    if (sizeToCopy > 0) {
        memmove(m_x + index + 1, m_x + index, sizeof(double) * sizeToCopy);
        memmove(m_y + index + 1, m_y + index, sizeof(double) * sizeToCopy);
    }

    m_x[index] = x;
    m_y[index] = y;
}

double Function::sampleTriangle(double x) const {
    x *= m_inputScale;
    const int closest = closestSample(x);

    if (m_size == 0) return 0;
    else if (x >= m_x[m_size - 1]) return m_y[m_size - 1] * m_outputScale;
    else if (x <= m_x[0]) return m_y[0] * m_outputScale;

    double sum = 0;
    double totalWeight = 0;
    for (int i = closest; i >= 0; --i) {
        if (m_x[i] > x) continue;
        if (std::abs(x - m_x[i]) > m_filterRadius) break;

        const double w = triangle(m_x[i] - x);
        sum += w * m_y[i];
        totalWeight += w;
    }

    for (int i = closest; i < m_size; ++i) {
        if (m_x[i] <= x) continue;
        if (std::abs(m_x[i] - x) > m_filterRadius) break;

        const double w = triangle(m_x[i] - x);
        sum += w * m_y[i];
        totalWeight += w;
    }

    return (totalWeight != 0)
        ? sum * m_outputScale / totalWeight
        : 0;
}

double Function::sampleGaussian(double x) const {
    x *= m_inputScale;
    const int closest = closestSample(x);
    const double filterRadius = m_filterRadius * m_gaussianFilter->getRadius();

    double sum = 0;
    double totalWeight = 0;

    if (m_size == 0) return 0;
    else if (x > m_x[m_size - 1]) {
        const double w = m_gaussianFilter->evaluate(0);
        sum += w * m_y[m_size - 1];
        totalWeight += w;
    }
    else if (x < m_x[0]) {
        const double w = m_gaussianFilter->evaluate(0);
        sum += w * m_y[0];
        totalWeight += w;
    }

    for (int i = closest; i >= 0; --i) {
        if (std::abs(x - m_x[i]) > filterRadius) break;

        const double w = m_gaussianFilter->evaluate((m_x[i] - x) / m_filterRadius);
        sum += w * m_y[i];
        totalWeight += w;
    }

    for (int i = closest + 1; i < m_size; ++i) {
        if (std::abs(m_x[i] - x) > filterRadius) break;

        const double w = m_gaussianFilter->evaluate((m_x[i] - x) / m_filterRadius);
        sum += w * m_y[i];
        totalWeight += w;
    }

    return (totalWeight != 0)
        ? sum * m_outputScale / totalWeight
        : 0;
}

bool Function::isOrdered() const {
    for (int i = 0; i < m_size - 1; ++i) {
        if (m_x[i] > m_x[i + 1]) return false;
    }

    return true;
}

void Function::getDomain(double *x0, double *x1) {
    if (m_size == 0) {
        *x0 = *x1 = 0;
    }
    else {
        *x0 = m_x[0];
        *x1 = m_x[m_size - 1];
    }
}

void Function::getRange(double *y0, double *y1) {
    *y0 = m_yMin;
    *y1 = m_yMax;
}

double Function::triangle(double x) const {
    return (m_filterRadius - std::abs(x)) / m_filterRadius;
}

int Function::closestSample(double x) const {
    if (std::isnan(x)) {
        return 0;
    }

    int l = 0;
    int r = m_size - 1;

    if (m_size == 0) return -1;
    else if (x <= m_x[l]) return l;
    else if (x >= m_x[r]) return r;

    while (l + 1 < r) {
        const int m = (l + r) / 2;
        if (x > m_x[m]) {
            l = m;
        }
        else if (x < m_x[m]) {
            r = m;
        }
        else if (x == m_x[m]) {
            return m;
        }
    }

    return (x - m_x[l] < m_x[r] - x)
        ? l
        : r;
}
