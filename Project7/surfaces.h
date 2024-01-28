#ifndef SURFACES_H
#define SURFACES_H

#include <functional>
#include <iostream>
#include <cmath>
#include "real.h"

constexpr Real DEG_TO_RAD = M_PI / 180.0;

class Point {
public:
    const Real x;
    const Real y;

    Point(Real x_, Real y_) : x(x_), y(y_) {}

    friend std::ostream& operator<<(std::ostream& os, const Point &p) {
        os << p.x << " " << p.y;
        return os;
    }
};

using Surface = std::function<Real(const Point&)>;

// Flat surface, always returns 0.
inline Surface plain() {
    return []([[maybe_unused]] const Point &p)->Real {
        return 0;
    };
}

// Sloping surface, returns x value of the point.
inline Surface slope() {
    return [](const Point &p)->Real {
      return p.x;
    };
}

// Steps surface, creates steps with width s.
inline Surface steps(const Real &s = 1) {
    return [s](const Point &p)->Real {
        return (s <= 0) ? 0 : std::floor(p.x / s);
    };
}

// Checker surface, creates a checkerboard pattern of size s.
inline Surface checker(const Real &s = 1) {
    return [s](const Point &p)->Real {
        return (s <= 0 ||
                (static_cast<int>(std::floor(p.x / s)) +
                 static_cast<int>(std::floor(p.y / s))) % 2 != 0) ? 0 : 1;
    };
}

// Square surface, returns the square of the x value of the point.
inline Surface sqr() {
    return [](const Point &p)->Real {
        return p.x * p.x;
    };
}

// Sinusoidal wave surface.
inline Surface sin_wave() {
    return [](const Point &p)->Real {
        return std::sin(p.x);
    };
}

// Cosinusoidal wave surface.
inline Surface cos_wave() {
    return [](const Point &p)->Real {
        return std::cos(p.x);
    };
}

// Rings surface, creates a pattern of rings with width s.
inline Surface rings(const Real &s = 1) {
    return [s](const Point &p)->Real {
        Real mod = (s > 0) ?
                std::fabs(std::fmod(std::sqrt(p.x * p.x + p.y * p.y) / s, 2))
                : 2;

        return ((p.x == 0 && p.y == 0 && s > 0) ||
                (mod <= 1 && mod > 0)) ? 1 : 0;
    };
}

// Ellipse surface, creates an ellipse with radii a and b.
inline Surface ellipse(const Real &a = 1, const Real &b = 1) {
    return [a, b](const Point &p)->Real {
        return (a <= 0 || b <= 0 ||
            ((p.x * p.x) / (a * a) + (p.y * p.y) / (b * b)) > 1) ? 0 : 1;
    };
}

// Rectangle surface, creates a rectangle with sides a and b.
inline Surface rectangle(const Real &a = 1, const Real &b = 1) {
    return [a, b](const Point &p)->Real {
        return (a > 0 && b > 0 &&
                std::abs(p.x) <= a && std::abs(p.y) <= b) ? 1 : 0;
    };
}

// Stripes surface, creates a pattern of stripes with width s.
inline Surface stripes(const Real &s) {
    return [s](const Point &p)->Real {
        Real normalized_x = (s > 0) ? std::fmod(p.x, 2 * s) : 0;
        normalized_x = (p.x < 0) ? normalized_x + 2 * s : normalized_x;

        return (s <= 0 || p.x == 0 ||
                normalized_x > s || normalized_x <= 0) ? 0 : 1;
    };
}

// Function to rotate a surface by a specified degree.
inline Surface rotate(const Surface &f, const Real &deg) {
    Real rad = -deg * DEG_TO_RAD;

    return [&f, rad](const Point &p)->Real {
        return f(Point(p.x * cos(rad) - p.y * sin(rad),
                p.x * sin(rad) + p.y * cos(rad)));
    };
}

// Function to translate a surface by a vector.
inline Surface translate(const Surface &f, const Point &v) {
    return [&f, v](const Point &p)->Real {
        return f(Point(p.x - v.x, p.y - v.y));
    };
}

// Function to scale a surface by a vector.
inline Surface scale(const Surface &f, const Point &s) {
    return [&f, s](const Point &p)->Real {
        return f(Point(p.x / s.x, p.y / s.y));
    };
}

// Function to invert the axes of a surface.
inline Surface invert(const Surface &f) {
    return [&f](const Point &p)->Real {
        return f(Point(p.y, p.x));
    };
}

// Function to flip a surface horizontally.
inline Surface flip(const Surface &f) {
    return [&f](const Point &p)->Real {
        return f(Point(-p.x, p.y));
    };
}

// Function to multiply the output of a surface by a constant.
inline Surface mul(const Surface &f, const Real &c) {
    return [&f, c](const Point &p)->Real {
        return f(p) * c;
    };
}

// Function to add a constant to the output of a surface.
inline Surface add(const Surface &f, const Real &c) {
    return [&f, c](const Point &p)->Real {
        return f(p) + c;
    };
}

// This is a utility to apply non-point functions in a point context.
template <typename H>
inline auto evaluate(H h) {
    return [h]([[maybe_unused]] const Point &p) {
        return h();
    };
}

// It applies each surface function to a point and then applies 'h'.
template <typename H, typename... Fs>
inline auto evaluate(H h, Fs... fs) {
    return [h, fs...](const Point &p)->Real {
        return h(fs(p)...);
    };
}

// Compose function with no arguments returns identity function.
inline auto compose() {
    return [](auto x) { return x; };
}

// Compose with a single function just returns that function.
template <typename F>
inline auto compose (F f) {
    return f;
}

// Compose multiple functions. It chains them together in order.
template <typename F1, typename... Fs>
inline auto compose(F1 f1, Fs... fs) {
    return [f1, fs...](auto... args) {
        return compose(fs...)(f1(args...));
    };
}

#endif
