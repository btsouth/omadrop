#include "backend.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QLocalSocket>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSharedPointer>
#include <QTextStream>
#include <QTimer>
#include <QVariantMap>
#include <QUuid>

#include <signal.h>
#include <unistd.h>

#include <algorithm>

namespace {
// Runs a callback exactly once, whether the process finished or failed to
// start, so a missing helper binary still resolves a state instead of hanging.
template <typename Callback>
void onProcessSettled(QProcess* process, QObject* context, Callback callback) {
    auto handled = QSharedPointer<bool>::create(false);
    auto dispatch = [process, handled, callback](bool success, const QByteArray& output) {
        if (*handled) {
            return;
        }
        *handled = true;
        QString detail = QString::fromUtf8(process->readAllStandardError()).trimmed();
        if (process->property("omadropTimedOut").toBool()) detail = QStringLiteral("The command timed out.");
        if (detail.isEmpty() && !success) detail = process->errorString();
        process->deleteLater();
        callback(success, output, detail);
    };
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     context, [process, dispatch](int code, QProcess::ExitStatus status) {
                         dispatch(code == 0 && status == QProcess::NormalExit,
                                  process->readAllStandardOutput());
                     });
    QObject::connect(process, &QProcess::errorOccurred, context,
                     [dispatch](QProcess::ProcessError error) {
                         if (error == QProcess::FailedToStart) dispatch(false, QByteArray());
                     });
}

QString envOr(const char* name, const QString& fallback) {
    const QString value = qEnvironmentVariable(name);
    return value.isEmpty() ? fallback : value;
}

QString configHome() {
    return envOr("XDG_CONFIG_HOME", QDir::homePath() + QStringLiteral("/.config"));
}

bool parseBool(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    return normalized == QLatin1String("1") || normalized == QLatin1String("true");
}

bool validMode(const QString& value) {
    return value == QLatin1String("milkdrop") || value == QLatin1String("omarchy");
}

bool validDisplay(const QString& value) {
    return value == QLatin1String("single") || value == QLatin1String("all");
}

QMap<QString, QString> readKeyValues(const QString& path) {
    QMap<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return values;
    }
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const int separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }
        values.insert(line.left(separator).trimmed(), line.mid(separator + 1).trimmed());
    }
    return values;
}

// /proc/<pid>/exe resolves the exact renderer even when the dispatcher has
// already replaced itself with the real executable.
QString processExecutable(qint64 pid) {
    if (pid <= 0) {
        return {};
    }
    QFile link(QStringLiteral("/proc/%1/exe").arg(pid));
    QString target = link.symLinkTarget();
    const QString deleted = QStringLiteral(" (deleted)");
    if (target.endsWith(deleted)) {
        target.chop(deleted.size());
    }
    return target;
}

QString canonical(const QString& path) {
    if (path.isEmpty()) {
        return {};
    }
    const QString resolved = QFileInfo(path).canonicalFilePath();
    return resolved.isEmpty() ? path : resolved;
}
} // namespace

Backend::Backend(QObject* parent) : QObject(parent) {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString config = configHome();

    // omadrop-ui is installed in <root>/bin next to the other product
    // binaries; <root> is its parent and also holds presets/ and experiments/.
    m_root = QFileInfo(appDir).dir().absolutePath();

    m_controllerPath = envOr("OMADROP_CONTROLLER_BACKEND", appDir + QStringLiteral("/omadrop"));
    m_effectsHelper = envOr("OMADROP_EFFECTS_HELPER", appDir + QStringLiteral("/omadrop-effects"));
    m_effectsBinary = envOr("OMADROP_EFFECTS_BINARY", appDir + QStringLiteral("/ttfx-music"));
    m_rendererPath = envOr("OMADROP_MILKDROP_LIVE",
                           m_root + QStringLiteral("/experiments/projectm-ascii/projectm-ascii-live"));
    m_omarchyBackend = envOr("OMADROP_OMARCHY_BACKEND", appDir + QStringLiteral("/omadrop-screensaver"));
    m_hyprctl = envOr("OMADROP_HYPRCTL", QStringLiteral("hyprctl"));
    m_screensaverClass = envOr("OMADROP_SCREENSAVER_CLASS", QStringLiteral("org.omadrop.screensaver"));

    m_productConf = config + QStringLiteral("/omadrop/product.conf");
    m_modeConf = config + QStringLiteral("/omadrop/mode.conf");
    m_preferencesConf = config + QStringLiteral("/omadrop/preferences.conf");

    m_milkdropAvailable = QFileInfo(m_rendererPath).isExecutable();
    m_omarchyAvailable = QFileInfo(m_omarchyBackend).isExecutable();

    bool ok = false;
    const int pollInterval = qEnvironmentVariableIntValue("OMADROP_POLL_INTERVAL_MS", &ok);
    if (ok && pollInterval > 0) {
        m_pollIntervalMs = pollInterval;
    }
    const int startupTimeout = qEnvironmentVariableIntValue("OMADROP_STARTUP_TIMEOUT_MS", &ok);
    if (ok && startupTimeout > 0) {
        m_startupTimeoutMs = startupTimeout;
    }
    const int queryTimeout = qEnvironmentVariableIntValue("OMADROP_QUERY_TIMEOUT_MS", &ok);
    if (ok && queryTimeout > 0) {
        m_queryTimeoutMs = queryTimeout;
    }

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(m_pollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &Backend::pollSession);
    m_startupTimer = new QTimer(this);
    m_startupTimer->setSingleShot(true);
    connect(m_startupTimer, &QTimer::timeout, this, [this] {
        if (m_busy && !m_sessionSeen && !m_stopping) {
            failStart(QStringLiteral("The %1 did not start within %2 seconds. Check the installed backend and compositor connection.")
                          .arg(m_pendingLabel).arg(m_startupTimeoutMs / 1000.0, 0, 'g', 3));
        }
    });

    loadPreferences();
    m_hiddenScenes = readHiddenScenes();
    loadScenes();
    m_status = QStringLiteral("Ready");
    refreshEffects();
}

Backend::~Backend() {
    // waitForFinished can synchronously emit finished. Disable our callbacks
    // before teardown so a completed listing cannot spawn a description query
    // while the QObject children are being destroyed.
    for (QProcess* process : findChildren<QProcess*>()) {
        disconnect(process, nullptr, this, nullptr);
    }
    if (m_cleanupNeeded) shutdown();
    // Helpers can have grandchildren too (for example a hung query wrapper).
    for (QProcess* process : findChildren<QProcess*>()) {
        if (process->state() == QProcess::NotRunning) continue;
        const qint64 pid = process->processId();
        if (pid > 0) ::kill(-pid, SIGKILL);
        process->kill();
        process->waitForFinished(100);
    }
}

void Backend::setMode(const QString& mode) {
    if (!validMode(mode) || mode == m_mode) {
        return;
    }
    m_mode = mode;
    persistPreferences();
    emit stateChanged();
}

void Backend::setDisplay(const QString& display) {
    if (!validDisplay(display) || display == m_display) {
        return;
    }
    m_display = display;
    persistPreferences();
    emit stateChanged();
}

void Backend::setAscii(bool ascii) {
    if (ascii == m_ascii) {
        return;
    }
    m_ascii = ascii;
    persistPreferences();
    emit stateChanged();
}

void Backend::setCaptions(bool captions) {
    if (captions == m_captions) return;
    const auto saved = readKeyValues(m_preferencesConf);
    if (saved.value(QStringLiteral("version")).toUInt() > 4) {
        setError(QStringLiteral("Could not save captions: the settings format is newer."));
        return;
    }
    QDir().mkpath(QFileInfo(m_preferencesConf).absolutePath());
    QStringList lines;
    QFile input(m_preferencesConf);
    if (input.exists() && !input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(QStringLiteral("Could not read captions settings."));
        return;
    }
    if (input.isOpen()) {
        for (const QString& line : QString::fromUtf8(input.readAll()).split('\n')) {
            if (line.section('=', 0, 0).trimmed() != QLatin1String("captions") && !line.isEmpty())
                lines.append(line);
        }
    }
    if (!saved.contains(QStringLiteral("version"))) lines.prepend(QStringLiteral("version=4"));
    lines.append(QStringLiteral("captions=%1").arg(captions ? 1 : 0));
    QSaveFile file(m_preferencesConf);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)
        || file.write((lines.join('\n') + '\n').toUtf8()) < 0 || !file.commit()) {
        setError(QStringLiteral("Could not save captions settings."));
        return;
    }
    m_captions = captions;
    emit stateChanged();
}

void Backend::loadPreferences() {
    m_mode = !m_milkdropAvailable && m_omarchyAvailable
                 ? QStringLiteral("omarchy") : QStringLiteral("milkdrop");
    const bool firstRun = !QFileInfo::exists(m_productConf);
    m_display = firstRun ? QStringLiteral("single") : QStringLiteral("all");
    m_ascii = false;

    const QMap<QString, QString> product = readKeyValues(m_productConf);
    if (product.contains(QStringLiteral("mode")) && validMode(product.value(QStringLiteral("mode")))) {
        m_mode = product.value(QStringLiteral("mode"));
    } else {
        const QMap<QString, QString> saved = readKeyValues(m_modeConf);
        if (validMode(saved.value(QStringLiteral("mode")))) {
            m_mode = saved.value(QStringLiteral("mode"));
        }
    }

    const QMap<QString, QString> preferences = readKeyValues(m_preferencesConf);
    if (product.contains(QStringLiteral("display")) && validDisplay(product.value(QStringLiteral("display")))) {
        m_display = product.value(QStringLiteral("display"));
    } else if (!firstRun && validDisplay(preferences.value(QStringLiteral("display")))) {
        m_display = preferences.value(QStringLiteral("display"));
    }

    if (product.contains(QStringLiteral("ascii"))) {
        m_ascii = parseBool(product.value(QStringLiteral("ascii")));
    } else if (preferences.contains(QStringLiteral("ascii"))) {
        m_ascii = parseBool(preferences.value(QStringLiteral("ascii")));
    }
    m_captions = preferences.value(QStringLiteral("captions"), QStringLiteral("1")) != QLatin1String("0");
    const QString requestedMode = qEnvironmentVariable("OMADROP_UI_MODE");
    if (validMode(requestedMode)) {
        m_mode = requestedMode;
    }
}

void Backend::persistPreferences() {
    const QString directory = QFileInfo(m_productConf).absolutePath();
    if (!QDir().mkpath(directory)) {
        setError(QStringLiteral("Could not create the settings directory: %1").arg(directory));
        return;
    }
    QSaveFile file(m_productConf);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setError(QStringLiteral("Could not save settings: %1").arg(file.errorString()));
        return;
    }
    QTextStream stream(&file);
    stream << "mode=" << m_mode << '\n'
           << "display=" << m_display << '\n'
           << "ascii=" << (m_ascii ? 1 : 0) << '\n';
    stream.flush();
    if (!file.commit()) {
        setError(QStringLiteral("Could not save settings: %1").arg(file.errorString()));
    }
}

void Backend::clearError() {
    setError(QString());
}

void Backend::setError(const QString& message) {
    if (message == m_error) {
        return;
    }
    m_error = message;
    m_errorDetails.clear();
    if (!message.isEmpty()) {
        const QString state = envOr("XDG_STATE_HOME", QDir::homePath() + QStringLiteral("/.local/state"));
        QFile crash(state + QStringLiteral("/omadrop/last-crash.txt"));
        if (crash.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // Bound both the read and the disclosure to the end of the crash log.
            if (crash.size() > 16384) crash.seek(crash.size() - 16384);
            QStringList lines = QString::fromUtf8(crash.readAll()).trimmed().split('\n');
            m_errorDetails = lines.mid(qMax<qsizetype>(0, lines.size() - 20)).join('\n');
        }
    }
    emit stateChanged();
}

void Backend::beginSession(const QStringList& arguments, const QString& label) {
    if (m_stopping || m_busy || m_playing) {
        return;
    }
    ++m_sessionGeneration;
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
                            + QStringLiteral("/omadrop-ui");
    if (!QDir().mkpath(runtime)) {
        failStart(QStringLiteral("Could not create the session directory: %1").arg(runtime));
        return;
    }
    m_cancelFile = runtime + QStringLiteral("/session-%1.cancel")
                                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_sessionSeen = false;
    m_playing = false;
    m_busy = true;
    m_curtainVisible = true;
    m_pendingLabel = label;
    m_status = QStringLiteral("Starting…");
    emit stateChanged();

    if (!QFileInfo(m_controllerPath).isExecutable()) {
        failStart(QStringLiteral("The Omadrop launcher is missing: %1").arg(m_controllerPath));
        return;
    }
    m_cleanupNeeded = true;
    m_startupTimer->start(m_startupTimeoutMs);
    m_preexistingPids.clear();
    m_windowAddresses.clear();
    connectEvents();
    const quint64 generation = m_sessionGeneration;
    auto* snapshot = new QProcess(this);
    m_hyprProcess = snapshot;
    onProcessSettled(snapshot, this, [this, snapshot, arguments, generation](bool success, const QByteArray& payload, const QString&) {
        const bool current = m_hyprProcess == snapshot && generation == m_sessionGeneration && !m_stopping;
        if (m_hyprProcess == snapshot) m_hyprProcess = nullptr;
        if (!current) return;
        const QJsonDocument document = QJsonDocument::fromJson(payload);
        if (success && document.isArray()) {
            for (const QJsonValue& value : document.array()) {
                const qint64 pid = static_cast<qint64>(value.toObject().value(QStringLiteral("pid")).toDouble());
                if (pid > 0) m_preexistingPids.insert(pid);
            }
        }
        launchController(arguments);
        startPolling();
    });
    boundProcess(snapshot, m_queryTimeoutMs);
    snapshot->start(m_hyprctl, {QStringLiteral("clients"), QStringLiteral("-j")});
}

void Backend::failStart(const QString& message) {
    setError(message);
    stop();
}

void Backend::play() {
    clearError();
    QStringList arguments;
    arguments << QStringLiteral("--mode") << m_mode
              << (m_display == QLatin1String("single") ? QStringLiteral("--single")
                                                       : QStringLiteral("--all"));
    if (m_mode == QLatin1String("milkdrop")) {
        arguments << (m_ascii ? QStringLiteral("--ascii") : QStringLiteral("--no-ascii"));
    }
    m_previewing = false;
    beginSession(arguments, QStringLiteral("renderer"));
}

void Backend::playScene(int number) {
    clearError();
    if (number <= 0) {
        failStart(QStringLiteral("No scene was selected to play."));
        return;
    }
    QStringList arguments;
    arguments << QStringLiteral("--mode") << QStringLiteral("milkdrop")
              << QStringLiteral("--scene") << QString::number(number)
              << (m_display == QLatin1String("single") ? QStringLiteral("--single")
                                                       : QStringLiteral("--all"))
              << (m_ascii ? QStringLiteral("--ascii") : QStringLiteral("--no-ascii"));
    m_previewing = false;
    beginSession(arguments, QStringLiteral("renderer"));
}

void Backend::preview(const QString& slug) {
    clearError();
    if (slug.isEmpty()) {
        failStart(QStringLiteral("No effect was selected to preview."));
        return;
    }
    m_previewing = true;
    beginSession({QStringLiteral("--preview-effect"), slug}, QStringLiteral("preview"));
}

void Backend::stop() {
    if (m_stopping) {
        return;
    }
    ++m_sessionGeneration;
    m_stopping = true;
    m_curtainVisible = false;
    markCancelled();
    m_playing = false;
    m_busy = true;
    m_sessionSeen = false;
    m_status = QStringLiteral("Stopping…");
    stopPolling();
    cancelLaunch();
    emit stateChanged();
    emit showControls();
    // Cancel the dispatcher before sweeping mapped windows. Compositor exec
    // requests already submitted may map later, so run a second bounded sweep.
    runStopPass(false);
}

void Backend::shutdown() {
    ++m_sessionGeneration;
    m_stopping = true;
    markCancelled();
    stopPolling();
    const qint64 launchPid = m_launchProcess ? m_launchProcess->processId() : 0;
    cancelLaunch();
    if (m_launchProcess && !m_launchProcess->waitForFinished(400)) {
        const qint64 pid = m_launchProcess->processId();
        if (pid > 0) ::kill(-pid, SIGKILL);
        m_launchProcess->kill();
        m_launchProcess->waitForFinished(300);
    }
    // Children that ignored TERM may survive even when their parent has exited.
    if (launchPid > 0) ::kill(-launchPid, SIGKILL);
    if (m_stopProcess) {
        const qint64 pid = m_stopProcess->processId();
        if (pid > 0) ::kill(-pid, SIGKILL);
        m_stopProcess->kill();
        m_stopProcess->waitForFinished(300);
    }
    if (!m_cleanupNeeded || !QFileInfo(m_controllerPath).isExecutable()) {
        return;
    }
    // Normal IPC quit waits for asynchronous stopCompleted. This bounded path
    // covers closing controls or desktop/application shutdown during startup.
    QProcess process;
    process.setChildProcessModifier([] { ::setsid(); });
    process.start(m_controllerPath, {QStringLiteral("--stop")});
    if (!process.waitForFinished(4000)) {
        const qint64 pid = process.processId();
        if (pid > 0) ::kill(-pid, SIGKILL);
        process.kill();
        process.waitForFinished(300);
    }
    m_cleanupNeeded = false;
}

void Backend::cancelLaunch() {
    if (!m_launchProcess) return;
    const qint64 pid = m_launchProcess->processId();
    if (pid > 0) ::kill(-pid, SIGTERM);
    m_launchProcess->terminate();
    const QPointer<QProcess> process = m_launchProcess;
    QTimer::singleShot(400, this, [process, pid] {
        if (pid > 0) ::kill(-pid, SIGKILL);
        if (process && process->state() != QProcess::NotRunning) process->kill();
    });
}

void Backend::markCancelled() {
    if (m_cancelFile.isEmpty()) return;
    // Keep this marker until the runtime directory is cleared. A compositor
    // command queued before cancellation may not map until after we have quit.
    QFile marker(m_cancelFile);
    if (!marker.open(QIODevice::WriteOnly)) {
        setError(QStringLiteral("Could not cancel queued visuals: %1").arg(marker.errorString()));
    }
}

void Backend::runStopPass(bool finalPass) {
    if (!QFileInfo(m_controllerPath).isExecutable()) {
        finishStop();
        return;
    }
    auto* process = new QProcess(this);
    m_stopProcess = process;
    process->setChildProcessModifier([] { ::setsid(); });
    auto complete = [this, process, finalPass](bool failed) {
        if (m_stopProcess != process) return;
        m_stopProcess = nullptr;
        if (failed) {
            QString detail = QString::fromUtf8(process->readAllStandardError()).trimmed();
            if (detail.isEmpty()) detail = process->errorString();
            setError(QStringLiteral("Could not finish stopping the visuals: %1").arg(detail));
        }
        process->deleteLater();
        if (finalPass) {
            m_cleanupNeeded = failed;
            finishStop();
        }
        else QTimer::singleShot(1200, this, [this] { if (m_stopping) runStopPass(true); });
    };
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [complete](int code, QProcess::ExitStatus status) {
                complete(code != 0 || status != QProcess::NormalExit);
            });
    connect(process, &QProcess::errorOccurred, this, [complete](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) complete(true);
    });
    boundProcess(process, 4000);
    process->start(m_controllerPath, {QStringLiteral("--stop")});
}

void Backend::finishStop() {
    m_stopping = false;
    m_busy = false;
    m_status = QStringLiteral("Ready");
    emit stateChanged();
    emit showControls();
    emit stopCompleted();
}

void Backend::sessionExited() {
    ++m_sessionGeneration;
    markCancelled();
    stopPolling();
    m_curtainVisible = false;
    m_playing = false;
    m_sessionSeen = false;
    m_cleanupNeeded = false;
    finishStop();
}

void Backend::connectEvents() {
    if (m_events) m_events->deleteLater();
    m_eventBuffer.clear();
    m_events = new QLocalSocket(this);
    connect(m_events, &QLocalSocket::connected, this, [this] {
        if (m_playing) m_pollTimer->setInterval(1000);
    });
    connect(m_events, &QLocalSocket::disconnected, this, [this] {
        m_pollTimer->setInterval(m_pollIntervalMs);
    });
    connect(m_events, &QLocalSocket::readyRead, this, [this] {
        m_eventBuffer += m_events->readAll();
        int end;
        while ((end = m_eventBuffer.indexOf('\n')) >= 0) {
            const QByteArray line = m_eventBuffer.left(end);
            m_eventBuffer.remove(0, end + 1);
            if (line.startsWith("closewindow>>")) {
                const QByteArray address = line.mid(13);
                if (m_playing && m_windowAddresses.contains(address)) {
                    // The window is already gone. Restore controls before any
                    // asynchronous cleanup of siblings or queued launches.
                    stop();
                }
            } else if (line.startsWith("openwindow>>")) {
                m_pollPending = true;
                pollSession();
            }
        }
    });
    const QString path = envOr("OMADROP_HYPR_EVENT_SOCKET",
        qEnvironmentVariable("XDG_RUNTIME_DIR") + QStringLiteral("/hypr/")
        + qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE") + QStringLiteral("/.socket2.sock"));
    m_events->connectToServer(path, QIODevice::ReadOnly);
}

void Backend::launchController(const QStringList& arguments) {
    QProcess* process = new QProcess(this);
    m_launchProcess = process;
    const quint64 generation = m_sessionGeneration;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("OMADROP_CANCEL_FILE"), m_cancelFile);
    environment.insert(QStringLiteral("OMADROP_CAPTIONS"), m_captions ? QStringLiteral("1") : QStringLiteral("0"));
    process->setProcessEnvironment(environment);
    // Preserve an immediate --play dispatcher origin once. Later Play requests
    // begin a fresh measurement in their own dispatcher.
    qunsetenv("OMADROP_TIMING_ORIGIN_MS");
    if (qEnvironmentVariable("OMADROP_TIMING") == QLatin1String("1")) {
        process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    }
    process->setChildProcessModifier([] { ::setsid(); });
    connect(process, &QProcess::started, this, [this, process, generation] {
        if (generation != m_sessionGeneration || m_stopping) {
            const qint64 pid = process->processId();
            if (pid > 0) ::kill(-pid, SIGTERM);
            process->terminate();
            QTimer::singleShot(400, this, [pid] { if (pid > 0) ::kill(-pid, SIGKILL); });
        }
    });
    connect(process, &QProcess::errorOccurred, this, [this, process, generation](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) {
            return;
        }
        if (m_launchProcess == process) {
            m_launchProcess = nullptr;
        }
        process->deleteLater();
        if (generation == m_sessionGeneration && !m_stopping) {
            failStart(QStringLiteral("The Omadrop launcher could not start: %1").arg(m_controllerPath));
        }
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, process, generation](int exitCode, QProcess::ExitStatus status) {
                if (m_launchProcess == process) {
                    m_launchProcess = nullptr;
                }
                process->deleteLater();
                // Omarchy submits compositor exec requests and can exit before
                // mapping. MilkDrop owns its renderer until it exits.
                const bool failed = status == QProcess::CrashExit || exitCode != 0;
                if (generation == m_sessionGeneration && !m_stopping) {
                    if (failed && (!m_sessionSeen || m_mode == QLatin1String("omarchy"))) {
                        failStart(QStringLiteral("The visuals stopped unexpectedly."));
                    } else if (m_sessionSeen && m_mode == QLatin1String("milkdrop")) {
                        if (failed) setError(QStringLiteral("The visuals stopped unexpectedly."));
                        sessionExited();
                    }
                }
            });
    process->start(m_controllerPath, arguments);
}

void Backend::startPolling() {
    m_pollTimer->start(m_pollIntervalMs);
    pollSession();
}

void Backend::stopPolling() {
    m_pollTimer->stop();
    m_startupTimer->stop();
    m_pollPending = false;
    if (m_events) m_events->abort();
    if (m_hyprProcess) {
        const qint64 pid = m_hyprProcess->processId();
        if (pid > 0) ::kill(-pid, SIGKILL);
        m_hyprProcess->kill();
        m_hyprProcess = nullptr;
    }
}

void Backend::pollSession() {
    if (m_hyprProcess) {
        return;
    }
    m_pollPending = false;
    QProcess* process = new QProcess(this);
    m_hyprProcess = process;
    const quint64 generation = m_sessionGeneration;
    onProcessSettled(process, this, [this, process, generation](bool success, const QByteArray& payload, const QString& detail) {
        const bool current = m_hyprProcess == process && generation == m_sessionGeneration && !m_stopping;
        if (m_hyprProcess == process) m_hyprProcess = nullptr;
        if (!current) return;
        if (!success) {
            setError(QStringLiteral("Could not check the visual session: %1").arg(detail));
            return;
        }
        handleClients(payload);
        if (m_pollPending && !m_stopping) pollSession();
    });
    boundProcess(process, m_queryTimeoutMs);
    process->start(m_hyprctl, {QStringLiteral("clients"), QStringLiteral("-j")});
}

void Backend::handleClients(const QByteArray& payload) {
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (!document.isArray()) {
        setError(QStringLiteral("Could not check the visual session: the compositor returned invalid data."));
        return;
    }
    const QJsonArray clients = document.array();
    for (const QJsonValue& value : clients) {
        const QJsonObject client = value.toObject();
        if (!value.isObject() || !client.value(QStringLiteral("pid")).isDouble()
            || (!client.value(QStringLiteral("class")).isString()
                && !client.value(QStringLiteral("initialClass")).isString())) {
            setError(QStringLiteral("Could not check the visual session: the compositor returned invalid clients."));
            return;
        }
    }

    const int mapped = countSessionWindows(clients);
    if (mapped > 0) {
        m_windowAddresses.clear();
        for (const QJsonValue& value : clients) {
            if (countSessionWindows(QJsonArray{value}) > 0) {
                QByteArray address = value.toObject().value(QStringLiteral("address")).toString().toUtf8();
                if (address.startsWith("0x")) address.remove(0, 2);
                if (!address.isEmpty()) m_windowAddresses.insert(address);
            }
        }
        m_sessionSeen = true;
        m_startupTimer->stop();
        m_pollTimer->setInterval(m_events && m_events->state() == QLocalSocket::ConnectedState
            ? 1000 : m_pollIntervalMs);
        if (!m_playing) {
            m_playing = true;
            m_busy = false;
            m_curtainVisible = false;
            m_status = QStringLiteral("Playing");
            emit stateChanged();
        }
        return;
    }

    if (m_playing) {
        if (!m_launchProcess) sessionExited();
        else stop();
        return;
    }
}

// Counts windows that belong to *our* session. The screensaver class is ours;
// MilkDrop is matched by the exact renderer executable a window's PID points
// at. An unrelated window named Omadrop must never count as playback.
int Backend::countSessionWindows(const QJsonArray& clients) const {
    int count = 0;
    const QString renderer = canonical(m_rendererPath);
    for (const QJsonValue& value : clients) {
        const QJsonObject client = value.toObject();
        const qint64 pid = static_cast<qint64>(client.value(QStringLiteral("pid")).toDouble(0));
        if (m_preexistingPids.contains(pid)) continue;
        if (!client.value(QStringLiteral("mapped")).toBool(true)) {
            continue;
        }
        QString windowClass = client.value(QStringLiteral("class")).toString();
        if (windowClass.isEmpty()) {
            windowClass = client.value(QStringLiteral("initialClass")).toString();
        }
        if (windowClass == m_screensaverClass) {
            ++count;
            continue;
        }
        if (m_mode != QLatin1String("milkdrop")) {
            continue;
        }
        const QString executable = processExecutable(pid);
        if (!executable.isEmpty() && (executable == m_rendererPath || canonical(executable) == renderer)) {
            ++count;
            continue;
        }
    }
    return count;
}

void Backend::refreshEffects() {
    if (m_effectsRefreshing) {
        m_effectsDirty = true;
        return;
    }
    if (!QFileInfo(m_effectsHelper).isExecutable()) {
        setError(QStringLiteral("The effects helper is missing: %1").arg(m_effectsHelper));
        return;
    }
    m_effectsRefreshing = true;
    QProcess* process = new QProcess(this);
    m_effectsProcess = process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("OMADROP_EFFECTS_BINARY"), m_effectsBinary);
    process->setProcessEnvironment(environment);
    onProcessSettled(process, this, [this, process](bool success, const QByteArray& listing, const QString& detail) {
        if (m_effectsProcess == process) m_effectsProcess = nullptr;
        if (!success) {
            setError(QStringLiteral("Could not load effects: %1").arg(detail));
            finishEffectsRefresh();
            return;
        }
        m_pendingListing = listing;
        fetchEffectDescriptions();
    });
    boundProcess(process, m_queryTimeoutMs);
    process->start(m_effectsHelper, {QStringLiteral("--list")});
}

void Backend::fetchEffectDescriptions() {
    if (!QFileInfo(m_effectsBinary).isExecutable()) {
        setError(QStringLiteral("The effects binary is missing: %1").arg(m_effectsBinary));
        buildEffects(m_pendingListing, QByteArray());
        finishEffectsRefresh();
        return;
    }
    QProcess* process = new QProcess(this);
    m_effectsProcess = process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("OMADROP_EFFECTS_BINARY"), m_effectsBinary);
    process->setProcessEnvironment(environment);
    onProcessSettled(process, this, [this](bool success, const QByteArray& help, const QString& detail) {
        if (!success) setError(QStringLiteral("Could not load effect descriptions: %1").arg(detail));
        buildEffects(m_pendingListing, success ? help : QByteArray());
        finishEffectsRefresh();
    });
    boundProcess(process, m_queryTimeoutMs);
    process->start(m_effectsBinary, {QStringLiteral("--help")});
}

void Backend::buildEffects(const QByteArray& listing, const QByteArray& help) {
    QMap<QString, QString> descriptions;
    bool inCommands = false;
    for (const QByteArray& raw : help.split('\n')) {
        const QByteArray line = raw;
        if (line.startsWith("Commands:")) {
            inCommands = true;
            continue;
        }
        if (!inCommands) {
            continue;
        }
        if (!line.startsWith("  ")) {
            break;
        }
        const QByteArray trimmed = line.trimmed();
        const int space = trimmed.indexOf(' ');
        const QString slug = QString::fromUtf8(space < 0 ? trimmed : trimmed.left(space));
        if (slug.isEmpty() || slug == QLatin1String("help")) {
            continue;
        }
        descriptions.insert(slug, space < 0 ? QString() : QString::fromUtf8(trimmed.mid(space + 1)).trimmed());
    }

    QVariantList effects;
    for (const QByteArray& raw : listing.split('\n')) {
        const QString line = QString::fromUtf8(raw);
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const QStringList fields = line.split(QLatin1Char('\t'));
        if (fields.size() < 3) {
            continue;
        }
        const QString slug = fields.at(0).trimmed();
        if (slug.isEmpty()) {
            continue;
        }
        const QString name = fields.at(1).trimmed().isEmpty() ? slug : fields.at(1).trimmed();
        const QString status = fields.at(2).trimmed().toLower();
        QString description = descriptions.value(slug);
        if (description.isEmpty()) {
            description = name;
        }
        QVariantMap effect;
        effect.insert(QStringLiteral("slug"), slug);
        effect.insert(QStringLiteral("name"), name);
        effect.insert(QStringLiteral("description"), description);
        effect.insert(QStringLiteral("favorite"), status.contains(QLatin1String("favorite")));
        effect.insert(QStringLiteral("hidden"), status.contains(QLatin1String("hidden")));
        const QString thumbnail = QStringLiteral(":/assets/effects/%1.jpg").arg(slug);
        effect.insert(QStringLiteral("thumbnail"),
                      QFile::exists(thumbnail) ? QStringLiteral("qrc") + thumbnail : QString());
        effects.append(effect);
    }
    std::stable_sort(effects.begin(), effects.end(), [](const QVariant& left, const QVariant& right) {
        const auto a = left.toMap(), b = right.toMap();
        if (a.value("hidden").toBool() != b.value("hidden").toBool()) return !a.value("hidden").toBool();
        if (a.value("favorite").toBool() != b.value("favorite").toBool()) return a.value("favorite").toBool();
        return QString::compare(a.value("name").toString(), b.value("name").toString(), Qt::CaseInsensitive) < 0;
    });
    m_effects = effects;
    if (effects.isEmpty()) setError(QStringLiteral("The effects helper returned no usable effects."));
    emit effectsChanged();
}

void Backend::finishEffectsRefresh() {
    m_effectsProcess = nullptr;
    m_effectsRefreshing = false;
    if (m_effectsDirty) {
        m_effectsDirty = false;
        QTimer::singleShot(0, this, &Backend::refreshEffects);
    }
}

void Backend::boundProcess(QProcess* process, int timeoutMs) {
    process->setChildProcessModifier([] { ::setsid(); });
    auto* timer = new QTimer(process);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, process, [process] {
        if (process->state() == QProcess::NotRunning) return;
        process->setProperty("omadropTimedOut", true);
        const qint64 pid = process->processId();
        if (pid > 0) ::kill(-pid, SIGKILL);
        process->kill();
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), timer, &QTimer::stop);
    timer->start(timeoutMs);
}

void Backend::toggleFavorite(const QString& slug) {
    runEffectToggle({QStringLiteral("--favorite"), slug});
}

void Backend::toggleHidden(const QString& slug) {
    runEffectToggle({QStringLiteral("--hide"), slug});
}

void Backend::runEffectToggle(const QStringList& arguments) {
    if (arguments.size() < 2 || arguments.at(1).isEmpty()) {
        return;
    }
    if (!QFileInfo(m_effectsHelper).isExecutable()) {
        setError(QStringLiteral("The effects helper is missing: %1").arg(m_effectsHelper));
        return;
    }
    QProcess* process = new QProcess(this);
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("OMADROP_EFFECTS_BINARY"), m_effectsBinary);
    process->setProcessEnvironment(environment);
    onProcessSettled(process, this, [this](bool success, const QByteArray&, const QString& detail) {
        if (!success) setError(QStringLiteral("Could not update the effect: %1").arg(detail));
        refreshEffects();
    });
    boundProcess(process, m_queryTimeoutMs);
    process->start(m_effectsHelper, arguments);
}

QString Backend::sceneManifestPath() const {
    const QString override = qEnvironmentVariable("OMADROP_COLLECTION_MANIFEST");
    if (!override.isEmpty()) {
        return override;
    }
    const QString installed = m_root + QStringLiteral("/presets/collection-manifest.json");
    if (QFileInfo::exists(installed)) {
        return installed;
    }
    return m_root + QStringLiteral("/experiments/milkdrop-audio-pilot/manifest.json");
}

QString Backend::scenesConfPath() const {
    return configHome() + QStringLiteral("/omadrop/scenes.conf");
}

QSet<int> Backend::readHiddenScenes() const {
    QSet<int> hidden;
    QFile file(scenesConfPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return hidden;
    }
    QString hiddenValue;
    bool versionSeen = false;
    bool versionOk = false;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const int separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }
        const QString key = line.left(separator).trimmed();
        const QString value = line.mid(separator + 1).trimmed();
        if (key == QLatin1String("version")) {
            versionSeen = true;
            versionOk = value == QLatin1String("1");
        } else if (key == QLatin1String("hidden")) {
            hiddenValue = value;
        }
    }
    if (!versionSeen || !versionOk) {
        return hidden;
    }
    for (const QString& part : hiddenValue.split(QLatin1Char(','))) {
        bool ok = false;
        const int number = part.trimmed().toInt(&ok);
        if (ok && number > 0) {
            hidden.insert(number);
        }
    }
    return hidden;
}

bool Backend::writeHiddenScenes(const QSet<int>& hidden) {
    const QString path = scenesConfPath();
    const QString directory = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(directory)) {
        setError(QStringLiteral("Could not create the settings directory: %1").arg(directory));
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setError(QStringLiteral("Could not save hidden scenes: %1").arg(file.errorString()));
        return false;
    }
    QList<int> numbers = hidden.values();
    std::sort(numbers.begin(), numbers.end());
    QStringList parts;
    for (int number : numbers) {
        parts << QString::number(number);
    }
    QTextStream stream(&file);
    stream << "version=1\n"
           << "hidden=" << parts.join(QLatin1Char(',')) << '\n';
    stream.flush();
    if (!file.commit()) {
        setError(QStringLiteral("Could not save hidden scenes: %1").arg(file.errorString()));
        return false;
    }
    return true;
}

void Backend::loadScenes() {
    QVariantList scenes;
    QFile file(sceneManifestPath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        const QJsonArray presets = document.object().value(QStringLiteral("presets")).toArray();
        for (const QJsonValue& value : presets) {
            const QJsonObject preset = value.toObject();
            const int number = preset.value(QStringLiteral("number")).toInt();
            if (number <= 0) {
                continue;
            }
            const QString label = preset.value(QStringLiteral("label")).toString().trimmed();
            QVariantMap scene;
            scene.insert(QStringLiteral("number"), number);
            scene.insert(QStringLiteral("label"),
                         label.isEmpty() ? QStringLiteral("Scene %1").arg(number) : label);
            scene.insert(QStringLiteral("description"),
                         preset.value(QStringLiteral("appearance")).toString().trimmed());
            scene.insert(QStringLiteral("hidden"), m_hiddenScenes.contains(number));
            scene.insert(QStringLiteral("thumbnail"),
                         QStringLiteral("qrc:/assets/scenes/collection-%1.jpg")
                             .arg(number, 2, 10, QLatin1Char('0')));
            scenes.append(scene);
        }
    }
    std::stable_sort(scenes.begin(), scenes.end(), [](const QVariant& left, const QVariant& right) {
        const auto a = left.toMap(), b = right.toMap();
        if (a.value("hidden").toBool() != b.value("hidden").toBool()) return !a.value("hidden").toBool();
        return QString::compare(a.value("label").toString(), b.value("label").toString(), Qt::CaseInsensitive) < 0;
    });
    m_scenes = scenes;
    emit scenesChanged();
}

void Backend::toggleSceneHidden(int number) {
    if (number <= 0) {
        return;
    }
    QSet<int> hidden = readHiddenScenes();
    if (hidden.contains(number)) {
        hidden.remove(number);
    } else {
        hidden.insert(number);
    }
    if (!writeHiddenScenes(hidden)) {
        return;
    }
    m_hiddenScenes = hidden;
    loadScenes();
}
