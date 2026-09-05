#include "collection_playlist.h"
#include <cassert>
#include <iostream>
#include <set>
int main() {
    NativeSceneDirector director;
    director.setCollectionOnly(true);
    director.selectScene(NativeSceneKind::CrystalVault);
    director.setScenePreferences({}, {"light-speed"});
    CollectionPlaylist playlist;
    std::mt19937 random(42);
    std::set<NativeSceneKind> visited{director.state().currentScene};
    for (int i=0;i<18;++i) {
        auto next=playlist.next(director,random);
        assert(isCollectionScene(next));
        assert(next != NativeSceneKind::LightSpeed);
        assert(visited.insert(next).second);
        director.selectScene(next);
    }
    director.resetForTrack();
    assert(director.sceneHidden(NativeSceneKind::ConstellationField));
    assert(director.visibleSceneCount()==19);
    for (int i=0;i<100;++i) {
        auto previous=director.state().currentScene;
        auto next=playlist.next(director,random);
        assert(next!=previous && !director.sceneHidden(next));
        director.selectScene(next);
    }
    director.setScenePreferences({},{});
    assert(director.visibleSceneCount()==20);
    assert(director.sceneHidden(NativeSceneKind::ConstellationField));
    std::cout << "collection playlist passed\n";
}
