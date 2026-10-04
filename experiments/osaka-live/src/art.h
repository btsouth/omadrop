#pragma once

// Small value types and easing helpers shared by the worlds and rigs.
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Journey {
constexpr double Pi = 3.14159265358979323846;
constexpr double Tau = 2 * Pi;

struct Col {
    float r = 0, g = 0, b = 0;
    constexpr Col() = default;
    constexpr Col(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}
    Col operator*(float k) const { return {r * k, g * k, b * k}; }
    Col operator+(Col o) const { return {r + o.r, g + o.g, b + o.b}; }
};

constexpr Col hex(uint32_t v) {
    return {float((v >> 16) & 255) / 255.f, float((v >> 8) & 255) / 255.f, float(v & 255) / 255.f};
}
inline Col mix(Col a, Col b, double t) {
    const float k = float(t);
    return {a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k, a.b + (b.b - a.b) * k};
}

struct V2 {
    double x = 0, y = 0;
    V2() = default;
    V2(double x_, double y_) : x(x_), y(y_) {}
    V2 operator+(V2 o) const { return {x + o.x, y + o.y}; }
    V2 operator-(V2 o) const { return {x - o.x, y - o.y}; }
    V2 operator*(double k) const { return {x * k, y * k}; }
    double len() const { return std::hypot(x, y); }
};
inline V2 lerp(V2 a, V2 b, double t) { return a + (b - a) * t; }
inline double lerp(double a, double b, double t) { return a + (b - a) * t; }

inline double clamp01(double v) { return std::clamp(v, 0.0, 1.0); }
inline double wrap(double value,double period) {
    const double result=std::fmod(value,period);
    return result<0 ? result+period : result;
}
inline double sstep(double a, double b, double x) {
    const double t = clamp01((x - a) / (b - a));
    return t * t * (3 - 2 * t);
}
// Window: rises over [a, a+in], holds, falls over [b-out, b].
inline double window(double t, double a, double in, double b, double out) {
    return sstep(a, a + in, t) * (1 - sstep(b - out, b, t));
}
inline double easeInOut(double t) {
    t = clamp01(t);
    return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
}
inline double easeOut(double t) { t = clamp01(t); return 1 - std::pow(1 - t, 3); }
inline double easeIn(double t) { t = clamp01(t); return t * t * t; }
// Anticipate, overshoot and settle: dips below 0 early, peaks above 1, rests at 1.
inline double backOut(double t, double s = 1.6) {
    t = clamp01(t) - 1;
    return 1 + t * t * ((s + 1) * t + s);
}
inline double anticipate(double t, double s = 1.4) {
    t = clamp01(t);
    return t * t * ((s + 1) * t - s);
}
// Damped spring response to a step at time 0 (0 before, settles at 1).
inline double springStep(double t, double freq = 2.2, double damp = 4.0) {
    if (t <= 0) return 0;
    return 1 - std::exp(-damp * t) * std::cos(Tau * freq * t);
}
// Decaying oscillation kicked at time 0, for swings and wobbles.
inline double ring(double t, double freq, double damp) {
    if (t <= 0) return 0;
    return std::exp(-damp * t) * std::sin(Tau * freq * t);
}

// Deterministic hashes for per-object variation.
inline double hash1(double n) {
    const double s = std::sin(n * 12.9898 + 78.233) * 43758.5453;
    return s - std::floor(s);
}
inline double hash2(double a, double b) { return hash1(a * 37.17 + b * 101.3); }

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed) : s(seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) {}
    uint64_t next() {
        s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
        return s * 0x2545F4914F6CDD1Dull;
    }
    double uni() { return double(next() >> 11) * (1.0 / 9007199254740992.0); }
    double range(double a, double b) { return a + (b - a) * uni(); }
    double normal() {
        const double u = std::max(1e-12, uni()), v = uni();
        return std::sqrt(-2 * std::log(u)) * std::cos(Tau * v);
    }
};

// Smooth 1D value noise in [-1, 1] for organic drift.
inline double noise1(double x, double seed = 0) {
    const double i = std::floor(x), f = x - i;
    const double a = hash2(i, seed) * 2 - 1, b = hash2(i + 1, seed) * 2 - 1;
    const double u = f * f * (3 - 2 * f);
    return a + (b - a) * u;
}
inline double fbm1(double x, double seed = 0) {
    return 0.6 * noise1(x, seed) + 0.3 * noise1(x * 2.1, seed + 7) + 0.1 * noise1(x * 4.3, seed + 13);
}
}
