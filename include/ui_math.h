#ifndef ATG_ENGINE_SIM_UI_MATH_H
#define ATG_ENGINE_SIM_UI_MATH_H

#include <cmath>

struct Point {
    float x, y;

    Point(float x, float y) : x(x), y(y) { /* void */ }
    Point(float s) : x(s), y(s) { /* void */ }
    Point() : x(0), y(0) { /* void */ }

    Point operator+(const Point &b) const {
        return { x + b.x, y + b.y };
    }

    Point operator-(const Point &b) const {
        return { x - b.x, y - b.y };
    }

    Point operator-() const {
        return { -x, -y };
    }

    Point operator*(const Point &b) const {
        return { x * b.x, y * b.y };
    }

    Point operator*(float s) const {
        return { x * s, y * s };
    }

    Point operator/(const Point &b) const {
        return { x / b.x, y / b.y };
    }

    Point operator/(float s) const {
        return { x / s, y / s };
    }

    Point operator+=(const Point &b) {
        x += b.x;
        y += b.y;
        return { x, y };
    }

    Point operator-=(const Point &b) {
        x -= b.x;
        y -= b.y;
        return { x, y };
    }

    Point operator/=(const Point &b) {
        x /= b.x;
        y /= b.y;
        return { x, y };
    }

    Point operator*=(const Point &b) {
        x *= b.x;
        y *= b.y;
        return { x, y };
    }

    bool operator>(const Point &b) const {
        return x > b.x && y > b.y;
    }

    bool operator>=(const Point &b) const {
        return x >= b.x && y >= b.y;
    }

    bool operator<(const Point &b) const {
        return x < b.x && y < b.y;
    }

    bool operator<=(const Point &b) const {
        return x <= b.x && y <= b.y;
    }

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    float lengthSquared() const {
        return x * x + y * y;
    }

    float dot(const Point &b) const {
        return x * b.x + y * b.y;
    }

    Point normalized() const {
        return (*this) / length();
    }

    Point componentMax(const Point &b) const {
        return { std::fmax(x, b.x), std::fmax(y, b.y) };
    }

    Point componentMin(const Point &b) const {
        return { std::fmin(x, b.x), std::fmin(y, b.y) };
    }
};

struct Bounds {
    static const Point center;
    static const Point tl, tr, tm;
    static const Point bl, br, bm;
    static const Point lm, rm;

    Point m0;
    Point m1;

    Bounds() { /* void */ }
    Bounds(const Point &a, const Point &b) : m0(a), m1(b) { /* void */ }
    Bounds(float x0, float x1, float y0, float y1) {
        m0 = { std::fmin(x0, x1), std::fmin(y0, y1) };
        m1 = { std::fmax(x0, x1), std::fmax(y0, y1) };
    }
    Bounds(float width, float height, const Point &pos, const Point &ref = tl) {
        m0 = Point(0, 0);
        m1 = Point(width, height);
        setPosition(pos, ref);
    }

    bool overlaps(const Point &p) const {
        return p >= m0 && p <= m1;
    }

    Bounds add(const Bounds &b) const {
        return { m0.componentMin(b.m0), m1.componentMax(b.m1) };
    }

    Bounds move(const Point &delta) {
        m0 += delta;
        m1 += delta;

        return *this;
    }

    void setPosition(const Point &pos, const Point &ref = tl) {
        move(pos - getPosition(ref));
    }

    Point getPosition(const Point &ref = tl) const {
        const Point offset = ref * dim();
        return m0 + offset;
    }

    float left() const { return m0.x; }
    float right() const { return m1.x; }
    float top() const { return m1.y; }
    float bottom() const { return m0.y; }
    float center_h() const { return (m0.x + m1.x) / 2; }
    float center_v() const { return (m0.y + m1.y) / 2; }

    float width() const { return m1.x - m0.x; }
    float height() const { return m1.y - m0.y; }
    Point dim() const { return Point(width(), height()); }

    Bounds inset(float amount) const {
        return { m0 + amount, m1 - amount };
    }

    Bounds grow(float amount) const {
        return inset(-amount);
    }

    Bounds verticalSplit(float s0, float s1) const {
        const float s_min = std::fmin(s0, s1);
        const float s_max = std::fmax(s0, s1);

        return {
                m0 + Point(0.0f, height() * s_min),
                m0 + Point(0.0f, height() * s_max) + Point(width(), 0.0f) };
    }

    Bounds horizontalSplit(float s0, float s1) const {
        const float s_min = std::fmin(s0, s1);
        const float s_max = std::fmax(s0, s1);

        return {
                m0 + Point(width() * s_min, 0.0f),
                m0 + Point(width() * s_max, 0.0f) + Point(0.0f, height()) };
    }
};

struct Grid {
    int h_cells;
    int v_cells;

    Bounds get(const Bounds &a, int x, int y, int w = 1, int h = 1) const {
        const float cellWidth = a.width() / h_cells;
        const float cellHeight = a.height() / v_cells;

        const float width = cellWidth * w;
        const float height = cellHeight * h;

        const Point p0 = a.getPosition(Bounds::tl) + Point(x * cellWidth, -y * cellHeight);
        return Bounds(width, height, p0, Bounds::tl);
    }
};

#endif /* ATG_ENGINE_SIM_UI_MATH_H */
