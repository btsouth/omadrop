// Unit/smoke tests for the Omadrop native product controller.
//
// No GUI and no real session: the controller dispatcher, hyprctl and the
// effects helper are local fake scripts, driven by environment overrides and
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

#include "../src/backend.h"

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
    void defaultsWithoutStoredPreferences();
    void loadsStoredPreferences();
    void selectionsPersistToProductConfOnly();
    void playMapsWhenSessionWindowAppears();
    void playReportsStartupFailure();
    void pollFailureStillTimesOut();
    void previewDispatchesWithoutTouchingPreferences();
    void stopDispatchesStop();
    void missingControllerReportsError();
    void effectsDiscoveryAndToggles();
    void defaultsToInstalledScreensaver();
    void explicitUiModeDoesNotRewriteSettings();
    void startupDeadlineWithBrokenQuery_data();
    void startupDeadlineWithBrokenQuery();
    void brokenPollDoesNotEndPlayback_data();
    void brokenPollDoesNotEndPlayback();
    void unrelatedTitleDoesNotCountAsPlayback();
    void preexistingWindowsDoNotCountAsNewSession();
    void stopCancelsLaunchAndQueuedWindows();
    void runtimeCrashReportsFailure();
    void effectFailuresAreVisible();
    void effectRefreshRerunsAfterConcurrentToggle();
    void preferencesFailureIsVisible();

private:
    void writeClients(const QString& json);
    QVariantMap effectBySlug(const Backend& backend, const QString& slug) const;

    QTemporaryDir m_dir;
    QString m_controller;
    QString m_hyprctl;
    QString m_helper;
    QString m_ttfx;
    QString m_controlLog;
    QString m_helperLog;
    QString m_effectsState;
    QString m_clients;
    QString m_configHome;
    QString m_productConf;
    QString m_preferencesConf;
    QString m_modeConf;
};

void BackendTest::init() {
    const QString root = m_dir.path();
    QDir(root).removeRecursively();
    QDir().mkpath(root);

    m_controller = root + "/omadrop";
    m_hyprctl = root + "/hyprctl";
    m_helper = root + "/omadrop-effects";
    m_ttfx = root + "/ttfx-music";
    m_controlLog = root + "/control.log";
    m_helperLog = root + "/helper.log";
    m_effectsState = root + "/effects-state";
    m_clients = root + "/clients.json";
    m_configHome = root + "/config";
    m_productConf = m_configHome + "/omadrop/product.conf";
    m_preferencesConf = m_configHome + "/omadrop/preferences.conf";
    m_modeConf = m_configHome + "/omadrop/mode.conf";

    makeExecutable(m_controller,
                   "#!/bin/sh\n"
                   "printf '%s\\n' \"$*\" >>\"$OMADROP_TEST_CONTROL_LOG\"\n");
    makeExecutable(m_hyprctl,
                   "#!/bin/sh\n"
                   "if [ \"$1\" = clients ]; then cat \"$OMADROP_TEST_CLIENTS\" 2>/dev/null; fi\n");
    makeExecutable(m_ttfx,
                   "#!/bin/sh\n"
                   "if [ \"$1\" = --help ]; then\n"
                   "cat <<'EOF'\n"
                   "Terminal text effects\n"
                   "\n"
                   "Commands:\n"
                   "  beams            Beams description\n"
                   "  matrix           Matrix description\n"
                   "  rain             Rain description\n"
                   "  swarm            Swarm description\n"
                   "  help             Print help\n"
                   "EOF\n"
                   "fi\n");
    makeExecutable(m_helper,
                   "#!/bin/bash\n"
                   "state=\"$OMADROP_TEST_EFFECTS_STATE\"\n"
                   "printf '%s\\n' \"$*\" >>\"$OMADROP_TEST_EFFECTS_LOG\"\n"
                   "toggle() {\n"
                   "  local file=\"$state.$1\" slug=\"$2\"\n"
                   "  touch -- \"$file\"\n"
                   "  if grep -qxF -- \"$slug\" \"$file\"; then\n"
                   "    grep -vF -x -- \"$slug\" \"$file\" >\"$file.tmp\" || true\n"
                   "    mv \"$file.tmp\" \"$file\"\n"
                   "  else\n"
                   "    printf '%s\\n' \"$slug\" >>\"$file\"\n"
                   "  fi\n"
                   "}\n"
                   "status_of() {\n"
                   "  local fav=\"\" hid=\"\"\n"
                   "  grep -qxF -- \"$1\" \"$state.favorites\" 2>/dev/null && fav=1\n"
                   "  grep -qxF -- \"$1\" \"$state.hidden\" 2>/dev/null && hid=1\n"
                   "  if [ -n \"$fav\" ] && [ -n \"$hid\" ]; then printf 'favorite, hidden\\n'\n"
                   "  elif [ -n \"$fav\" ]; then printf 'favorite\\n'\n"
                   "  elif [ -n \"$hid\" ]; then printf 'hidden\\n'\n"
                   "  else printf 'enabled\\n'; fi\n"
                   "}\n"
                   "case \"$1\" in\n"
                   "  --list) for slug in beams matrix rain swarm; do\n"
                   "            printf '%s\\t%s\\t%s\\n' \"$slug\" \"$slug\" \"$(status_of \"$slug\")\"\n"
                   "          done ;;\n"
                   "  --favorite) toggle favorites \"$2\" ;;\n"
                   "  --hide) toggle hidden \"$2\" ;;\n"
                   "esac\n"
                   "exit 0\n");

    writeFile(m_controlLog, QString());
    writeFile(m_helperLog, QString());
    writeClients(QStringLiteral("[]"));

    qputenv("OMADROP_CONTROLLER_BACKEND", m_controller.toUtf8());
    qputenv("OMADROP_HYPRCTL", m_hyprctl.toUtf8());
    qputenv("OMADROP_EFFECTS_HELPER", m_helper.toUtf8());
    qputenv("OMADROP_EFFECTS_BINARY", m_ttfx.toUtf8());
    qputenv("OMADROP_MILKDROP_LIVE", (root + "/no-such-renderer").toUtf8());
    qputenv("OMADROP_POLL_INTERVAL_MS", "40");
    qputenv("OMADROP_STARTUP_TIMEOUT_MS", "500");
    qputenv("OMADROP_QUERY_TIMEOUT_MS", "1000");
    qunsetenv("OMADROP_UI_MODE");
    qunsetenv("OMADROP_MILKDROP_BACKEND");
    qunsetenv("OMADROP_OMARCHY_BACKEND");
    qputenv("OMADROP_TEST_CONTROL_LOG", m_controlLog.toUtf8());
    qputenv("OMADROP_TEST_EFFECTS_LOG", m_helperLog.toUtf8());
    qputenv("OMADROP_TEST_EFFECTS_STATE", m_effectsState.toUtf8());
    qputenv("OMADROP_TEST_CLIENTS", m_clients.toUtf8());
    qputenv("HOME", root.toUtf8());
    qputenv("XDG_CONFIG_HOME", m_configHome.toUtf8());
    qputenv("XDG_DATA_HOME", (root + "/data").toUtf8());
    qputenv("XDG_STATE_HOME", (root + "/state").toUtf8());
}

void BackendTest::writeClients(const QString& json) {
    writeFile(m_clients, json);
}

QVariantMap BackendTest::effectBySlug(const Backend& backend, const QString& slug) const {
    for (const QVariant& value : backend.effects()) {
        const QVariantMap effect = value.toMap();
        if (effect.value(QStringLiteral("slug")).toString() == slug) {
            return effect;
        }
    }
    return {};
}

void BackendTest::defaultsWithoutStoredPreferences() {
    Backend backend;
    QCOMPARE(backend.mode(), QStringLiteral("milkdrop"));
    QCOMPARE(backend.display(), QStringLiteral("all"));
    QVERIFY(!backend.ascii());
    QVERIFY(!backend.playing());
    QVERIFY(!backend.busy());
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
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--mode milkdrop --single --ascii")));
    writeClients(QStringLiteral(
        "[{\"class\":\"org.omadrop.screensaver\",\"mapped\":true,\"pid\":999999}]"));
    QTRY_VERIFY(backend.playing());
    QVERIFY(!backend.busy());
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

void BackendTest::previewDispatchesWithoutTouchingPreferences() {
    const QString preferences = QStringLiteral("version=4\nascii=1\ndisplay=single\n");
    writeFile(m_preferencesConf, preferences);
    writeFile(m_modeConf, QStringLiteral("mode=omarchy\n"));
    Backend backend;
    backend.preview(QStringLiteral("beams"));
    QTRY_VERIFY(readFile(m_controlLog).contains(QStringLiteral("--preview-effect beams")));
    QCOMPARE(readFile(m_preferencesConf), preferences);
    QCOMPARE(readFile(m_modeConf), QStringLiteral("mode=omarchy\n"));
    QVERIFY(!QFileInfo::exists(m_productConf));
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

void BackendTest::effectsDiscoveryAndToggles() {
    Backend backend;
    QTRY_COMPARE(backend.effects().size(), 4);

    const QVariantMap beams = effectBySlug(backend, QStringLiteral("beams"));
    QVERIFY(!beams.isEmpty());
    QCOMPARE(beams.value(QStringLiteral("slug")).toString(), QStringLiteral("beams"));
    QVERIFY(beams.contains(QStringLiteral("name")));
    QVERIFY(beams.contains(QStringLiteral("description")));
    QCOMPARE(beams.value(QStringLiteral("description")).toString(),
             QStringLiteral("Beams description"));
    QVERIFY(!beams.value(QStringLiteral("favorite")).toBool());
    QVERIFY(!beams.value(QStringLiteral("hidden")).toBool());

    backend.toggleFavorite(QStringLiteral("beams"));
    QTRY_VERIFY(effectBySlug(backend, QStringLiteral("beams"))
                    .value(QStringLiteral("favorite"))
                    .toBool());
    QVERIFY(readFile(m_helperLog).contains(QStringLiteral("--favorite beams")));

    backend.toggleHidden(QStringLiteral("swarm"));
    QTRY_VERIFY(effectBySlug(backend, QStringLiteral("swarm"))
                    .value(QStringLiteral("hidden"))
                    .toBool());
    QVERIFY(readFile(m_helperLog).contains(QStringLiteral("--hide swarm")));
}

void BackendTest::defaultsToInstalledScreensaver() {
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
    QVERIFY(!backend.busy());
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
    QTRY_VERIFY(backend.error().contains(QStringLiteral("exit code 17")));
    QTRY_VERIFY(!backend.busy());
    QVERIFY(!backend.playing());
}

void BackendTest::effectFailuresAreVisible() {
    makeExecutable(m_helper, QStringLiteral(
        "#!/bin/sh\n"
        "if [ \"$1\" = --list ]; then printf 'beams\\tBeams\\tenabled\\n'; exit 0; fi\n"
        "echo 'at least one effect must stay visible' >&2\nexit 1\n"));
    Backend backend;
    QTRY_COMPARE(backend.effects().size(), 1);
    backend.toggleHidden(QStringLiteral("beams"));
    QTRY_VERIFY(backend.error().contains(QStringLiteral("stay visible")));
    QVERIFY(!effectBySlug(backend, QStringLiteral("beams")).value(QStringLiteral("hidden")).toBool());
}

void BackendTest::effectRefreshRerunsAfterConcurrentToggle() {
    makeExecutable(m_helper, QStringLiteral(
        "#!/bin/sh\n"
        "state=\"$OMADROP_TEST_EFFECTS_STATE\"\n"
        "if [ \"$1\" = --list ]; then\n"
        "  status=enabled; [ -e \"$state\" ] && status=favorite\n"
        "  touch \"$state.listing\"\n"
        "  sleep 0.3\n"
        "  printf 'beams\\tBeams\\t%s\\n' \"$status\"\n"
        "else touch \"$state\"; fi\n"));
    Backend backend;
    QTRY_VERIFY(QFileInfo::exists(m_effectsState + ".listing"));
    backend.toggleFavorite(QStringLiteral("beams"));
    QTRY_VERIFY(QFileInfo::exists(m_effectsState));
    QTRY_VERIFY(effectBySlug(backend, QStringLiteral("beams")).value(QStringLiteral("favorite")).toBool());
}

void BackendTest::preferencesFailureIsVisible() {
    writeFile(m_configHome + "/omadrop", QStringLiteral("a file blocks the settings directory"));
    Backend backend;
    backend.setDisplay(QStringLiteral("single"));
    QVERIFY(backend.error().contains(QStringLiteral("settings")));
}

QTEST_GUILESS_MAIN(BackendTest)
#include "tst_backend.moc"
