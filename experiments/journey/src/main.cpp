#include "backend.h"
#include "headless.h"
#include "world.h"
#include "glcore.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageWriter>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSaveFile>
#include <QSurfaceFormat>
#include <QQuickWindow>
#include <QTextStream>
#include <array>
#include <cmath>
#include <cstdio>
#include <utility>

namespace {
int fail(const QString& error) {
    QTextStream(stderr) << "omadrop-journey: " << error << '\n';
    return 1;
}

struct Options {
    QString audio, exportDir, video, stills, outDir, bench;
    double offset = 0, duration = 60, at = 0;
    int fps = 30, width = 1920, height = 1080, seed = 1;
    bool paused = false, overwrite = false, inspect = false;
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

QImage frameImage(Journey::World& world, const Options& o, const Journey::AudioTimeline& audio, double t,
                  std::vector<unsigned char>& rgb) {
    world.render(o.width, o.height, t, audio.at(t));
    world.gpu().readRgb(rgb);
    return QImage(rgb.data(), o.width, o.height, o.width * 3, QImage::Format_RGB888);
}

int inspect(const Options& o) {
    Journey::AudioTimeline audio;
    QString error;
    QFile f(o.audio);
    const double seconds = f.size() / 8.0 / 44100.0 - 0.05;
    if (!audio.load(o.audio, 0, seconds, error)) return fail(error);
    Journey::Score score;
    score.build(audio, seconds, true);
    QTextStream out(stdout);
    out << "sec  energy  bass  surge  accmax accmean  onsets bass mid  bands\n";
    for (int s = 0; s + 1 < int(seconds); ++s) {
        double e = 0, b = 0, sg = 0, am = 0, aa = 0;
        std::array<double, 6> bands{};
        for (int k = 0; k < 20; ++k) {
            const auto a = audio.at(s + k / 20.0);
            double p = 0;
            for (int i = 0; i < 6; ++i) { p += a.bands[i] * a.bands[i]; bands[i] += a.bands[i] / 20; }
            e += std::sqrt(p / 6) / 20; b += a.bass / 20; sg = std::max(sg, a.surge);
            am = std::max(am, a.accent); aa += a.accent / 20;
        }
        out << QString::asprintf("%3d  %.3f  %.3f  %.2f  %.3f %.3f  %2d %2d %2d  ", s, e, b, sg, am, aa,
            Journey::Score::count(score.onsets, s, s + 1), Journey::Score::count(score.bassHits, s, s + 1),
            Journey::Score::count(score.midPeaks, s, s + 1));
        for (double v : bands) out << QString::asprintf("%.2f ", v);
        out << '\n';
    }
    for (const auto& e : score.surges) out << QString::asprintf("surge at %.2f strength %.2f\n", e.t, e.strength);
    return 0;
}

int exportFrames(const Options& options, const Journey::AudioTimeline& audio, const Journey::Score& score) {
    Journey::HeadlessContext context;
    QString error;
    if (!context.create(error)) return fail(error);
    Journey::World world(options.seed);
    if (!world.init(error)) return fail(error);
    world.setScore(&score);
    QTextStream(stderr) << "GPU: " << context.renderer() << '\n';
    std::vector<unsigned char> rgb;

    if (!options.bench.isEmpty()) {
        for (const QString& part : options.bench.split(',', Qt::SkipEmptyParts)) {
            const double t0 = part.toDouble();
            world.render(options.width, options.height, t0, audio.at(t0));
            glFinish();
            QElapsedTimer timer;
            timer.start();
            const int n = 60;
            for (int i = 0; i < n; ++i) {
                const double t = t0 + i / 60.0;
                world.render(options.width, options.height, t, audio.at(t));
            }
            glFinish();
            QTextStream(stderr) << QString::asprintf("t=%5.1f  %.2f ms/frame\n", t0, timer.nsecsElapsed() / 1e6 / n);
        }
        return 0;
    }
    if (!options.stills.isEmpty()) {
        QDir dir(options.outDir.isEmpty() ? QStringLiteral(".") : options.outDir);
        if (!dir.exists() && !QDir().mkpath(dir.absolutePath())) return fail(QStringLiteral("Cannot create output directory."));
        for (const QString& part : options.stills.split(',', Qt::SkipEmptyParts)) {
            bool ok = false;
            const double t = part.toDouble(&ok);
            if (!ok || t < 0 || t > options.duration) return fail(QStringLiteral("Bad still time: %1").arg(part));
            QElapsedTimer timer;
            timer.start();
            const QImage image = frameImage(world, options, audio, t, rgb);
            const QString path = dir.filePath(QStringLiteral("still-%1.png").arg(t, 6, 'f', 2, QLatin1Char('0')));
            if (!image.save(path)) return fail(QStringLiteral("Cannot write %1").arg(path));
            QTextStream(stderr) << path << "  " << timer.nsecsElapsed() / 1e6 << " ms\n";
        }
        return 0;
    }

    const int count = int(std::ceil(options.duration * options.fps - 1e-9));
    if (!options.video.isEmpty()) {
        // Raw RGB frames into ffmpeg; audio is muxed from the analysed fixture.
        QString cmd = QStringLiteral("ffmpeg -v error -y -f rawvideo -pix_fmt rgb24 -s %1x%2 -r %3 -i - ")
            .arg(options.width).arg(options.height).arg(options.fps);
        if (!options.audio.isEmpty())
            cmd += QStringLiteral("-f f32le -ar 44100 -ac 2 -ss %1 -t %2 -i '%3' ")
                .arg(options.offset, 0, 'f', 6).arg(options.duration, 0, 'f', 6).arg(options.audio);
        // Fade in from black and out at the end so the film does not cut mid-phrase.
        const double end = options.duration;
        cmd += QStringLiteral("-vf fade=in:st=0:d=0.6,fade=out:st=%1:d=1.2 ").arg(end - 1.2, 0, 'f', 3);
        cmd += QStringLiteral("-c:v libx264 -preset slow -crf 16 -pix_fmt yuv420p -movflags +faststart ");
        if (!options.audio.isEmpty())
            cmd += QStringLiteral("-af afade=in:st=0:d=0.4,afade=out:st=%1:d=1.6 -c:a aac -b:a 256k -shortest ").arg(end - 1.6, 0, 'f', 3);
        cmd += QStringLiteral("'%1'").arg(options.video);
        FILE* pipe = popen(cmd.toLocal8Bit().constData(), "w");
        if (!pipe) return fail(QStringLiteral("Cannot start ffmpeg."));
        QElapsedTimer timer;
        timer.start();
        for (int frame = 0; frame < count; ++frame) {
            const double t = double(frame) / options.fps;
            world.render(options.width, options.height, t, audio.at(t));
            world.gpu().readRgb(rgb);
            if (std::fwrite(rgb.data(), 1, rgb.size(), pipe) != rgb.size()) { pclose(pipe); return fail(QStringLiteral("ffmpeg pipe closed.")); }
            if (frame % (options.fps * 5) == 0)
                QTextStream(stderr) << "Frame " << frame << '/' << count << "  " << timer.elapsed() / 1000.0 << " s\n";
        }
        if (pclose(pipe) != 0) return fail(QStringLiteral("ffmpeg failed."));
        QTextStream(stdout) << "Wrote " << count << " frames to " << options.video << '\n';
        return 0;
    }

    QDir directory(options.exportDir);
    if (!options.overwrite) {
        for (int frame = 0; frame < count; ++frame) {
            const QFileInfo target(framePath(directory, frame));
            if (target.exists() || target.isSymLink())
                return fail(QStringLiteral("Export target exists: %1. Use --overwrite to replace it.").arg(target.filePath()));
        }
    }
    if (!directory.exists() && !QDir().mkpath(directory.absolutePath()))
        return fail(QStringLiteral("Cannot create export directory: %1").arg(directory.absolutePath()));
    for (int frame = 0; frame < count; ++frame) {
        const double seconds = double(frame) / options.fps;
        const QImage image = frameImage(world, options, audio, seconds, rgb);
        const QString target = framePath(directory, frame);
        QSaveFile file(target);
        if (!file.open(QIODevice::WriteOnly)) return fail(file.errorString());
        QImageWriter writer(&file, "jpeg");
        writer.setQuality(94);
        if (!writer.write(image)) return fail(QStringLiteral("%1: %2").arg(target, writer.errorString()));
        if (!file.commit()) return fail(QStringLiteral("%1: %2").arg(target, file.errorString()));
        if (frame % options.fps == 0) QTextStream(stderr) << "Export " << frame << '/' << count << '\n';
    }
    QTextStream(stdout) << "Exported " << count << " JPEG frames to " << directory.absolutePath() << '\n';
    return 0;
}
}

int main(int argc, char* argv[]) {
    QCoreApplication::setApplicationName(QStringLiteral("omadrop-journey"));
    QCoreApplication::setApplicationVersion(QStringLiteral("study"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Omarchy Journey: Osaka Jade to Kanagawa"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
        {QStringLiteral("audio"), QStringLiteral("Recorded stereo 44100 Hz float32 little-endian PCM; omitted = silence."), QStringLiteral("path")},
        {QStringLiteral("audio-offset"), QStringLiteral("Fixture seconds corresponding to movie time zero."), QStringLiteral("seconds"), QStringLiteral("0")},
        {QStringLiteral("duration"), QStringLiteral("Movie length, at most 600 seconds (default: 60)."), QStringLiteral("seconds"), QStringLiteral("60")},
        {QStringLiteral("at"), QStringLiteral("Initial native movie time."), QStringLiteral("seconds"), QStringLiteral("0")},
        {QStringLiteral("paused"), QStringLiteral("Start native playback paused.")},
        {QStringLiteral("export-dir"), QStringLiteral("Headless JPEG export destination."), QStringLiteral("path")},
        {QStringLiteral("video"), QStringLiteral("Headless H.264 export (audio muxed from --audio)."), QStringLiteral("path")},
        {QStringLiteral("stills"), QStringLiteral("Comma-separated movie times to export as PNG."), QStringLiteral("times")},
        {QStringLiteral("out-dir"), QStringLiteral("Directory for --stills."), QStringLiteral("path")},
        {QStringLiteral("inspect"), QStringLiteral("Print a per-second audio feature report for the whole --audio file.")},
        {QStringLiteral("bench"), QStringLiteral("Headless: time rendering at these movie times (comma list), no output."), QStringLiteral("times")},
        {QStringLiteral("seed"), QStringLiteral("Variation seed for details of the authored timeline."), QStringLiteral("n"), QStringLiteral("1")},
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
        || !integer(parser, QStringLiteral("seed"), 0, 1 << 30, options.seed, error)
        || !integer(parser, QStringLiteral("width"), 1, 8192, options.width, error)
        || !integer(parser, QStringLiteral("height"), 1, 8192, options.height, error)) return fail(error);
    if (qint64(options.width) * options.height > 33554432)
        return fail(QStringLiteral("Image dimensions exceed the 33554432-pixel limit."));
    if (options.offset + options.duration > 21600)
        return fail(QStringLiteral("Audio offset + duration exceeds 21600 seconds."));
    options.audio = parser.value(QStringLiteral("audio"));
    options.exportDir = parser.value(QStringLiteral("export-dir"));
    options.video = parser.value(QStringLiteral("video"));
    options.stills = parser.value(QStringLiteral("stills"));
    options.bench = parser.value(QStringLiteral("bench"));
    options.outDir = parser.value(QStringLiteral("out-dir"));
    options.paused = parser.isSet(QStringLiteral("paused"));
    options.overwrite = parser.isSet(QStringLiteral("overwrite"));
    options.inspect = parser.isSet(QStringLiteral("inspect"));
    const bool exporting = parser.isSet(QStringLiteral("export-dir")) || parser.isSet(QStringLiteral("video"))
        || parser.isSet(QStringLiteral("stills")) || parser.isSet(QStringLiteral("bench"));
    if (parser.isSet(QStringLiteral("audio")) && options.audio.isEmpty()) return fail(QStringLiteral("--audio path is empty."));
    if (options.inspect) {
        if (options.audio.isEmpty()) return fail(QStringLiteral("--inspect requires --audio."));
        QCoreApplication app(argc, argv);
        return inspect(options);
    }
    if (options.audio.isEmpty() && options.offset != 0) return fail(QStringLiteral("--audio-offset requires --audio."));
    if (parser.isSet(QStringLiteral("export-dir")) && options.exportDir.isEmpty()) return fail(QStringLiteral("--export-dir path is empty."));
    if (exporting && (parser.isSet(QStringLiteral("at")) || options.paused))
        return fail(QStringLiteral("--at and --paused apply to native playback; export always starts at movie time zero."));
    if (!exporting && (options.overwrite || parser.isSet(QStringLiteral("fps"))))
        return fail(QStringLiteral("--overwrite and --fps require an export option."));

    Journey::AudioTimeline audio;
    if (!options.audio.isEmpty() && !audio.load(options.audio, options.offset, options.duration, error))
        return fail(error);
    Journey::Score score;
    score.build(audio, options.duration, !options.audio.isEmpty());

    // Export never initializes a platform plugin or a display.
    if (exporting) {
        QCoreApplication app(argc, argv);
        return exportFrames(options, audio, score);
    }
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapInterval(1);
    QSurfaceFormat::setDefaultFormat(format);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QGuiApplication app(argc, argv);
    app.setDesktopFileName(QStringLiteral("omadrop-journey-study"));
    qmlRegisterType<JourneyItem>("Journey", 1, 0, "JourneyItem");
    qmlRegisterUncreatableType<JourneyBackend>("Journey", 1, 0, "JourneyBackend", "Provided by the native harness");
    JourneyBackend backend(std::move(audio), std::move(score), options.duration, options.at, options.paused, options.seed);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("journeyBackend"), &backend);
    engine.rootContext()->setContextProperty(QStringLiteral("initialWidth"), options.width);
    engine.rootContext()->setContextProperty(QStringLiteral("initialHeight"), options.height);
    engine.load(QUrl(QStringLiteral("qrc:/journey/Main.qml")));
    if (engine.rootObjects().isEmpty()) return fail(QStringLiteral("Cannot load the native Journey window."));
    return app.exec();
}
