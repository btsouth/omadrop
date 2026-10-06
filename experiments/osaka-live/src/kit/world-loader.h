#pragma once
#include "composition.h"
#include <QString>
#include <array>
#include <memory>
#include <vector>
namespace Journey::Kit {
// Owns every slot and exposes only a const view. Addresses remain stable.
class LoadedOsakaWorld {
public:
    const OsakaWorldDescription& description() const { return world_; }
    const std::vector<QString>& notes() const { return notes_; }
    LoadedOsakaWorld(const LoadedOsakaWorld&) = delete;
    LoadedOsakaWorld& operator=(const LoadedOsakaWorld&) = delete;
private:
    LoadedOsakaWorld() = default;
    std::array<std::vector<OsakaRenderSlot>, 4> entries_;
    OsakaWorldDescription world_{};
    std::vector<QString> notes_;
    friend std::unique_ptr<const LoadedOsakaWorld> loadOsakaWorld(const QString&);
};
QString osakaWorldsRoot();
void initializeOsakaWorld(const QString& world = QStringLiteral("osaka-jade"));
// The folder a world name resolves to under the worlds root. Throws for invalid names.
QString osakaWorldFolder(const QString& name);
// Loads the world in a folder as the current world. Throws like loadOsakaWorld.
void initializeOsakaWorldAt(const QString& folder);
// Preview reload. Any thread hands a fully loaded world to queueOsakaWorld; the
// render thread calls applyQueuedOsakaWorld between frames and, when it returns
// true, must drop every cache built from the old world. The newest queued world
// wins. Nothing else may change the current world while frames are drawing.
void queueOsakaWorld(std::unique_ptr<const LoadedOsakaWorld> next);
bool applyQueuedOsakaWorld();
// Throws std::runtime_error with file, JSON path and expectation.
std::unique_ptr<const LoadedOsakaWorld> loadOsakaWorld(const QString& folder);
}
