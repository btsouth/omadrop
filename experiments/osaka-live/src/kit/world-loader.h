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
    LoadedOsakaWorld(const LoadedOsakaWorld&) = delete;
    LoadedOsakaWorld& operator=(const LoadedOsakaWorld&) = delete;
private:
    LoadedOsakaWorld() = default;
    std::array<std::vector<OsakaRenderSlot>, 4> entries_;
    OsakaWorldDescription world_{};
    friend std::unique_ptr<const LoadedOsakaWorld> loadOsakaWorld(const QString&);
};
QString osakaWorldsRoot();
void initializeOsakaWorld();
// Throws std::runtime_error with file, JSON path and expectation.
std::unique_ptr<const LoadedOsakaWorld> loadOsakaWorld(const QString& folder);
}
