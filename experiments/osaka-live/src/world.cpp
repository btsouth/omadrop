#include "world.h"
namespace Journey {
void World::setGeometryCacheEnabled(bool enabled) {
    if (enabled != cacheGeometry_) {
        gpu_.clearGeometryCache();
        staticGeometry_.canvases.clear();
        staticGeometry_.points.clear();
        staticGeometry_.layouts.clear();
    }
    cacheGeometry_ = enabled;
    gpu_.setGeometryCacheEnabled(enabled);
}
void World::render(int w,int h,double t,const Audio& audio,const Score& score,const Schedule& schedule) {
    if (w != cacheWidth_ || h != cacheHeight_) {
        gpu_.clearGeometryCache();
        staticGeometry_.canvases.clear();
        staticGeometry_.points.clear();
        staticGeometry_.layouts.clear();
        cacheWidth_ = w; cacheHeight_ = h;
    }
    gpu_.begin(w,h);
    Ctx c{gpu_,t,audio,&score,seed_,&canvases_,&schedule,cacheGeometry_ ? &staticGeometry_ : nullptr};
    drawOsaka(c,{});
}
}
