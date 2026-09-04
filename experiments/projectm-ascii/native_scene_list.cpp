#include "native_scene_registry.h"

#include <iostream>
#include <string_view>
#include <unordered_set>

int main() {
    std::unordered_set<std::string_view> slugs;
    std::unordered_set<std::string_view> names;
    for (std::size_t index = 0; index < nativeSceneRegistry.size(); ++index) {
        const NativeSceneDefinition& scene = nativeSceneRegistry[index];
        if (static_cast<std::size_t>(scene.kind) != index
            || scene.slug.empty() || scene.name.empty() || scene.shader.empty()
            || !slugs.insert(scene.slug).second
            || !names.insert(scene.name).second) {
            std::cerr << "invalid native scene registry entry at " << index << "\n";
            return 1;
        }
        std::cout << scene.slug << '\t' << scene.name << '\n';
    }
    return 0;
}
