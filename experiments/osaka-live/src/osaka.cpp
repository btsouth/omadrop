// Osaka Jade is an immutable world-folder description, loaded before rendering.
#include "kit/world-loader.h"
#include <QCoreApplication>
#include <QDir>
#include <stdexcept>
namespace Journey::Kit {
namespace { std::unique_ptr<const LoadedOsakaWorld> world; }
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
void initializeOsakaWorldAt(const QString& folder) { world = loadOsakaWorld(folder); }
void initializeOsakaWorld(const QString& name) { initializeOsakaWorldAt(osakaWorldFolder(name)); }
const OsakaParametersV1& osakaParameters() { return osakaWorld().parameters; }
const OsakaWorldDescription& osakaWorld() {
    if (!world) throw std::logic_error("Osaka world was not initialized at startup");
    return world->description();
}
}
