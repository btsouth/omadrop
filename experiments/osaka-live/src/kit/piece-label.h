#pragma once

#include <QString>
#include <cstddef>
#include <string>

namespace Journey::Kit {
struct SvgElement;

// A music-bound SVG layer other than a window: lantern, lamp, neon, glow or wire.
// The label grammar is `piece(.token)*`, lowercase, with exactly one band0..band5.
struct OsakaPieceNodeV1 {
    std::string id;
    std::string piece;
    std::string profile;
    int band = -1;
    bool kick = false;
    bool onset = false;
    bool always = false;
    bool sway = false;
    bool flicker = false;
    bool pulse = false;
    std::size_t element = 0;   // index of the SVG element in the art, set by the loader
};

// True for labels naming one of these pieces ("lantern", "lamp.band1.kick"...).
// Any other label stays plain art.
bool isPieceLabelCandidate(const QString& label);
// Lanterns, neon signs and wires are drawn by the piece instead of as plain art.
bool pieceReplacesArt(const QString& label);
// Throws std::runtime_error: "<file>: element id '<id>' label '<label>': <reason>".
OsakaPieceNodeV1 parsePieceLabel(const QString& svgFile, const QString& elementId,
                                 const QString& label);
// Checks the labelled element can carry the piece. Throws like parsePieceLabel.
void checkPieceElement(const QString& svgFile, const SvgElement& element,
                       const OsakaPieceNodeV1& node);
}
