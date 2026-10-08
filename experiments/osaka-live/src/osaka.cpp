// Osaka Jade is an immutable world-folder description, loaded before rendering.
#include "kit/world-loader.h"
#include <QCoreApplication>
#include <QDir>
#include <atomic>
#include <mutex>
#include <stdexcept>
namespace Journey::Kit {
namespace {
// `world` is read every frame and, in preview mode, replaced; both happen only on
// the render thread, between frames. Other threads hand over a finished world
// through `pending`.
std::unique_ptr<const LoadedOsakaWorld> world;
std::mutex pendingMutex;
std::unique_ptr<const LoadedOsakaWorld> pending;
std::atomic_bool hasPending{false};
}
QString osakaWorldsRoot() {
    if (qEnvironmentVariableIsSet("OMADROP_WORLDS")) return qEnvironmentVariable("OMADROP_WORLDS");
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../worlds");
}
QString osakaWorldFolder(const QString& name) {
    const QString clean=QDir::cleanPath(name);
    if (name.isEmpty() || QDir::isAbsolutePath(name) || name.contains('\\')
        || clean==".." || clean.startsWith("../")) {
        throw std::runtime_error("invalid world name: "+name.toStdString());
    }
    return QDir(osakaWorldsRoot()).filePath(clean);
}
void configureWaveTrain() {
    std::shared_ptr<const WaveTrainParametersV2> parameters;
    const auto& d=world->description();
    for(const auto* stage:{&d.backdrop,&d.coast,&d.distantTown,&d.foreground})
        for(size_t i=0;i<stage->count;++i)if(stage->entries[i].piece==OsakaOp::WaveTrain)
            parameters=std::make_shared<WaveTrainParametersV2>(stage->entries[i].params->waveTrain);
    std::atomic_store(&Schedule::waveTrainParameters,parameters);
}
void initializeOsakaWorldAt(const QString& folder) { world = loadOsakaWorld(folder); configureWaveTrain(); }
void queueOsakaWorld(std::unique_ptr<const LoadedOsakaWorld> next) {
    if (!next) return;
    std::lock_guard<std::mutex> lock(pendingMutex);
    pending = std::move(next);
    hasPending.store(true, std::memory_order_release);
}
bool applyQueuedOsakaWorld() {
    if (!hasPending.load(std::memory_order_acquire)) return false;
    std::unique_ptr<const LoadedOsakaWorld> retired;
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        if (!pending) return false;
        retired = std::move(world);
        world = std::move(pending);
        hasPending.store(false, std::memory_order_release);
    }
    configureWaveTrain();
    return true; // the previous world is destroyed here, after the swap
}
void initializeOsakaWorld(const QString& name) { initializeOsakaWorldAt(osakaWorldFolder(name)); }
const OsakaParametersV1& osakaParameters() { return osakaWorld().parameters; }
const OsakaWorldDescription& osakaWorld() {
    if (!world) throw std::logic_error("Osaka world was not initialized at startup");
    return world->description();
}
}
