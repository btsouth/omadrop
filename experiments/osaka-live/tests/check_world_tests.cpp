// End to end: runs the world check on real world folders on this GPU (or
// Mesa software rendering) and expects the right check to pass or fail.
//   osaka-check-tests WORLDS_ROOT WORLD ready|fail:ID[,ID] [strobe]
//   osaka-check-tests --broken-art   (a world whose artwork cannot load)
#include "../src/kit/check.h"
#include "../src/kit/check-signal.h"
#include "../src/kit/world-loader.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>
#include <iostream>
#include <stdexcept>

using namespace Journey::Kit;
using namespace Journey::Kit::Check;

namespace {
void require(bool condition, const QString& message) {
    if (!condition) throw std::runtime_error(message.toStdString());
}

void brokenArt(const QString& worldsRoot, QTextStream& out) {
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory failed");
    const QString folder = temporary.filePath("broken");
    require(QDir().mkpath(folder), "mkpath failed");
    require(QFile::copy(worldsRoot + "/examples/lit-windows/scene.json", folder + "/scene.json"), "copy failed");
    QFile art(folder + "/art.svg");
    require(art.open(QIODevice::WriteOnly), "write failed");
    art.write("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1920 1080'>\n"
              "<text id='title' x='10' y='10'>Hello</text></svg>");
    art.close();
    qputenv("OMADROP_WORLDS", temporary.path().toUtf8());
    Options options;
    options.world = "broken";
    options.analysisSeconds = 4;
    const Report report = runWorldCheck(options);
    out << report.summary();
    require(!report.ready(), "a world with unloadable art was ready");
    const Item* loads = report.find("loads");
    require(loads && loads->status == Status::Fail, "loads did not fail");
    require(loads->message.contains("art.svg:2") && loads->message.contains("text"),
            "the importer diagnostic was not passed on verbatim: " + loads->message);
    for (const char* id : {"deterministic", "reacts", "budget", "flashing"})
        require(report.find(id)->status == Status::Skipped, QString("%1 was not skipped").arg(id));
    out << "PASS: unloadable artwork fails 'loads' with the importer's message and skips the rest\n";
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTextStream out(stdout), err(stderr);
    try {
        if (argc >= 3 && QString(argv[1]) == "--broken-art") { brokenArt(argv[2], out); return 0; }
        require(argc >= 4, "usage: osaka-check-tests WORLDS_ROOT WORLD ready|fail:ID [strobe]");
        const QString root = argv[1], world = argv[2], expect = argv[3];
        const bool strobe = argc >= 5 && QString(argv[4]) == "strobe";
        qputenv("OMADROP_WORLDS", root.toUtf8());
        QTemporaryDir temporary;
        Options options;
        options.world = world;
        options.analysisSeconds = 8;
        options.reference = QString(WORLDS_FOLDER) + "/osaka-jade";
        options.jsonPath = temporary.filePath("report.json");
        if (strobe) {
            const auto samples = makeCheckSignal(SignalKind::Strobe, 12);
            QFile file(temporary.filePath("strobe.f32"));
            require(file.open(QIODevice::WriteOnly), "fixture write failed");
            file.write(reinterpret_cast<const char*>(samples.data()), qint64(samples.size() * sizeof(float)));
            file.close();
            options.fixture = file.fileName();
        }
        const int code = runCheckCommand(options, out, err);
        out.flush();
        QFile json(options.jsonPath);
        require(json.open(QIODevice::ReadOnly), "JSON report was not written");
        const auto checks = QJsonDocument::fromJson(json.readAll()).object()["checks"].toArray();
        require(checks.size() == 5, "the JSON report does not hold five checks");
        const QStringList failing = expect == "ready" ? QStringList() : expect.mid(5).split(',');
        for (const auto& value : checks) {
            const auto item = value.toObject();
            const QString id = item["id"].toString(), status = item["status"].toString();
            if (failing.contains(id)) {
                require(status == "fail", id + " should have failed");
                require(!item["message"].toString().isEmpty(), id + " failed without a message");
            } else {
                require(status == "pass" || status == "info",
                        id + " should have passed: " + item["message"].toString());
            }
        }
        require(code == (failing.isEmpty() ? 0 : 1), QString("exit code was %1").arg(code));
        out << "PASS: " << world << " " << expect << (strobe ? " (strobe music)" : "") << '\n';
    } catch (const std::exception& e) {
        err << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
