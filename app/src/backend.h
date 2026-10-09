#pragma once

#include <QJsonArray>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QSet>
#include <QVariantList>

class QTimer;
class QLocalSocket;

// Native Omadrop product controller.
//
// One persistent process around the preserved renderers. It drives the legacy
// `omadrop` dispatcher (sibling of this executable, or OMADROP_CONTROLLER_BACKEND)
// without ever calling it bare or recursively, and watches Hyprland clients
// to learn when the session has actually mapped or gone away.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode NOTIFY stateChanged)
    Q_PROPERTY(QString display READ display NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(bool curtainVisible READ curtainVisible NOTIFY stateChanged)
    Q_PROPERTY(bool captions READ captions NOTIFY stateChanged)
    Q_PROPERTY(QString errorDetails READ errorDetails NOTIFY stateChanged)
    Q_PROPERTY(bool ascii READ ascii NOTIFY stateChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool milkdropAvailable READ milkdropAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool omarchyAvailable READ omarchyAvailable NOTIFY stateChanged)
    Q_PROPERTY(QVariantList scenes READ scenes NOTIFY scenesChanged)
    Q_PROPERTY(QVariantList worlds READ worlds NOTIFY worldsChanged)
    Q_PROPERTY(QString world READ world NOTIFY stateChanged)
    Q_PROPERTY(QString worldTitle READ worldTitle NOTIFY stateChanged)

public:
    explicit Backend(QObject* parent = nullptr);
    ~Backend() override;

    QString mode() const { return m_mode; }
    QString display() const { return m_display; }
    QString error() const { return m_error; }
    QString status() const { return m_status; }
    bool curtainVisible() const { return m_curtainVisible; }
    bool captions() const { return m_captions; }
    QString errorDetails() const { return m_errorDetails; }
    bool ascii() const { return m_ascii; }
    bool playing() const { return m_playing; }
    bool busy() const { return m_busy; }
    bool milkdropAvailable() const { return m_milkdropAvailable; }
    bool omarchyAvailable() const { return m_omarchyAvailable; }
    QVariantList scenes() const { return m_scenes; }
    QVariantList worlds() const { return m_worlds; }
    QString world() const { return m_world; }
    QString worldTitle() const;

    Q_INVOKABLE void setMode(const QString& mode);
    Q_INVOKABLE void setDisplay(const QString& display);
    Q_INVOKABLE void setCaptions(bool captions);
    Q_INVOKABLE void setAscii(bool ascii);
    Q_INVOKABLE void play();
    Q_INVOKABLE void playScene(int number);
    Q_INVOKABLE void setWorld(const QString& name);
    Q_INVOKABLE void playWorld(const QString& name);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggleSceneHidden(int number);
    Q_INVOKABLE void clearError();

    // Application-quit teardown: stop the session and wait a bounded time so no
    // renderer is orphaned. Never blocks the UI except during shutdown.
    void shutdown();

signals:
    void showControls();
    void stateChanged();
    void scenesChanged();
    void worldsChanged();
    void stopCompleted();

private:
    void loadPreferences();
    void persistPreferences();
    void loadScenes();
    void loadWorlds();
    QString currentThemeName() const;

    QString sceneManifestPath() const;
    QString scenesConfPath() const;
    QSet<int> readHiddenScenes() const;
    bool writeHiddenScenes(const QSet<int>& hidden);

    void beginSession(const QStringList& arguments, const QString& label);
    void launchController(const QStringList& arguments);
    void startPolling();
    void pollSession();
    void handleClients(const QByteArray& payload);
    void stopPolling();
    void cancelLaunch();
    void markCancelled();
    void runStopPass(bool finalPass);
    void finishStop();
    void sessionExited();
    void connectEvents();
    void boundProcess(QProcess* process, int timeoutMs);
    int countSessionWindows(const QJsonArray& clients) const;

    void setError(const QString& message);
    void failStart(const QString& message);

    QString m_mode;
    QString m_display;
    QString m_error;
    QString m_status;
    bool m_captions = true;
    bool m_curtainVisible = false;
    QString m_errorDetails;
    bool m_ascii = false;
    bool m_playing = false;
    bool m_busy = false;
    bool m_milkdropAvailable = false;
    bool m_omarchyAvailable = false;
    QVariantList m_scenes;
    QVariantList m_worlds;
    QString m_world;
    QString m_savedWorld;

    // Resolved paths.
    QString m_root;
    QString m_controllerPath;
    QString m_rendererPath;
    QString m_omarchyBackend;
    QString m_hyprctl;
    QString m_screensaverClass;
    QString m_productConf;
    QString m_modeConf;
    QString m_preferencesConf;
    QSet<int> m_hiddenScenes;

    // Session lifecycle.
    bool m_sessionSeen = false;
    QString m_pendingLabel;
    int m_startupTimeoutMs = 12000;
    int m_pollIntervalMs = 50;
    QTimer* m_pollTimer = nullptr;
    QTimer* m_startupTimer = nullptr;
    QPointer<QProcess> m_hyprProcess;
    QPointer<QProcess> m_launchProcess;
    QLocalSocket* m_events = nullptr;
    QByteArray m_eventBuffer;
    QSet<QByteArray> m_windowAddresses;
    bool m_pollPending = false;
    QPointer<QProcess> m_stopProcess;
    quint64 m_sessionGeneration = 0;
    bool m_stopping = false;
    bool m_cleanupNeeded = false;
    QString m_cancelFile;
    QSet<qint64> m_preexistingPids;
    int m_queryTimeoutMs = 1500;

};
