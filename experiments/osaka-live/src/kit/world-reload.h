#pragma once
#include "world-loader.h"
#include <QByteArray>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
class QThread;
namespace Journey::Kit {
struct WorldLoadResult {
    std::unique_ptr<const LoadedOsakaWorld> world;
    QString error; // plain text; empty when world is set
};
// Loads a world folder without touching the current world. Never throws; safe on any thread.
WorldLoadResult tryLoadOsakaWorld(const QString& folder);

// Watches scene.json and art.svg in a world folder and, after each save settles,
// loads the folder off the GUI thread. A good world is queued for the render thread
// (see queueOsakaWorld); a bad one only records the diagnostic, so the last good world
// keeps running. Lives on the GUI thread.
class WorldReloader : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString worldName READ worldName CONSTANT)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(bool flash READ flash NOTIFY changed)
public:
    WorldReloader(QString name, QString folder, QObject* parent = nullptr);
    ~WorldReloader() override;
    void start(); // arm the watches; the world already loaded at startup counts as the first good one
    QString worldName() const { return name_; }
    QString error() const { return error_; }
    bool flash() const { return flash_; }
    bool busy() const { return worker_ != nullptr; }
    // Reload even when the files look unchanged (the R key).
    Q_INVOKABLE void reloadNow() { request(true); }
signals:
    void changed();
    void finished(bool ok, const QString& message);
private:
    void request(bool force);
    void startLoad();
    void finishLoad(WorldLoadResult result);
    void rearm();
    QByteArray signature() const;
    QString name_, folder_;
    QFileSystemWatcher watcher_;
    QTimer debounce_, flashTimer_;
    QThread* worker_ = nullptr;
    QByteArray loadedSignature_;
    QString error_;
    bool flash_ = false, again_ = false, force_ = false;
};
}
