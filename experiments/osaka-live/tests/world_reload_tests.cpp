// Reload logic for `omadrop-osaka --preview`: no window, no GPU.
#include "../src/kit/world-reload.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace Journey;
using namespace Journey::Kit;

namespace {
void require(bool ok, const QString& message) {
    if (!ok) throw std::runtime_error(message.toStdString());
}
QByteArray read(const QString& path) {
    QFile f(path);
    require(f.open(QIODevice::ReadOnly), "cannot read " + path);
    return f.readAll();
}
// The way editors save: write a temporary file, then rename it over the original.
void saveAtomically(const QString& path, const QByteArray& bytes) {
    const QString temporary = path + ".tmp";
    QFile f(temporary);
    require(f.open(QIODevice::WriteOnly | QIODevice::Truncate) && f.write(bytes) == bytes.size(), "cannot write " + temporary);
    f.close();
    std::filesystem::rename(temporary.toStdString(), path.toStdString());
}
// A fingerprint of everything the loaded art draws.
QByteArray artFingerprint() {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const auto& element : osakaWorld().art->elements()) {
        hash.addData(element.id.toUtf8());
        if (!element.canvas) continue;
        const auto& v = element.canvas->vertices();
        hash.addData(QByteArrayView(reinterpret_cast<const char*>(v.data()), qsizetype(v.size() * sizeof(Vertex))));
    }
    return hash.result();
}
struct Waiter {
    int count = 0;
    bool ok = false;
    QString message;
    explicit Waiter(WorldReloader& reloader) {
        QObject::connect(&reloader, &WorldReloader::finished, [this](bool result, const QString& text) {
            ++count; ok = result; message = text;
        });
    }
    bool until(int expected, int timeoutMs = 10000) {
        QElapsedTimer clock; clock.start();
        while (count < expected && clock.elapsed() < timeoutMs) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        return count >= expected;
    }
    // Nothing more may arrive.
    bool quiet(int milliseconds) {
        const int before = count;
        QElapsedTimer clock; clock.start();
        while (clock.elapsed() < milliseconds) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        return count == before;
    }
};
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir temp;
        require(temp.isValid(), "no temporary directory");
        const QString folder = temp.filePath("lit");
        require(QDir().mkpath(folder), "cannot create the world folder");
        const QString source = QStringLiteral(LIT_WINDOWS_FOLDER);
        for (const char* file : {"scene.json", "art.svg"})
            require(QFile::copy(QDir(source).filePath(file), QDir(folder).filePath(file)), "cannot copy the example world");
        const QString scene = folder + "/scene.json", art = folder + "/art.svg";
        const QByteArray goodScene = read(scene), goodArt = read(art);

        // Startup path: first load becomes the current world.
        auto first = tryLoadOsakaWorld(folder);
        require(bool(first.world), "example world failed to load: " + first.error);
        queueOsakaWorld(std::move(first.world));
        require(applyQueuedOsakaWorld(), "first world was not applied");
        const QByteArray original = artFingerprint();
        require(!applyQueuedOsakaWorld(), "an empty queue reported a swap");

        WorldReloader reloader("examples/lit-windows", folder);
        Waiter waiter(reloader);
        reloader.start();

        // 1. Change a colour and save atomically; the watcher notices without help.
        QByteArray recolored = goodArt;
        require(recolored.contains("#0b2521"), "building colour not found in the example");
        recolored.replace("#0b2521", "#5a1f2b");
        saveAtomically(art, recolored);
        require(waiter.until(1), "no reload after an atomic save of art.svg");
        require(waiter.ok, "recolored art failed to load: " + waiter.message);
        require(reloader.error().isEmpty() && reloader.flash(), "good reload should clear the error and flash");
        require(applyQueuedOsakaWorld(), "reloaded world was not queued");
        const QByteArray changed = artFingerprint();
        require(changed != original, "the new world's art matches the old one");

        // A second atomic save works too (the watch on the replaced file was re-armed).
        QByteArray recolored2 = goodArt;
        recolored2.replace("#0b2521", "#1f3a5a");
        saveAtomically(art, recolored2);
        require(waiter.until(2), "no reload after a second atomic save");
        require(waiter.ok && applyQueuedOsakaWorld(), "second reload failed: " + waiter.message);
        require(artFingerprint() != changed, "second save did not change the art");

        // Saving identical content is not a reload.
        saveAtomically(art, recolored2);
        require(waiter.quiet(900), "identical content triggered a reload");

        // 2. Invalid scene.json: reload fails with the diagnostic, the old world stays.
        const QByteArray beforeBreak = artFingerprint();
        saveAtomically(scene, "{ \"schema\": ");
        require(waiter.until(3), "no result after breaking scene.json");
        require(!waiter.ok, "a broken scene.json loaded");
        require(waiter.message.contains("scene.json"), "diagnostic does not name scene.json: " + waiter.message);
        require(!reloader.error().isEmpty() && !reloader.flash(), "failure should keep the diagnostic and stop the flash");
        require(!applyQueuedOsakaWorld(), "a failed reload queued a world");
        require(artFingerprint() == beforeBreak, "the previous world did not stay current");

        // Broken art.svg reports art.svg.
        saveAtomically(scene, goodScene);
        require(waiter.until(4) && waiter.ok && applyQueuedOsakaWorld(), "restoring scene.json failed: " + waiter.message);
        saveAtomically(art, "<svg xmlns=\"http://www.w3.org/2000/svg\"><text id=\"t\">hi</text></svg>");
        require(waiter.until(5), "no result after breaking art.svg");
        require(!waiter.ok && waiter.message.contains("art.svg"), "art.svg diagnostic: " + waiter.message);
        require(!applyQueuedOsakaWorld() && artFingerprint() == beforeBreak, "broken art replaced the running world");

        // 3. Fix it: reload succeeds and the error clears.
        saveAtomically(art, recolored2);
        require(waiter.until(6) && waiter.ok, "fixing art.svg did not reload: " + waiter.message);
        require(reloader.error().isEmpty(), "error stayed after a good save");
        require(applyQueuedOsakaWorld() && artFingerprint() == beforeBreak, "fixed world is not the expected art");

        // 4. R reloads even when nothing changed.
        reloader.reloadNow();
        require(waiter.until(7) && waiter.ok && applyQueuedOsakaWorld(), "forced reload failed");
        std::cout << "PASS: atomic-save reload, failed reload keeps the last good world, fix recovers\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
