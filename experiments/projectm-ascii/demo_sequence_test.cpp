#include "native_scene_registry.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    assert(argc == 2);
    std::ifstream input(argv[1]);
    assert(input);
    std::vector<NativeSceneKind> scenes;
    std::string slug;
    while (std::getline(input, slug)) {
        if (slug.empty()) continue;
        NativeSceneKind scene;
        assert(nativeSceneFromName(slug, scene));
        scenes.push_back(scene);
    }

    assert(scenes.size() == 5);
    assert(scenes.front() == NativeSceneKind::InkCurrent);
    assert(scenes.back() == NativeSceneKind::LivingMosaic);

    std::set<NativeVisualFamily> families;
    std::set<NativeTransitionStyle> transitions;
    for (std::size_t index = 0; index < scenes.size(); ++index) {
        families.insert(nativeSceneDefinition(scenes[index]).visualFamily);
        if (index > 0) {
            transitions.insert(nativeTransitionStyle(
                scenes[index - 1], scenes[index]));
        }
    }
    assert(families.size() == scenes.size());
    assert(transitions.size() >= 3);

    std::cout << "demo sequence passed: 5 families, "
              << transitions.size() << " transition grammars\n";
}
