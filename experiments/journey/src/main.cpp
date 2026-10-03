#include "backend.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageWriter>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSaveFile>
#include <QTextStream>
#include <cmath>
#include <utility>

namespace {
int fail(const QString& error) {
    QTextStream(stderr) << "omadrop-journey: " << error << '\n';
    return 1;
}

struct Options {
    QString audio, exportDir;
    double offset = 0, duration = 72, at = 0;
    int fps = 30, width = 1920, height = 1080;
    bool paused = false, overwrite = false;
};

bool number(const QCommandLineParser& parser, const QString& name,
            double minimum, double maximum, double& result, QString& error) {
    bool ok = false;
    result = parser.value(name).toDouble(&ok);
    if (!ok || !std::isfinite(result) || result < minimum || result > maximum) {
        error = QStringLiteral("--%1 must be a finite number in [%2, %3].")
            .arg(name).arg(minimum).arg(maximum);
        return false;
    }
    return true;
}

bool integer(const QCommandLineParser& parser, const QString& name,
             int minimum, int maximum, int& result, QString& error) {
    bool ok = false;
    result = parser.value(name).toInt(&ok);
    if (!ok || result < minimum || result > maximum) {
        error = QStringLiteral("--%1 must be an integer in [%2, %3].")
            .arg(name).arg(minimum).arg(maximum);
        return false;
    }
    return true;
}

QString framePath(const QDir& directory, int frame) {
    return directory.filePath(QStringLiteral("frame-%1.jpg").arg(frame, 6, 10, QLatin1Char('0')));
}

int exportFrames(const Options& options, const Journey::AudioTimeline& audio) {
    QDir directory(options.exportDir);
    const int count = int(std::ceil(options.duration * options.fps - 1e-9));
    // Reject the entire request before producing frames if any target exists.
    if (!options.overwrite) {
        for (int frame = 0; frame < count; ++frame) {
            const QFileInfo target(framePath(directory, frame));
            if (target.exists() || target.isSymLink())
                return fail(QStringLiteral("Export target exists: %1. Use --overwrite to replace it.").arg(target.filePath()));
        }
    }
    if (!QImageWriter::supportedImageFormats().contains("jpeg"))
        return fail(QStringLiteral("Qt JPEG image writer is unavailable."));
    if (!directory.exists() && !QDir().mkpath(directory.absolutePath()))
        return fail(QStringLiteral("Cannot create export directory: %1").arg(directory.absolutePath()));

    QImage image(options.width, options.height, QImage::Format_RGB32);
    if (image.isNull()) return fail(QStringLiteral("Cannot allocate export image."));
    for (int frame = 0; frame < count; ++frame) {
        const double seconds = double(frame) / options.fps;
        image.fill(Qt::black);
        {
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            // The renderer must remain font-free for this QCoreApplication path.
            Journey::paint(painter, image.size(), seconds, audio.at(seconds));
        }
        const QString target = framePath(directory, frame);
        if (options.overwrite) {
            QSaveFile file(target);
            if (!file.open(QIODevice::WriteOnly)) return fail(file.errorString());
            QImageWriter writer(&file, "jpeg");
            writer.setQuality(94);
            if (!writer.write(image)) return fail(QStringLiteral("%1: %2").arg(target, writer.errorString()));
            if (!file.commit()) return fail(QStringLiteral("%1: %2").arg(target, file.errorString()));
        } else {
            // NewOnly closes the race between preflight and write without
            // allowing another process's output to be replaced.
            QFile file(target);
            if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly))
                return fail(QStringLiteral("%1: %2").arg(target, file.errorString()));
            QImageWriter writer(&file, "jpeg");
            writer.setQuality(94);
            const bool written = writer.write(image);
            const QString writeError = writer.errorString();
            const bool flushed = written && file.flush();
            const QString flushError = file.errorString();
            file.close();
            if (!written || !flushed) {
                file.remove();
                return fail(QStringLiteral("%1: %2").arg(target, written ? flushError : writeError));
            }
        }
        if (frame % options.fps == 0)
            QTextStream(stderr) << "Export " << frame << '/' << count << '\n';
    }
    QTextStream(stdout) << "Exported " << count << " JPEG frames to " << directory.absolutePath() << '\n';
    return 0;
}
}

int main(int argc, char* argv[]) {
    QCoreApplication::setApplicationName(QStringLiteral("omadrop-journey"));
    QCoreApplication::setApplicationVersion(QStringLiteral("study"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Original Osaka Jade to Kanagawa motion study"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
        {QStringLiteral("audio"), QStringLiteral("Recorded stereo 44100 Hz float32 little-endian PCM; omitted = silence."), QStringLiteral("path")},
        {QStringLiteral("audio-offset"), QStringLiteral("Fixture seconds corresponding to movie time zero."), QStringLiteral("seconds"), QStringLiteral("0")},
        {QStringLiteral("duration"), QStringLiteral("Movie length, at most 600 seconds."), QStringLiteral("seconds"), QStringLiteral("72")},
        {QStringLiteral("at"), QStringLiteral("Initial native movie time."), QStringLiteral("seconds"), QStringLiteral("0")},
        {QStringLiteral("paused"), QStringLiteral("Start native playback paused.")},
        {QStringLiteral("export-dir"), QStringLiteral("Headless JPEG export destination."), QStringLiteral("path")},
        {QStringLiteral("fps"), QStringLiteral("Export frames per second, 1..120."), QStringLiteral("fps"), QStringLiteral("30")},
        {QStringLiteral("width"), QStringLiteral("Export width / initial native width, 1..8192."), QStringLiteral("pixels"), QStringLiteral("1920")},
        {QStringLiteral("height"), QStringLiteral("Export height / initial native height, 1..8192."), QStringLiteral("pixels"), QStringLiteral("1080")},
        {QStringLiteral("overwrite"), QStringLiteral("Allow replacement of existing export frames.")}
    });
    QStringList arguments;
    for (int i = 0; i < argc; ++i) arguments << QString::fromLocal8Bit(argv[i]);
    if (!parser.parse(arguments)) return fail(parser.errorText());
    if (parser.isSet(QStringLiteral("help")) || parser.isSet(QStringLiteral("version"))) {
        QCoreApplication app(argc, argv);
        QTextStream(stdout) << (parser.isSet(QStringLiteral("help")) ? parser.helpText() : QStringLiteral("omadrop-journey study\n"));
        return 0;
    }
    if (!parser.positionalArguments().isEmpty()) return fail(QStringLiteral("Unexpected positional argument; use --help."));
    Options options;
    QString error;
    if (!number(parser, QStringLiteral("duration"), 1.0 / 60, 600, options.duration, error)
        || !number(parser, QStringLiteral("audio-offset"), 0, 21600, options.offset, error)
        || !number(parser, QStringLiteral("at"), 0, options.duration, options.at, error)
        || !integer(parser, QStringLiteral("fps"), 1, 120, options.fps, error)
        || !integer(parser, QStringLiteral("width"), 1, 8192, options.width, error)
        || !integer(parser, QStringLiteral("height"), 1, 8192, options.height, error)) return fail(error);
    if (qint64(options.width) * options.height > 33554432)
        return fail(QStringLiteral("Image dimensions exceed the 33554432-pixel limit."));
    if (options.offset + options.duration > 21600)
        return fail(QStringLiteral("Audio offset + duration exceeds 21600 seconds."));
    options.audio = parser.value(QStringLiteral("audio"));
    options.exportDir = parser.value(QStringLiteral("export-dir"));
    options.paused = parser.isSet(QStringLiteral("paused"));
    options.overwrite = parser.isSet(QStringLiteral("overwrite"));
    const bool exporting = parser.isSet(QStringLiteral("export-dir"));
    if (parser.isSet(QStringLiteral("audio")) && options.audio.isEmpty()) return fail(QStringLiteral("--audio path is empty."));
    if (options.audio.isEmpty() && options.offset != 0) return fail(QStringLiteral("--audio-offset requires --audio."));
    if (exporting && options.exportDir.isEmpty()) return fail(QStringLiteral("--export-dir path is empty."));
    if (exporting && (parser.isSet(QStringLiteral("at")) || options.paused))
        return fail(QStringLiteral("--at and --paused apply to native playback; export always starts at movie time zero."));
    if (!exporting && (options.overwrite || parser.isSet(QStringLiteral("fps"))))
        return fail(QStringLiteral("--overwrite and --fps require --export-dir."));

    // Select the application class before constructing any QGuiApplication.
    // In particular, export never initializes a platform plugin or a display.
    if (exporting) {
        QCoreApplication app(argc, argv);
        Journey::AudioTimeline audio;
        if (!options.audio.isEmpty() && !audio.load(options.audio, options.offset, options.duration, error))
            return fail(error);
        return exportFrames(options, audio);
    }
    // Input failures are useful even if no GUI platform is available.
    Journey::AudioTimeline audio;
    if (!options.audio.isEmpty() && !audio.load(options.audio, options.offset, options.duration, error)) return fail(error);
    QGuiApplication app(argc, argv);
    app.setDesktopFileName(QStringLiteral("omadrop-journey-study"));
    qmlRegisterType<JourneyItem>("Journey", 1, 0, "JourneyItem");
    qmlRegisterUncreatableType<JourneyBackend>("Journey", 1, 0, "JourneyBackend", "Provided by the native harness");
    JourneyBackend backend(std::move(audio), options.duration, options.at, options.paused);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("journeyBackend"), &backend);
    engine.rootContext()->setContextProperty(QStringLiteral("initialWidth"), options.width);
    engine.rootContext()->setContextProperty(QStringLiteral("initialHeight"), options.height);
    engine.load(QUrl(QStringLiteral("qrc:/journey/Main.qml")));
    if (engine.rootObjects().isEmpty()) return fail(QStringLiteral("Cannot load the native Journey window."));
    return app.exec();
}
