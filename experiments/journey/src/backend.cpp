#include "backend.h"
#include "world.h"
#include "glcore.h"

#include <QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

JourneyBackend::JourneyBackend(Journey::AudioTimeline audio, Journey::Score score, double duration,
                               double at, bool paused, int seed, QObject* parent)
    : QObject(parent), audio_(std::move(audio)), score_(std::move(score)), duration_(duration),
      seconds_(at), paused_(paused), seed_(seed) {
    resetClock();
}

void JourneyBackend::resetClock() { origin_ = seconds_; clock_.restart(); }

double JourneyBackend::seconds() {
    if (!paused_) {
        seconds_ = std::min(duration_, origin_ + clock_.nsecsElapsed() / 1e9);
        if (seconds_ >= duration_) paused_ = true;
    }
    return seconds_;
}

void JourneyBackend::togglePause() {
    seconds();
    paused_ = !paused_;
    resetClock();
    emit changed();
}

void JourneyBackend::scrub(double delta) {
    if (!std::isfinite(delta)) return;
    seconds();
    seconds_ = std::clamp(seconds_ + delta, 0.0, duration_);
    resetClock();
    emit changed();
}

namespace {
class FboRenderer : public QQuickFramebufferObject::Renderer {
public:
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
        return new QOpenGLFramebufferObject(size, format);
    }
    void synchronize(QQuickFramebufferObject* item) override {
        auto* journey = static_cast<JourneyItem*>(item);
        if (JourneyBackend* b = journey->backend()) {
            t_ = b->seconds();
            audio_ = b->audioAt(t_);
            score_ = b->score();
            seed_ = b->seed();
        }
    }
    void render() override {
        if (!world_) {
            world_ = std::make_unique<Journey::World>(seed_);
            QString error;
            if (!world_->init(error)) { QTextStream(stderr) << error << '\n'; return; }
            QTextStream(stderr) << "GL: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << '\n';
            timer_.start();
        }
        world_->setScore(score_);
        QOpenGLFramebufferObject* fbo = framebufferObject();
        world_->render(fbo->width(), fbo->height(), t_, audio_);
        world_->gpu().present(fbo->handle());
        QQuickOpenGLUtils::resetOpenGLState();
        // Report the delivered frame rate every two seconds.
        ++frames_;
        if (timer_.elapsed() > 2000) {
            QTextStream(stderr) << QString::asprintf("native %dx%d  %.1f fps  t=%.1f\n", fbo->width(), fbo->height(),
                frames_ * 1000.0 / timer_.elapsed(), t_);
            frames_ = 0;
            timer_.restart();
        }
        update();
    }
private:
    std::unique_ptr<Journey::World> world_;
    double t_ = 0;
    Journey::Audio audio_;
    const Journey::Score* score_ = nullptr;
    int seed_ = 1, frames_ = 0;
    QElapsedTimer timer_;
};
}

JourneyItem::JourneyItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setMirrorVertically(true);
    // Advance the clock every presented frame: synchronize() only runs when
    // the item is updated from the GUI thread.
    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow* w) {
        if (w) connect(w, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
    });
}

void JourneyItem::setBackend(JourneyBackend* backend) {
    if (backend_ == backend) return;
    backend_ = backend;
    emit backendChanged();
    update();
}

QQuickFramebufferObject::Renderer* JourneyItem::createRenderer() const { return new FboRenderer; }
