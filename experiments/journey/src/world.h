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
constexpr double kanagawaDelay = 8.0; // additional travel before the dawn chapter
constexpr double crossingEnd = 40.0 + kanagawaDelay;
constexpr double kanagawaEnd = 87.0; // travelling wave, wreck, washed bearer and mountain-road ending
}

struct Ctx {
    Gpu& gpu;
    double t;
    Audio a;
    const Score* score;
    int seed;
    std::array<Canvas, 6>* canvases;
    int next = 0;
    double cameraX = 0; // scene-local world coordinates; crossing defaults to zero
    double waterContact = -1; // Kanagawa pressure begins at one measured impact
    Canvas& canvas() {
        Canvas& c = (*canvases)[next++ % canvases->size()];
        c.reset(gpu.pixelScale());
        c.translate(-cameraX, 0);
        return c;
    }
    double band(int i) const { return a.bands[std::size_t(std::clamp(i, 0, 5))]; }
    // How far a band stands above its own recent level: 0 at or below the
    // running mean, about 1 at twice the mean. Silence gives 0.
    double lift(int i) const {
        if (!score) return 0;
        const double m = score->mean(std::clamp(i, 0, 5), t);
        return std::clamp((band(i) - m) / (m + 0.035), 0.0, 1.6);
    }
    // Recent bass hit / onset as a decaying envelope.
    double kick(double decay = 6) const { return score ? Score::envelope(score->bassHits, t, decay) : 0; }
    double hit(double decay = 8) const { return score ? Score::envelope(score->onsets, t, decay) : 0; }
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
    double reflection = 1; // street/harbour reflected light, unchanged in Osaka
    double harbourFeather = 0; // soften the retiring water strip only in the crossing
};

void drawOsaka(Ctx& c, const OsakaState& s);
void drawCrossing(Ctx& c);
void drawKanagawa(Ctx& c);
void drawDirectionStudy(Ctx& c);
void drawTitle(Canvas& canvas, int which, double alpha, Col accent, Col text);
// Sign lettering, one glyph per index: 0..2 居酒屋, 3..6 らーめん. (x, y) is the
// glyph origin on the baseline; size is the em in design pixels.
void drawSignGlyph(Canvas& canvas, int index, double x, double y, double size, Col col, double alpha = 1.0);
// Shared persistent elements.
struct Surge { double t = -1; double strength = 0; bool fallback = false; };
Surge surgeIn(const Score* score, double from, double to, double fallbackAt);

class World {
public:
    explicit World(int seed) : seed_(seed) {}
    bool init(QString& error) { return gpu_.init(error); }
    void setScore(const Score* score) { score_ = score; }
    void setDirectionStudy(bool enabled) { directionStudy_ = enabled; }
    void render(int width, int height, double t, const Audio& audio);
    Gpu& gpu() { return gpu_; }
private:
    Gpu gpu_;
    const Score* score_ = nullptr;
    int seed_;
    bool directionStudy_ = false;
    std::array<Canvas, 6> canvases_;
};
}
