#include "first_run_controls.h"

#include <cassert>
#include <iostream>

int main() {
    FirstRunControls disabled(false);
    disabled.onWindowShown(1000, true, 5.0f, 5.0f);
    assert(!disabled.pending());
    assert(disabled.scheduledAt() == 0);
    assert(!disabled.shouldShow(20000));

    FirstRunControls withArtwork(true);
    withArtwork.onWindowShown(1000, true, 5.0f, 5.0f);
    assert(withArtwork.scheduledAt() == 11500);
    assert(!withArtwork.shouldShow(11499));
    assert(withArtwork.shouldShow(11500));
    withArtwork.onWindowShown(12000, false, 0.0f, 0.0f);
    assert(withArtwork.scheduledAt() == 11500);
    withArtwork.markShown();
    assert(!withArtwork.pending());
    assert(!withArtwork.shouldShow(20000));

    FirstRunControls withoutArtwork(true);
    withoutArtwork.onWindowShown(240, false, 20.0f, 20.0f);
    assert(withoutArtwork.scheduledAt() == 1740);
    assert(withoutArtwork.shouldShow(1740));

    std::cout << "first-run controls passed\n";
}
