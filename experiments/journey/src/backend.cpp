#include "backend.h"
#include <algorithm>
#include <cmath>
#include <utility>

JourneyBackend::JourneyBackend(Journey::AudioTimeline audio, double duration, double at,
                               bool paused, QObject* parent)
    : QObject(parent), audio_(std::move(audio)), duration_(duration), seconds_(at), paused_(paused) {
    resetClock();
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(16);
    connect(&timer_, &QTimer::timeout, this, &JourneyBackend::tick);
    timer_.start();
}

void JourneyBackend::resetClock() { origin_ = seconds_; clock_.restart(); }

void JourneyBackend::tick() {
    if (paused_) return;
    seconds_ = std::min(duration_, origin_ + clock_.nsecsElapsed() / 1e9);
    if (seconds_ >= duration_) paused_ = true;
    emit changed();
}

void JourneyBackend::togglePause() {
    const bool next = !paused_;
    if (!paused_) tick();
    paused_ = next;
    resetClock();
    emit changed();
}

void JourneyBackend::scrub(double delta) {
    if (!std::isfinite(delta)) return;
    if (!paused_) tick();
    seconds_ = std::clamp(seconds_ + delta, 0.0, duration_);
    resetClock();
    emit changed();
}

JourneyItem::JourneyItem(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setOpaquePainting(true);
    setFillColor(Qt::black);
}

void JourneyItem::setBackend(JourneyBackend* backend) {
    if (backend_ == backend) return;
    if (backend_) disconnect(backend_, nullptr, this, nullptr);
    backend_ = backend;
    if (backend_) connect(backend_, &JourneyBackend::changed, this, [this] { update(); });
    emit backendChanged();
    update();
}

void JourneyItem::paint(QPainter* painter) {
    if (!backend_) return;
    painter->setRenderHint(QPainter::Antialiasing);
    Journey::paint(*painter, QSize(qRound(width()), qRound(height())),
                   backend_->seconds(), backend_->audio());
}
