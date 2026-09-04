#include "cover_presentation.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {
bool closeTo(float actual, float expected) {
    return std::abs(actual - expected) < 0.001f;
}
}

int main() {
    CoverPresentation presentation(5.0f, 5.0f);
    CoverPresentationFrame frame = presentation.frame(1000);
    assert(frame.complete && closeTo(frame.coverMix, 0.0f));

    presentation.show(1000);
    assert(presentation.hasArtwork());
    frame = presentation.frame(5999);
    assert(!frame.complete && closeTo(frame.coverMix, 1.0f));
    frame = presentation.frame(8500);
    assert(!frame.complete && closeTo(frame.coverMix, 0.5f));
    frame = presentation.frame(11000);
    assert(frame.complete && closeTo(frame.coverMix, 0.0f));
    assert(frame.paletteInfluence > 0.12f);

    presentation.restart(20000);
    frame = presentation.frame(20000);
    assert(!frame.complete && closeTo(frame.coverMix, 1.0f));

    presentation.clear();
    assert(!presentation.hasArtwork());
    frame = presentation.frame(20001);
    assert(frame.complete && closeTo(frame.coverMix, 0.0f));

    presentation.show(30000);
    frame = presentation.frame(30000);
    assert(presentation.hasArtwork() && closeTo(frame.coverMix, 1.0f));
    std::cout << "cover presentation passed\n";
    return 0;
}
