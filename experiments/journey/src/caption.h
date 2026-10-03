#pragma once

class QPainter;

namespace Journey {
// Call after the artwork, with the painter in normalized 1600 x 900 space.
// Uses only cached vector outlines; safe with QCoreApplication and QImage.
void paintThemeCaption(QPainter& painter, double seconds);
}
