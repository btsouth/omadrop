#pragma once

#include "audio.h"
#include <QElapsedTimer>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QTimer>

class JourneyBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(double seconds READ seconds NOTIFY changed)
    Q_PROPERTY(bool paused READ paused NOTIFY changed)
public:
    JourneyBackend(Journey::AudioTimeline audio, double duration, double at, bool paused,
                   QObject* parent = nullptr);
    double seconds() const { return seconds_; }
    bool paused() const { return paused_; }
    Journey::Audio audio() const { return audio_.at(seconds_); }
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void scrub(double delta);
signals:
    void changed();
private:
    void tick();
    void resetClock();
    Journey::AudioTimeline audio_;
    double duration_, seconds_, origin_ = 0;
    bool paused_;
    QElapsedTimer clock_;
    QTimer timer_;
};

class JourneyItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(JourneyBackend* backend READ backend WRITE setBackend NOTIFY backendChanged)
public:
    explicit JourneyItem(QQuickItem* parent = nullptr);
    JourneyBackend* backend() const { return backend_; }
    void setBackend(JourneyBackend* backend);
    void paint(QPainter* painter) override;
signals:
    void backendChanged();
private:
    QPointer<JourneyBackend> backend_;
};
