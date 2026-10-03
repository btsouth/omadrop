#pragma once

#include <QPainter>
#include <QSize>
#include <array>

namespace Journey {
struct Audio {
    std::array<double, 6> bands{};
    double bass = 0;
    double accent = 0;
};

// Time follows the film's pacing. Audio changes objects within the composition.
void paint(QPainter& painter, QSize size, double seconds, const Audio& audio);
}
