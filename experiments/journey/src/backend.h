#pragma once

#include "audio.h"
#include "score.h"
#include <QElapsedTimer>
#include <QPointer>
#include <QQuickFramebufferObject>

class JourneyBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(double seconds READ seconds NOTIFY changed)
    Q_PROPERTY(bool paused READ paused NOTIFY changed)
public:
    JourneyBackend(Journey::AudioTimeline audio, Journey::Score score, double duration, double at,
                   bool paused, int seed, QObject* parent = nullptr);
    double seconds();
    bool paused() const { return paused_; }
    Journey::Audio audioAt(double t) const { return audio_.at(t); }
    const Journey::Score* score() const { return &score_; }
    int seed() const { return seed_; }
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void scrub(double delta);
signals:
    void changed();
private:
    void resetClock();
    Journey::AudioTimeline audio_;
    Journey::Score score_;
    double duration_, seconds_, origin_ = 0;
    bool paused_;
    int seed_;
    QElapsedTimer clock_;
};

// Native preview: the same GPU renderer draws into Qt Quick's framebuffer.
class JourneyItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(JourneyBackend* backend READ backend WRITE setBackend NOTIFY backendChanged)
public:
    explicit JourneyItem(QQuickItem* parent = nullptr);
    JourneyBackend* backend() const { return backend_; }
    void setBackend(JourneyBackend* backend);
    Renderer* createRenderer() const override;
signals:
    void backendChanged();
private:
    QPointer<JourneyBackend> backend_;
};
