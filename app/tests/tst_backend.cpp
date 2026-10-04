// Unit/smoke tests for the Omadrop native product controller.
//
// No GUI and no real session: the controller dispatcher, hyprctl and the
// renderer are local fake scripts, driven by environment overrides and
// scratch paths. The backend talks to them through QProcess exactly as it
// would in production.

#include <QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextStream>
#include <QVariantMap>
#include <QLocalServer>
#include <QLocalSocket>

#include "../src/backend.h"
#include "../src/startup_request.h"

namespace {
void makeExecutable(const QString& path, const QString& contents) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream stream(&file);
    stream << contents;
    file.close();
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                                    | QFile::ReadGroup | QFile::ExeGroup
                                    | QFile::ReadOther | QFile::ExeOther);
}

void writeFile(const QString& path, const QString& contents) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream stream(&file);
    stream << contents;
}

QString readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}
} // namespace

class BackendTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void curtainFailsWithStart();
    void captionsRoundTrip();
    void firstRunDisplayIgnoresLegacyDefault();
    void errorDetailsLoadLastLines();
    void defaultsWithoutStoredPreferences();
    void startupRequests();
    void rendererExitReturnsImmediately();
    void compositorEventsDetectWindows();
    void rendererIdentityDetectsMappedWindow();
    void alphabeticalGrids();
    void loadsStoredPreferences();
    void selectionsPersistToProductConfOnly();
    void playMapsWhenSessionWindowAppears();
    void playReportsStartupFailure();
    void pollFailureStillTimesOut();
    void stopDispatchesStop();
    void missingControllerReportsError();
    void defaultsToInstalledOsaka();
    void explicitUiModeDoesNotRewriteSettings();
    void startupDeadlineWithBrokenQuery_data();
    void startupDeadlineWithBrokenQuery();
    void brokenPollDoesNotEndPlayback_data();
    void brokenPollDoesNotEndPlayback();
    void unrelatedTitleDoesNotCountAsPlayback();
    void preexistingWindowsDoNotCountAsNewSession();
    void stopCancelsLaunchAndQueuedWindows();
    void runtimeCrashReportsFailure();
    void preferencesFailureIsVisible();
    void scenesLoadFromManifest();
    void scenesReadHiddenConf();
    void scenesConfGarbledIsIgnored();
    void toggleSceneHiddenWritesConf();
    void playSceneDispatchesSceneArgument();
    void pathDefaultsRelativeToAppDir();

private:
    void writeClients(const QString& json);
    QVariantMap sceneByNumber(const Backend& backend, int number) const;

    QTemporaryDir m_dir;
    QString m_controller;
    QString m_hyprctl;
    QString m_controlLog;
    QString m_clients;
    QString m_configHome;
    QString m_productConf;
    QString m_preferencesConf;
    QString m_modeConf;
    QString m_scenesConf;
    QString m_manifest;
};

void BackendTest::init() {
    const QString root = m_dir.path();
    QDir(root).removeRecursively();
    QDir().mkpath(root);

    m_controller = root + "/omadrop";
    m_hyprctl = root + "/hyprctl";
    m_controlLog = root + "/control.log";
    m_clients = root + "/clients.json";
    m_configHome = root + "/config";
    m_productConf = m_configHome + "/omadrop/product.conf";
    m_preferencesConf = m_configHome + "/omadrop/preferences.conf";
    m_modeConf = m_configHome + "/omadrop/mode.conf";
    m_scenesConf = m_configHome + "/omadrop/scenes.conf";
    m_manifest = root + "/manifest.json";

    makeExecutable(m_controller,
                   "#!/bin/sh\n"
                   "printf '%s\\n' \"$*\" >>\"$OMADROP_TEST_CONTROL_LOG\"\n");
    makeExecutable(m_hyprctl,
                   "#!/bin/sh\n"
                   "if [ \"$1\" = clients ]; then cat \"$OMADROP_TEST_CLIENTS\" 2>/dev/null; fi\n");
    writeFile(m_controlLog, QString());
    writeClients(QStringLiteral("[]"));

    qputenv("OMADROP_CONTROLLER_BACKEND", m_controller.toUtf8());
    qputenv("OMADROP_HYPRCTL", m_hyprctl.toUtf8());
    qputenv("OMADROP_MILKDROP_LIVE", (root + "/no-such-renderer").toUtf8());
    qputenv("OMADROP_COLLECTION_MANIFEST", m_manifest.toUtf8());
    qputenv("OMADROP_POLL_INTERVAL_MS", "40");
    qputenv("OMADROP_STARTUP_TIMEOUT_MS", "500");
    qputenv("OMADROP_QUERY_TIMEOUT_MS", "1000");
    qunsetenv("OMADROP_UI_MODE");
    qunsetenv("OMADROP_HYPR_EVENT_SOCKET");
    qunsetenv("OMADROP_MILKDROP_BACKEND");
    qunsetenv("OMADROP_OMARCHY_BACKEND");
    qputenv("OMADROP_TEST_CONTROL_LOG", m_controlLog.toUtf8());
    qputenv("OMADROP_TEST_CLIENTS", m_clients.toUtf8());
    qputenv("HOME", root.toUtf8());
    qputenv("XDG_CONFIG_HOME", m_configHome.toUtf8());
    qputenv("XDG_DATA_HOME", (root + "/data").toUtf8());
    qputenv("XDG_STATE_HOME", (root + "/state").toUtf8());
}

void BackendTest::writeClients(const QString& json) {
    writeFile(m_clients, json);
}


QVariantMap BackendTest::sceneByNumber(const Backend& backend, int number) const {
    for (const QVariant& value : backend.scenes()) {
        const QVariantMap scene = value.toMap();
        if (scene.value(QStringLiteral("number")).toInt() == number) {
            return scene;
        }
    }
    return {};
}

void BackendTest::curtainFailsWithStart() {
    Backend backend;
    backend.play();
    QVERIFY(backend.curtainVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!backend.busy(), 3000);
    QVERIFY(!backend.curtainVisible());
    QVERIFY(!backend.playing());
    QVERIFY(!backend.error().isEmpty());
}

void BackendTest::captionsRoundTrip() {
    const QString preserved = QStringLiteral("version=4\nascii=1\nfavorite=paper-horizon\nmotion=0.65\n");
    writeFile(m_preferencesConf, preserved);
    Backend backend;
    QVERIFY(backend.captions());
    backend.setCaptions(false);
    QCOMPARE(readFile(m_preferencesConf), preserved + QStringLiteral("captions=0\n"));
    Backend restored;
    QVERIFY(!restored.captions());
    restored.setCaptions(true);
    Backend enabled;
    QVERIFY(enabled.captions());
    makeExecutable(m_controller, QStringLiteral(
        "#!/bin/sh\nprintf '%s %s\\n' \"$*\" \"$OMADROP_CAPTIONS\" >>\"$OMADROP_TEST_CONTROL_LOG\"\n"));
    backend.play();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--no-ascii 0"))
                || readFile(m_controlLog).contains(QStringLiteral("--ascii 0")));
}

void BackendTest::firstRunDisplayIgnoresLegacyDefault() {
    writeFile(m_preferencesConf, QStringLiteral("version=4\ndisplay=all\n"));
    Backend firstRun;
    QCOMPARE(firstRun.display(), QStringLiteral("single"));
    firstRun.setDisplay(QStringLiteral("all"));
    Backend returning;
    QCOMPARE(returning.display(), QStringLiteral("all"));
}

void BackendTest::errorDetailsLoadLastLines() {
    QString lines;
    for (int i = 0; i < 30; ++i) lines += QStringLiteral("crash line %1\n").arg(i);
    writeFile(qEnvironmentVariable("XDG_STATE_HOME") + "/omadrop/last-crash.txt", lines);
    QFile::remove(m_controller);
    Backend backend;
    backend.play();
    QVERIFY(!backend.error().isEmpty());
    QVERIFY(backend.errorDetails().startsWith(QStringLiteral("crash line 10\n")));
    QVERIFY(backend.errorDetails().endsWith(QStringLiteral("crash line 29")));
    backend.clearError();
    QVERIFY(backend.errorDetails().isEmpty());
    QFile::remove(qEnvironmentVariable("XDG_STATE_HOME") + "/omadrop/last-crash.txt");
    backend.play();
    QVERIFY(backend.errorDetails().isEmpty());
}

void BackendTest::defaultsWithoutStoredPreferences() {
    Backend backend;
    QCOMPARE(backend.mode(), QStringLiteral("milkdrop"));
    QCOMPARE(backend.display(), QStringLiteral("single"));
    QVERIFY(!backend.ascii());
    QVERIFY(!backend.playing());
    QVERIFY(!backend.busy());
}

void BackendTest::startupRequests() {
    StartupRequest request;
    QVERIFY(!request.play);
    QVERIFY(request.accept(QStringLiteral("--play")));
    QVERIFY(request.play);
    QVERIFY(request.accept(QStringLiteral("--controls")));
    QVERIFY(!request.play);
    QVERIFY(!request.accept(QStringLiteral("--bogus")));
}

void BackendTest::rendererExitReturnsImmediately() {
    // Hold the launcher until the test signals normal renderer completion.
    const QString exitFile = m_dir.path() + "/exit";
    qputenv("OMADROP_TEST_EXIT_FILE", exitFile.toUtf8());
    makeExecutable(m_controller, QStringLiteral(
        "#!/bin/sh\n"
        "printf '%s\\n' \"$*\" >>\"$OMADROP_TEST_CONTROL_LOG\"\n"
        "[ \"$1\" = --stop ] && exit 0\n"
        "while [ ! -e \"$OMADROP_TEST_EXIT_FILE\" ]; do sleep 0.01; done\n"));
    Backend backend;
    backend.play();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode")));
    writeClients(QStringLiteral("[{\"class\":\"org.omadrop.screensaver\",\"pid\":999999}]"));
    QTRY_VERIFY(backend.playing());
    QSignalSpy show(&backend, &Backend::showControls);
    QElapsedTimer elapsed;
    elapsed.start();
    writeFile(exitFile, QStringLiteral("done"));
    QTRY_COMPARE_WITH_TIMEOUT(show.count(), 1, 250);
    QVERIFY(elapsed.elapsed() < 300);
    QVERIFY(!backend.busy());
    QVERIFY(!backend.playing());
    QVERIFY(!readFile(m_controlLog).contains(QStringLiteral("--stop")));
}

void BackendTest::compositorEventsDetectWindows() {
    QLocalServer server;
    const QString socketPath = m_dir.path() + "/events.sock";
    QVERIFY(server.listen(socketPath));
    qputenv("OMADROP_HYPR_EVENT_SOCKET", socketPath.toUtf8());
    qputenv("OMADROP_POLL_INTERVAL_MS", "1000");
    Backend backend;
    backend.setMode(QStringLiteral("omarchy"));
    backend.play();
    QTRY_VERIFY(server.hasPendingConnections());
    auto* socket = server.nextPendingConnection();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode")));
    QTest::qWait(60); // Let the initial empty snapshot settle.
    writeClients(QStringLiteral("[{\"class\":\"org.omadrop.screensaver\",\"pid\":999999,\"address\":\"0xabc\"}]"));
    socket->write("openwindow>>abc,1,org.omadrop.screensaver,Omadrop\n");
    socket->flush();
    QTRY_VERIFY_WITH_TIMEOUT(backend.playing(), 250);
    QVERIFY(!backend.curtainVisible());
    QSignalSpy show(&backend, &Backend::showControls);
    socket->write("closewindow>>unrelated\n");
    socket->flush();
    QTest::qWait(20);
    QVERIFY(backend.playing());
    QElapsedTimer elapsed;
    elapsed.start();
    socket->write("closewindow>>abc\n");
    socket->flush();
    QTRY_VERIFY_WITH_TIMEOUT(show.count() > 0, 250);
    QVERIFY(elapsed.elapsed() < 300);
    QVERIFY(!backend.playing());
    QTRY_VERIFY(!backend.busy());
}

void BackendTest::alphabeticalGrids() {
    writeFile(m_manifest, QStringLiteral(R"({"presets":[
        {"number":1,"label":"zebra"},{"number":2,"label":"Apple"},
        {"number":3,"label":"banana"},{"number":4,"label":"Apricot"}]})"));
    writeFile(m_scenesConf, QStringLiteral("version=1\nhidden=4\n"));
    Backend backend;
    QCOMPARE(backend.scenes().at(0).toMap().value("number").toInt(), 2);
    QCOMPARE(backend.scenes().at(1).toMap().value("number").toInt(), 3);
    QCOMPARE(backend.scenes().at(2).toMap().value("number").toInt(), 1);
    QCOMPARE(backend.scenes().at(3).toMap().value("number").toInt(), 4);
    backend.toggleSceneHidden(4);
    QCOMPARE(backend.scenes().at(1).toMap().value("number").toInt(), 4);

}

void BackendTest::rendererIdentityDetectsMappedWindow() {
    qputenv("OMADROP_MILKDROP_LIVE", "/bin/sleep");
    QProcess renderer;
    renderer.start(QStringLiteral("/bin/sleep"), {QStringLiteral("30")});
    QVERIFY(renderer.waitForStarted());
    Backend backend;
    backend.play();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode")));
    const QString client = QStringLiteral(
        "[{\"class\":\"org.omadrop.milkdrop\",\"pid\":%1,\"mapped\":%2}]");
    writeClients(client.arg(renderer.processId()).arg(QStringLiteral("false")));
    QTest::qWait(60);
    QVERIFY(!backend.playing());
    writeClients(client.arg(renderer.processId()).arg(QStringLiteral("true")));
    QTRY_VERIFY(backend.playing());
    renderer.terminate();
    QVERIFY(renderer.waitForFinished());
    writeClients(QStringLiteral("[]"));
    QTRY_VERIFY(!backend.playing());
}

void BackendTest::loadsStoredPreferences() {
    writeFile(m_preferencesConf, QStringLiteral("version=4\nascii=1\ndisplay=single\n"));
    writeFile(m_modeConf, QStringLiteral("mode=omarchy\n"));
    Backend backend;
    QCOMPARE(backend.mode(), QStringLiteral("omarchy"));
    QCOMPARE(backend.display(), QStringLiteral("single"));
    QVERIFY(backend.ascii());
}

void BackendTest::selectionsPersistToProductConfOnly() {
    const QString preferences = QStringLiteral("version=4\nascii=1\ndisplay=single\n");
    writeFile(m_preferencesConf, preferences);
    writeFile(m_modeConf, QStringLiteral("mode=omarchy\n"));

    Backend backend;
    QSignalSpy spy(&backend, &Backend::stateChanged);
    backend.setMode(QStringLiteral("milkdrop"));
    backend.setDisplay(QStringLiteral("all"));
    backend.setAscii(false);
    QVERIFY(spy.count() >= 3);

    const QString product = readFile(m_productConf);
    QVERIFY(product.contains(QStringLiteral("mode=milkdrop")));
    QVERIFY(product.contains(QStringLiteral("display=all")));
    QVERIFY(product.contains(QStringLiteral("ascii=0")));
    QCOMPARE(readFile(m_preferencesConf), preferences);
    QCOMPARE(readFile(m_modeConf), QStringLiteral("mode=omarchy\n"));
}

void BackendTest::playMapsWhenSessionWindowAppears() {
    Backend backend;
    backend.setDisplay(QStringLiteral("single"));
    backend.setAscii(true);
    QSignalSpy show(&backend, &Backend::showControls);

    backend.play();
    QVERIFY(backend.curtainVisible());
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode milkdrop --single --ascii")));
    writeClients(QStringLiteral(
        "[{\"class\":\"org.omadrop.screensaver\",\"mapped\":true,\"pid\":999999}]"));
    QTRY_VERIFY(backend.playing());
    QVERIFY(!backend.busy());
    QVERIFY(!backend.curtainVisible());
    QVERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode milkdrop --single --ascii")));

    writeClients(QStringLiteral("[]"));
    QTRY_VERIFY(!backend.playing());
    QTRY_VERIFY(show.count() >= 1);
}

void BackendTest::playReportsStartupFailure() {
    writeClients(QStringLiteral("[]"));
    Backend backend;
    QSignalSpy show(&backend, &Backend::showControls);
    backend.play();
    QTRY_VERIFY_WITH_TIMEOUT(!backend.busy(), 3000);
    QVERIFY(!backend.playing());
    QVERIFY(!backend.error().isEmpty());
    QVERIFY(show.count() >= 1);
}

void BackendTest::pollFailureStillTimesOut() {
    // A missing hyprctl must resolve to the startup-timeout error, not hang busy.
    qputenv("OMADROP_HYPRCTL", (m_dir.path() + "/missing-hyprctl").toUtf8());
    Backend backend;
    QSignalSpy show(&backend, &Backend::showControls);
    backend.play();
    QTRY_VERIFY_WITH_TIMEOUT(!backend.busy(), 3000);
    QVERIFY(!backend.error().isEmpty());
    QVERIFY(show.count() >= 1);
}


void BackendTest::stopDispatchesStop() {
    Backend backend;
    backend.stop();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--stop")));
    QVERIFY(!backend.playing());
    QTRY_VERIFY(!backend.busy());
}

void BackendTest::missingControllerReportsError() {
    qputenv("OMADROP_CONTROLLER_BACKEND", (m_dir.path() + "/missing").toUtf8());
    Backend backend;
    QSignalSpy show(&backend, &Backend::showControls);
    backend.play();
    QVERIFY(!backend.busy());
    QVERIFY(!backend.error().isEmpty());
    QVERIFY(show.count() >= 1);
}


void BackendTest::defaultsToInstalledOsaka() {
    qputenv("OMADROP_OMARCHY_BACKEND", m_controller.toUtf8());
    Backend backend;
    QVERIFY(!backend.milkdropAvailable());
    QVERIFY(backend.omarchyAvailable());
    QCOMPARE(backend.mode(), QStringLiteral("omarchy"));
    QVERIFY(!QFileInfo::exists(m_productConf));
}

void BackendTest::explicitUiModeDoesNotRewriteSettings() {
    const QString settings = QStringLiteral("mode=milkdrop\ndisplay=single\nascii=1\n");
    writeFile(m_productConf, settings);
    qputenv("OMADROP_UI_MODE", "omarchy");
    Backend backend;
    QCOMPARE(backend.mode(), QStringLiteral("omarchy"));
    QCOMPARE(readFile(m_productConf), settings);
}

void BackendTest::startupDeadlineWithBrokenQuery_data() {
    QTest::addColumn<QString>("script");
    QTest::newRow("missing") << QString();
    QTest::newRow("hanging") << QStringLiteral("#!/bin/sh\nsleep 20\n");
}

void BackendTest::startupDeadlineWithBrokenQuery() {
    QFETCH(QString, script);
    if (script.isEmpty()) QFile::remove(m_hyprctl);
    else makeExecutable(m_hyprctl, script);
    qputenv("OMADROP_QUERY_TIMEOUT_MS", "100");
    Backend backend;
    QSignalSpy show(&backend, &Backend::showControls);
    backend.play();
    QTRY_VERIFY_WITH_TIMEOUT(show.count() >= 1, 4000);
    QTRY_VERIFY_WITH_TIMEOUT(!backend.busy(), 4000);
    QVERIFY(!backend.playing());
    QVERIFY(backend.error().contains(QStringLiteral("did not start")));
}

void BackendTest::brokenPollDoesNotEndPlayback_data() {
    QTest::addColumn<QString>("script");
    QTest::newRow("nonzero") << QStringLiteral("#!/bin/sh\nexit 9\n");
    QTest::newRow("malformed") << QStringLiteral("#!/bin/sh\necho invalid-json\n");
    QTest::newRow("invalid-clients") << QStringLiteral("#!/bin/sh\necho '[null]'\n");
    QTest::newRow("hanging") << QStringLiteral("#!/bin/sh\nsleep 20\n");
}

void BackendTest::brokenPollDoesNotEndPlayback() {
    QFETCH(QString, script);
    qputenv("OMADROP_QUERY_TIMEOUT_MS", "100");
    Backend backend;
    QSignalSpy show(&backend, &Backend::showControls);
    backend.play();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode")));
    writeClients(QStringLiteral("[{\"class\":\"org.omadrop.screensaver\",\"pid\":999999}]"));
    QTRY_VERIFY(backend.playing());
    makeExecutable(m_hyprctl, script);
    QTRY_VERIFY(!backend.error().isEmpty());
    QVERIFY(backend.playing());
    QCOMPARE(show.count(), 0);
    backend.stop();
    QTRY_VERIFY(!backend.busy());
}

void BackendTest::unrelatedTitleDoesNotCountAsPlayback() {
    Backend backend;
    backend.play();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode")));
    writeClients(QStringLiteral("[{\"class\":\"unrelated\",\"title\":\"Omadrop\",\"pid\":999999}]"));
    QTRY_VERIFY(!backend.busy());
    QVERIFY(!backend.playing());
    QVERIFY(backend.error().contains(QStringLiteral("did not start")));
}

void BackendTest::preexistingWindowsDoNotCountAsNewSession() {
    writeClients(QStringLiteral("[{\"class\":\"org.omadrop.screensaver\",\"pid\":999998}]"));
    Backend backend;
    backend.play();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode")));
    QVERIFY(!backend.playing());
    writeClients(QStringLiteral("[{\"class\":\"org.omadrop.screensaver\",\"pid\":999999}]"));
    QTRY_VERIFY(backend.playing());
    backend.stop();
    QTRY_VERIFY(!backend.busy());
}

void BackendTest::stopCancelsLaunchAndQueuedWindows() {
    const QString root = m_dir.path();
    qputenv("OMADROP_TEST_CANCEL_PATH", (root + "/cancel-path").toUtf8());
    qputenv("OMADROP_TEST_LATE_WINDOW", (root + "/late-window").toUtf8());
    qputenv("OMADROP_TEST_LATE_DONE", (root + "/late-done").toUtf8());
    qputenv("OMADROP_TEST_CHILD_SURVIVED", (root + "/child-survived").toUtf8());
    makeExecutable(m_controller, QStringLiteral(
        "#!/bin/sh\n"
        "printf '%s\\n' \"$*\" >>\"$OMADROP_TEST_CONTROL_LOG\"\n"
        "[ \"$1\" = --stop ] && exit 0\n"
        "setsid sh -c 'sleep 0.6; [ -e \"$OMADROP_CANCEL_FILE\" ] || echo mapped >\"$OMADROP_TEST_LATE_WINDOW\"; echo done >\"$OMADROP_TEST_LATE_DONE\"' >/dev/null 2>&1 &\n"
        "sh -c 'sleep 0.6; echo survived >\"$OMADROP_TEST_CHILD_SURVIVED\"' &\n"
        "printf '%s' \"$OMADROP_CANCEL_FILE\" >\"$OMADROP_TEST_CANCEL_PATH\"\n"
        "wait\n"));
    Backend backend;
    QSignalSpy stopped(&backend, &Backend::stopCompleted);
    backend.play();
    QTRY_VERIFY(QFileInfo::exists(root + "/cancel-path"));
    const QString cancelFile = readFile(root + "/cancel-path");
    QVERIFY(!cancelFile.isEmpty());
    QElapsedTimer clock;
    clock.start();
    backend.stop();
    QVERIFY(clock.elapsed() < 200);
    QVERIFY(QFileInfo::exists(cancelFile));
    QTRY_VERIFY(QFileInfo::exists(root + "/late-done"));
    QVERIFY(!QFileInfo::exists(root + "/late-window"));
    QVERIFY(!QFileInfo::exists(root + "/child-survived"));
    QTRY_COMPARE(stopped.count(), 1);
    QVERIFY(!backend.busy());
}

void BackendTest::runtimeCrashReportsFailure() {
    makeExecutable(m_controller, QStringLiteral(
        "#!/bin/sh\n"
        "printf '%s\\n' \"$*\" >>\"$OMADROP_TEST_CONTROL_LOG\"\n"
        "[ \"$1\" = --stop ] && exit 0\n"
        "sleep 0.35\nexit 17\n"));
    Backend backend;
    backend.play();
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode")));
    writeClients(QStringLiteral("[{\"class\":\"org.omadrop.screensaver\",\"pid\":999999}]"));
    QTRY_VERIFY(backend.playing());
    QTRY_COMPARE(backend.error(), QStringLiteral("The visuals stopped unexpectedly."));
    QTRY_VERIFY(!backend.busy());
    QVERIFY(!backend.playing());
}



void BackendTest::preferencesFailureIsVisible() {
    writeFile(m_configHome + "/omadrop", QStringLiteral("a file blocks the settings directory"));
    Backend backend;
    backend.setDisplay(QStringLiteral("all"));
    QVERIFY(backend.error().contains(QStringLiteral("settings")));
}

namespace {
const char* kManifest = R"({
  "presets": [
    {"number": 1, "label": "Cloud Cubes", "appearance": "Reflective cubes"},
    {"number": 2, "label": "Fractal Caves", "appearance": "Fractal landscape"},
    {"number": 3, "label": "Ice Wave", "appearance": "Blue wave"}
  ]
})";
} // namespace

void BackendTest::scenesLoadFromManifest() {
    writeFile(m_manifest, QString::fromUtf8(kManifest));
    Backend backend;
    QCOMPARE(backend.scenes().size(), 3);
    const QVariantMap first = backend.scenes().at(0).toMap();
    QCOMPARE(first.value(QStringLiteral("number")).toInt(), 1);
    QCOMPARE(first.value(QStringLiteral("label")).toString(), QStringLiteral("Cloud Cubes"));
    QCOMPARE(first.value(QStringLiteral("description")).toString(), QStringLiteral("Reflective cubes"));
    QVERIFY(!first.value(QStringLiteral("hidden")).toBool());
    QCOMPARE(first.value(QStringLiteral("thumbnail")).toString(),
             QStringLiteral("qrc:/assets/scenes/collection-01.jpg"));
    QCOMPARE(backend.scenes().at(2).toMap().value(QStringLiteral("thumbnail")).toString(),
             QStringLiteral("qrc:/assets/scenes/collection-03.jpg"));
}

void BackendTest::scenesReadHiddenConf() {
    writeFile(m_manifest, QString::fromUtf8(kManifest));
    writeFile(m_scenesConf, QStringLiteral("version=1\nhidden=2,3\n"));
    Backend backend;
    QCOMPARE(backend.scenes().size(), 3);
    QVERIFY(!sceneByNumber(backend, 1).value(QStringLiteral("hidden")).toBool());
    QVERIFY(sceneByNumber(backend, 2).value(QStringLiteral("hidden")).toBool());
    QVERIFY(sceneByNumber(backend, 3).value(QStringLiteral("hidden")).toBool());
}

void BackendTest::scenesConfGarbledIsIgnored() {
    writeFile(m_manifest, QString::fromUtf8(kManifest));
    writeFile(m_scenesConf, QStringLiteral("this is not a conf\nversion = nope\nhidden = 1\n"));
    Backend backend;
    QCOMPARE(backend.scenes().size(), 3);
    for (const QVariant& value : backend.scenes()) {
        QVERIFY(!value.toMap().value(QStringLiteral("hidden")).toBool());
    }
    // A garbled file is replaced with a well-formed one on the next write.
    backend.toggleSceneHidden(2);
    QVERIFY(sceneByNumber(backend, 2).value(QStringLiteral("hidden")).toBool());
    QCOMPARE(readFile(m_scenesConf), QStringLiteral("version=1\nhidden=2\n"));
}

void BackendTest::toggleSceneHiddenWritesConf() {
    writeFile(m_manifest, QString::fromUtf8(kManifest));
    Backend backend;
    QSignalSpy scenes(&backend, &Backend::scenesChanged);
    backend.toggleSceneHidden(3);
    QVERIFY(sceneByNumber(backend, 3).value(QStringLiteral("hidden")).toBool());
    QVERIFY(scenes.count() >= 1);
    QCOMPARE(readFile(m_scenesConf), QStringLiteral("version=1\nhidden=3\n"));

    backend.toggleSceneHidden(3);
    QVERIFY(!sceneByNumber(backend, 3).value(QStringLiteral("hidden")).toBool());
    QCOMPARE(readFile(m_scenesConf), QStringLiteral("version=1\nhidden=\n"));
}

void BackendTest::playSceneDispatchesSceneArgument() {
    Backend backend;
    backend.playScene(7);
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode milkdrop --scene 7")));
}

void BackendTest::pathDefaultsRelativeToAppDir() {
    qunsetenv("OMADROP_OMARCHY_BACKEND");
    Backend backend;
    const QString expected = QCoreApplication::applicationDirPath() + "/omadrop-osaka";
    QCOMPARE(backend.omarchyAvailable(), QFileInfo(expected).isExecutable());
    QVERIFY(backend.error().isEmpty());
}

QTEST_GUILESS_MAIN(BackendTest)
#include "tst_backend.moc"
