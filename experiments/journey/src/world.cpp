#include "world.h"

namespace Journey {
Surge surgeIn(const Score* score, double from, double to, double fallbackAt) {
    Surge s;
    if (score) {
        if (const Event* e = Score::first(score->surges, from, to)) {
            s.t = e->t; s.strength = e->strength; s.fallback = false;
            return s;
        }
    }
    s.t = fallbackAt; s.strength = 0.6; s.fallback = true;
    return s;
}

void World::render(int width, int height, double t, const Audio& audio) {
    gpu_.begin(width, height);
    Ctx c{gpu_, t, audio, score_, seed_, &canvases_};
    if (t < T::osakaEnd) {
        OsakaState s;
        s.cam = -20 + 50 * easeInOut(t / T::osakaEnd);
        drawOsaka(c, s);
    } else if (t < T::crossingEnd) {
        drawCrossing(c);
    } else {
        drawKanagawa(c);
    }
}
}
