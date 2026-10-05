#include "preview.h"
#include "world.h"
#include <QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <atomic>
#include <memory>

Journey::LiveSession* OsakaItem::session=nullptr;
double OsakaItem::fixedScale=0;
int OsakaItem::fixedFps=0;
QString OsakaItem::statsPath;
namespace {
class Renderer:public QQuickFramebufferObject::Renderer {
public:
    ~Renderer() override { QObject::disconnect(swapped_); }
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override {
        forceFrame_=true;
        return new QOpenGLFramebufferObject(size);
    }
    void synchronize(QQuickFramebufferObject* item) override {
        if(window_) return;
        window_=item->window();clock_.start();
        if(!OsakaItem::statsPath.isEmpty()) {
            static std::atomic_int ordinal{0};const int n=ordinal++;
            stats_.setFileName(OsakaItem::statsPath+(n ? "."+QString::number(n) : QString()));
            if(stats_.open(QIODevice::WriteOnly|QIODevice::Truncate))
                stats_.write("wall_seconds,scene_seconds,new_content,target_fps,scale,gpu_ms\n");
        }
        swapped_=QObject::connect(window_,&QQuickWindow::frameSwapped,window_,[this] {
            if(stats_.isOpen() && world_) {
                const auto line=QString::asprintf("%.9f,%.9f,%d,%d,%.3f,%.6f\n",clock_.nsecsElapsed()/1e9,
                    sceneSeconds_,int(newContent_),world_->fps(),world_->scale(),world_->gpuMilliseconds());
                stats_.write(line.toUtf8());
            }
        },Qt::DirectConnection);
    }
    void render() override {
        if(!world_) {
            world_=std::make_unique<Journey::World>(1);
            if(OsakaItem::fixedScale>0) world_->setScale(OsakaItem::fixedScale);
            if(OsakaItem::fixedFps>0) world_->setFps(OsakaItem::fixedFps);
            QString error;
            if(!world_->init(error)) { QTextStream(stderr)<<error<<'\n'; return; }
            QTextStream(stderr)<<"GPU: "<<reinterpret_cast<const char*>(glGetString(GL_RENDERER))<<"; requested swap interval "<<window_->format().swapInterval()<<'\n';
        }
        newContent_=pacer_.tick(clock_.nsecsElapsed()/1e9,world_->fps()) || forceFrame_;
        forceFrame_=false;
        if(newContent_) {
            const auto f=OsakaItem::session->snapshot();sceneSeconds_=f.seconds;
            auto* target=framebufferObject();
            world_->render(target->width(),target->height(),f.seconds,f.audio,f.score,f.schedule);
            world_->gpu().present(target->handle());
        }
        QQuickOpenGLUtils::resetOpenGLState();
        update();
    }
private:
    std::unique_ptr<Journey::World> world_;
    Journey::FramePacer pacer_;
    QQuickWindow* window_=nullptr;
    QMetaObject::Connection swapped_;
    QElapsedTimer clock_;
    QFile stats_;
    double sceneSeconds_=0;
    bool newContent_=false,forceFrame_=true;
};
}
QQuickFramebufferObject::Renderer* OsakaItem::createRenderer() const { return new ::Renderer; }
