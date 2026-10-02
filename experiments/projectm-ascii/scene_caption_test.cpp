#include "scene_caption.h"
#include "status_overlay.h"

#include <cassert>
#include <iostream>

int main() {
    SceneCaption caption;
    assert(!caption.update(0, false, true, false));
    assert(!caption.update(0, false, false, true));
    assert(caption.update(0, false, true, true));
    assert(!caption.update(0, false, true, true));
    assert(!caption.update(1, true, true, true));
    assert(!caption.update(1, false, false, true));
    assert(caption.update(1, false, true, true));
    // A completed transition can return to the same preset (single-scene rotation).
    assert(!caption.update(1, true, true, true));
    assert(caption.update(1, false, true, true));
    assert(SceneCaption::opacity(0) == 0);
    assert(SceneCaption::opacity(200) == 0.5f);
    assert(SceneCaption::opacity(400) == 1);
    assert(SceneCaption::opacity(3399) == 1);
    assert(SceneCaption::opacity(3800) == 0.5f);
    assert(SceneCaption::opacity(4200) == 0);
    const auto bitmap = rasterizeStatusLabel("Cloud Cubes\nflexi - bouncing icecubes", true);
    assert(bitmap.width > 0 && bitmap.height > 0);
    bool dimCredit = false;
    for (std::size_t i = 0; i < bitmap.rgba.size(); i += 4) {
        if (bitmap.rgba[i] == 158 && bitmap.rgba[i + 3] == 255) dimCredit = true;
    }
    assert(dimCredit);
    assert(bitmap.rgba[3] == 0);
    std::cout << "scene caption passed\n";
}
