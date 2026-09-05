#pragma once
#include "native_scene_registry.h"
#include <algorithm>

// These scenes implement musical-response.glsl v1 and its causal contract.
inline constexpr std::array musicalScenes{
    NativeSceneKind::ConstellationField,
    NativeSceneKind::PrismGarden,
    NativeSceneKind::InkCurrent,
};
inline bool hasMusicalResponse(NativeSceneKind scene) {
    return std::find(musicalScenes.begin(), musicalScenes.end(), scene) != musicalScenes.end();
}
inline NativeSceneKind adjacentMusicalScene(NativeSceneKind scene, bool previous) {
    auto found = std::find(musicalScenes.begin(), musicalScenes.end(), scene);
    if (found == musicalScenes.end()) return musicalScenes.front();
    const auto index = static_cast<std::size_t>(found - musicalScenes.begin());
    return musicalScenes[(index + (previous ? musicalScenes.size()-1 : 1)) % musicalScenes.size()];
}
