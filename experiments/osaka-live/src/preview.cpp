#include "preview.h"
#include "world.h"
#include <QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QTextStream>
#include <memory>

Journey::LiveSession* OsakaItem::session=nullptr;
double OsakaItem::fixedScale=0;
namespace {
class Renderer:public QQuickFramebufferObject::Renderer {
public:
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override {
        return new QOpenGLFramebufferObject(size);
    }
    void render() override {
        if(!world_) {
            world_=std::make_unique<Journey::World>(1);
            if(OsakaItem::fixedScale>0) world_->setScale(OsakaItem::fixedScale);
            QString error;
            if(!world_->init(error)) { QTextStream(stderr)<<error<<'\n'; return; }
        }
        const auto f=OsakaItem::session->snapshot();
        auto* target=framebufferObject();
        world_->render(target->width(),target->height(),f.seconds,f.audio,f.score,f.schedule);
        world_->gpu().present(target->handle());
        QQuickOpenGLUtils::resetOpenGLState();
        update();
    }
private:
    std::unique_ptr<Journey::World> world_;
};
}
QQuickFramebufferObject::Renderer* OsakaItem::createRenderer() const { return new ::Renderer; }
