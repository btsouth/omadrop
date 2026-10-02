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

// Native Omadrop product controller.
//
// One persistent process around the preserved renderers. It drives the legacy
// `omadrop` dispatcher (sibling of this executable, or OMADROP_CONTROLLER_BACKEND)
// without ever calling it bare or recursively, discovers effects through the
// `omadrop-effects` helper, and watches Hyprland clients to learn when the
// session has actually mapped or gone away.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode NOTIFY stateChanged)
    Q_PROPERTY(QString display READ display NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(bool ascii READ ascii NOTIFY stateChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool milkdropAvailable READ milkdropAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool omarchyAvailable READ omarchyAvailable NOTIFY stateChanged)
    Q_PROPERTY(QVariantList effects READ effects NOTIFY effectsChanged)

public:
    explicit Backend(QObject* parent = nullptr);
    ~Backend() override;

    QString mode() const { return m_mode; }
    QString display() const { return m_display; }
    QString error() const { return m_error; }
    QString status() const { return m_status; }
    bool ascii() const { return m_ascii; }
    bool playing() const { return m_playing; }
    bool busy() const { return m_busy; }
    bool milkdropAvailable() const { return m_milkdropAvailable; }
    bool omarchyAvailable() const { return m_omarchyAvailable; }
    QVariantList effects() const { return m_effects; }

    Q_INVOKABLE void setMode(const QString& mode);
    Q_INVOKABLE void setDisplay(const QString& display);
    Q_INVOKABLE void setAscii(bool ascii);
    Q_INVOKABLE void play();
    Q_INVOKABLE void preview(const QString& slug);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggleFavorite(const QString& slug);
    Q_INVOKABLE void toggleHidden(const QString& slug);
    Q_INVOKABLE void clearError();

    // Application-quit teardown: stop the session and wait a bounded time so no
    // renderer is orphaned. Never blocks the UI except during shutdown.
    void shutdown();

signals:
    void showControls();
    void stateChanged();
    void effectsChanged();
    void stopCompleted();

private:
    void loadPreferences();
    void persistPreferences();
    void refreshEffects();
    void fetchEffectDescriptions();
    void buildEffects(const QByteArray& listing, const QByteArray& help);
    void runEffectToggle(const QStringList& arguments);

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
    void finishEffectsRefresh();
    void boundProcess(QProcess* process, int timeoutMs);
    int countSessionWindows(const QJsonArray& clients) const;

    void setError(const QString& message);
    void failStart(const QString& message);

    QString m_mode;
    QString m_display;
    QString m_error;
    QString m_status;
    bool m_ascii = false;
    bool m_playing = false;
    bool m_busy = false;
    bool m_milkdropAvailable = false;
    bool m_omarchyAvailable = false;
    QVariantList m_effects;

    // Resolved paths.
    QString m_controllerPath;
    QString m_effectsHelper;
    QString m_effectsBinary;
    QString m_rendererPath;
    QString m_hyprctl;
    QString m_screensaverClass;
    QString m_productConf;
    QString m_modeConf;
    QString m_preferencesConf;

    // Session lifecycle.
    bool m_sessionSeen = false;
    bool m_previewing = false;
    QString m_pendingLabel;
    int m_startupTimeoutMs = 12000;
    int m_pollIntervalMs = 300;
    QTimer* m_pollTimer = nullptr;
    QTimer* m_startupTimer = nullptr;
    QPointer<QProcess> m_hyprProcess;
    QPointer<QProcess> m_launchProcess;
    QPointer<QProcess> m_stopProcess;
    quint64 m_sessionGeneration = 0;
    bool m_stopping = false;
    bool m_cleanupNeeded = false;
    QString m_cancelFile;
    QSet<qint64> m_preexistingPids;
    int m_queryTimeoutMs = 1500;

    // Effects discovery.
    QByteArray m_pendingListing;
    QPointer<QProcess> m_effectsProcess;
    bool m_effectsRefreshing = false;
    bool m_effectsDirty = false;
};
