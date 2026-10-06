#include "world-reload.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QThread>
#include <exception>
namespace Journey::Kit {
namespace {
constexpr int DebounceMs = 250, FlashMs = 2500;
const char* const watchedFiles[] = {"scene.json", "art.svg"};
}
WorldLoadResult tryLoadOsakaWorld(const QString& folder) {
    WorldLoadResult result;
    try { result.world = loadOsakaWorld(folder); }
    catch (const std::exception& e) { result.error = QString::fromUtf8(e.what()); }
    catch (...) { result.error = "unknown error while loading the world"; }
    if (!result.world && result.error.isEmpty()) result.error = "the world did not load";
    return result;
}
WorldReloader::WorldReloader(QString name, QString folder, QObject* parent)
    : QObject(parent), name_(std::move(name)), folder_(std::move(folder)) {
    debounce_.setSingleShot(true);
    debounce_.setInterval(DebounceMs);
    flashTimer_.setSingleShot(true);
    flashTimer_.setInterval(FlashMs);
    connect(&debounce_, &QTimer::timeout, this, [this] { request(false); });
    connect(&flashTimer_, &QTimer::timeout, this, [this] { flash_ = false; emit changed(); });
    auto settle = [this] { rearm(); debounce_.start(); };
    connect(&watcher_, &QFileSystemWatcher::fileChanged, this, settle);
    connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, settle);
}
WorldReloader::~WorldReloader() {
    if (worker_) { worker_->wait(); delete worker_; }
}
void WorldReloader::start() {
    loadedSignature_ = signature();
    rearm();
}
// Editors save by writing a temporary file and renaming it over the original. The
// watch on the replaced file then dies, so watch the folder too and re-add the files.
void WorldReloader::rearm() {
    const QDir dir(folder_);
    if (!watcher_.directories().contains(folder_)) watcher_.addPath(folder_);
    for (const char* file : watchedFiles) {
        const QString path = dir.filePath(file);
        if (QFile::exists(path) && !watcher_.files().contains(path)) watcher_.addPath(path);
    }
}
QByteArray WorldReloader::signature() const {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const char* file : watchedFiles) {
        QFile f(QDir(folder_).filePath(file));
        hash.addData(f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray("missing"));
        hash.addData(QByteArray(1, '\0'));
    }
    return hash.result();
}
void WorldReloader::request(bool force) {
    force_ = force_ || force;
    if (worker_) { again_ = true; return; }
    startLoad();
}
void WorldReloader::startLoad() {
    const QByteArray current = signature();
    if (!force_ && current == loadedSignature_) return; // a temporary file came and went
    force_ = false;
    loadedSignature_ = current;
    again_ = false;
    worker_ = QThread::create([this, folder = folder_] {
        auto result = std::make_shared<WorldLoadResult>(tryLoadOsakaWorld(folder));
        QMetaObject::invokeMethod(this, [this, result] { finishLoad(std::move(*result)); }, Qt::QueuedConnection);
    });
    worker_->start();
}
void WorldReloader::finishLoad(WorldLoadResult result) {
    worker_->wait();
    delete worker_;
    worker_ = nullptr;
    const bool ok = bool(result.world);
    QString message;
    if (ok) {
        queueOsakaWorld(std::move(result.world));
        error_.clear();
        flash_ = true;
        flashTimer_.start();
        message = "reloaded " + name_;
    } else {
        error_ = result.error;
        flash_ = false;
        flashTimer_.stop();
        message = "reload failed, still showing the last good version of " + name_ + ": " + result.error;
    }
    emit changed();
    emit finished(ok, message);
    if (again_ || force_) request(false);
}
}
