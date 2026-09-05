#pragma once
#include "native_scene_state.h"
#include <algorithm>
#include <random>
#include <vector>

// A shuffled bag visits every available scene before refilling. Current scenes
// are excluded even across bag boundaries and after a manual selection.
class CollectionPlaylist {
public:
    NativeSceneKind next(const NativeSceneDirector& director, std::mt19937& random) {
        const auto& state = director.state();
        const auto current = state.transitioning ? state.incomingScene : state.currentScene;
        auto eligible = [&](NativeSceneKind scene) {
            return isCollectionScene(scene) && scene != current && !director.sceneHidden(scene);
        };
        std::erase_if(bag_, [&](auto scene) { return !eligible(scene); });
        if (bag_.empty()) {
            for (const auto& definition : nativeSceneRegistry)
                if (eligible(definition.kind)) bag_.push_back(definition.kind);
            std::shuffle(bag_.begin(), bag_.end(), random);
        }
        if (bag_.empty()) return current;
        auto selected = bag_.back();
        bag_.pop_back();
        return selected;
    }
private:
    std::vector<NativeSceneKind> bag_;
};
