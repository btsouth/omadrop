#pragma once

// The Journey film: Osaka Jade, the crossing, Kanagawa. Movie time drives an
// authored timeline; measured audio modulates objects within it and fills
// the surge windows. Everything is a function of (time, audio, score, seed).
#include "gpu.h"
#include "score.h"
#include <QString>
#include <array>
#include <memory>

namespace Journey {
namespace T {
constexpr double osakaEnd = 28.0;      // glide begins
constexpr double crossingEnd = 40.0;   // Kanagawa title, full palette
constexpr double kanagawaEnd = 74.0;
}

struct Ctx {
    Gpu& gpu;
    double t;
    Audio a;
    const Score* score;
    int seed;
    std::array<Canvas, 6>* canvases;
    int next = 0;
    Canvas& canvas() {
        Canvas& c = (*canvases)[next++ % canvases->size()];
        c.reset(gpu.pixelScale());
        return c;
    }
    double band(int i) const { return a.bands[std::size_t(std::clamp(i, 0, 5))]; }
    // Deterministic per-run jitter in [-1, 1].
    double jit(double k) const { return hash2(k, seed * 7.13 + 0.5) * 2 - 1; }
};

// State shared by the Osaka stage and the crossing that carries it away.
struct OsakaState {
    double cam = 0;       // camera x in mock coordinates
    double drift = 0;     // small authored camera drift
    double land = 1;      // town, ridges and city visibility under fog
    double fog = 0, fogTop = 600;
    double moonDx = 0, moonDy = 0, moonWarm = 0;
    double wisteria = 1;
    bool chapter = true;  // false during the crossing (no new street events)
    double outAlpha = 1;  // aerial wires from the last pole to the sea
    double harbour = 1;   // dark harbour water beyond the quay
};

void drawOsaka(Ctx& c, const OsakaState& s);
void drawCrossing(Ctx& c);
void drawKanagawa(Ctx& c);
void drawTitle(Canvas& canvas, int which, double alpha, Col accent, Col text);
// Shared persistent elements.
struct Surge { double t = -1; double strength = 0; bool fallback = false; };
Surge surgeIn(const Score* score, double from, double to, double fallbackAt);

class World {
public:
    explicit World(int seed) : seed_(seed) {}
    bool init(QString& error) { return gpu_.init(error); }
    void setScore(const Score* score) { score_ = score; }
    void render(int width, int height, double t, const Audio& audio);
    Gpu& gpu() { return gpu_; }
private:
    Gpu gpu_;
    const Score* score_ = nullptr;
    int seed_;
    std::array<Canvas, 6> canvases_;
};
}
