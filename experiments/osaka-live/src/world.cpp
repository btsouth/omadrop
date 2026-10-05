#include "world.h"
#include <QElapsedTimer>
#include <cmath>
namespace Journey {
World::~World() { for(auto& q:queries_) if(q.id) glDeleteQueries(1,&q.id); }
bool World::init(QString& error) {
    if(!gpu_.init(error)) return false;
    GLint bits=0; glGetQueryiv(GL_TIME_ELAPSED,GL_QUERY_COUNTER_BITS,&bits);
    gpuTiming_=bits>0;
    if(gpuTiming_) for(auto& q:queries_) glGenQueries(1,&q.id);
    sampledAt_=std::chrono::steady_clock::now();
    return true;
}
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
    const auto now=std::chrono::steady_clock::now();
    const double dt=std::chrono::duration<double>(now-sampledAt_).count();
    auto& query=queries_[queryIndex_];
    if(query.pending) {
        GLint ready=0; glGetQueryObjectiv(query.id,GL_QUERY_RESULT_AVAILABLE,&ready);
        if(ready) {
            GLuint64 ns=0; glGetQueryObjectui64v(query.id,GL_QUERY_RESULT,&ns);
            gpuMs_=ns/1e6; resolution_.sample(gpuMs_,dt);
            sampledAt_=now; query.pending=false;
        }
    }
    const bool measure=gpuTiming_ && !query.pending;
    renderedScale_=resolution_.scale();
    const int outputW=w,outputH=h;
    w=std::max(1,int(std::lround(w*renderedScale_)));
    h=std::max(1,int(std::lround(h*renderedScale_)));
    QElapsedTimer cpu; cpu.start();
    gpu_.profile.begin(t,schedule.fullFireworkShow,w,h);
    if(measure) glBeginQuery(GL_TIME_ELAPSED,query.id);
    if (w != cacheWidth_ || h != cacheHeight_) {
        gpu_.clearGeometryCache();
        staticGeometry_.canvases.clear();
        staticGeometry_.points.clear();
        staticGeometry_.layouts.clear();
        cacheWidth_ = w; cacheHeight_ = h;
    }
    gpu_.setOutputSize(outputW,outputH);
    gpu_.begin(w,h);
    Ctx c{gpu_,t,audio,&score,seed_,&canvases_,&schedule,cacheGeometry_ ? &staticGeometry_ : nullptr};
    drawOsaka(c,{});
    gpu_.profile.end();
    if(measure) {
        glEndQuery(GL_TIME_ELAPSED); query.pending=true;
        queryIndex_=(queryIndex_+1)%queries_.size();
    } else if(!gpuTiming_) {
        resolution_.sample(cpu.nsecsElapsed()/1e6,dt); sampledAt_=now;
    }
}
}
