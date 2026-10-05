#pragma once
#include "glcore.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace Journey {
// Opt-in timestamp pairs coexist with World's GL_TIME_ELAPSED query.
// Two frame banks are reused only after their final timestamp is available.
// A busy bank skips measurement, never waits or overwrites an in-flight query.
class GpuProfile {
    struct Pass { GLuint q[2]{}; std::string name,format; int w=0,h=0,taps=0; float sigma=0; };
    struct Bank { std::vector<Pass> passes; std::size_t count=0; bool pending=false;
        unsigned long long frame=0; double seconds=0; bool show=false;
        unsigned fullscreen=0,binds=0,switches=0; };
    std::array<Bank,2> banks_{};
    FILE* file_=nullptr; Bank* active_=nullptr; unsigned next_=0;
    unsigned long long frame_=0,skipped_=0;
    GLuint read_=0,draw_=0;
public:
    std::string group="frame";
    bool enabled() const { return file_; }
    void init() {
        const char* path=std::getenv("OSAKA_GPU_PROFILE");
        if(!path || !*path) return;
        GLint bits=0; glGetQueryiv(GL_TIMESTAMP,GL_QUERY_COUNTER_BITS,&bits);
        if(!bits) { std::fprintf(stderr,"GPU profile: timestamps unavailable\n"); return; }
        file_=std::fopen(path,"w");
        if(!file_) { std::perror("GPU profile CSV"); return; }
        std::fprintf(file_,"frame,seconds,full_firework_show,group,pass,width,height,format,sigma_px,taps,gpu_ms,fullscreen,fb_binds,fb_switches\n");
    }
    void collect(Bank& b) {
        if(!b.pending) return;
        GLint ready=0; glGetQueryObjectiv(b.passes[0].q[1],GL_QUERY_RESULT_AVAILABLE,&ready);
        if(!ready) return;
        for(std::size_t i=0;i<b.count;++i) {
            const auto& p=b.passes[i]; GLuint64 a=0,z=0;
            glGetQueryObjectui64v(p.q[0],GL_QUERY_RESULT,&a);
            glGetQueryObjectui64v(p.q[1],GL_QUERY_RESULT,&z);
            const auto sep=p.name.find('/');
            std::fprintf(file_,"%llu,%.9f,%d,%s,%s,%d,%d,%s,%.6f,%d,%.9f,%u,%u,%u\n",
                b.frame,b.seconds,int(b.show),p.name.substr(0,sep).c_str(),p.name.substr(sep+1).c_str(),
                p.w,p.h,p.format.c_str(),p.sigma,p.taps,double(z-a)/1e6,
                i==0?b.fullscreen:0,i==0?b.binds:0,i==0?b.switches:0);
        }
        b.pending=false;
    }
    int start(const std::string& name,int w,int h,const char* format,float sigma=0,int taps=0) {
        if(!active_) return -1;
        auto& b=*active_; const int i=int(b.count++);
        if(b.passes.size()<=std::size_t(i)) { b.passes.emplace_back(); glGenQueries(2,b.passes.back().q); }
        auto& p=b.passes[i];p.name=group+"/"+name;p.w=w;p.h=h;p.format=format;p.sigma=sigma;p.taps=taps;
        glQueryCounter(p.q[0],GL_TIMESTAMP); return i;
    }
    void stop(int i) { if(i>=0) glQueryCounter(active_->passes[i].q[1],GL_TIMESTAMP); }
    void begin(double seconds,bool show,int w,int h) {
        if(!file_) return;
        auto& b=banks_[next_];collect(b);++frame_;
        if(b.pending) { ++skipped_;return; }
        b.count=0;b.frame=frame_;b.seconds=seconds;b.show=show;
        b.fullscreen=b.binds=b.switches=0;active_=&b;group="frame";
        start("total",w,h,"mixed");
    }
    void end() {
        if(!active_) return;
        stop(0);active_->pending=true;active_=nullptr;next_=(next_+1)%2;
    }
    void framebuffer(GLenum target,GLuint fbo) {
        if(!active_) return;
        ++active_->binds; bool changed=false;
        if(target!=GL_DRAW_FRAMEBUFFER) {changed|=read_!=fbo;read_=fbo;}
        if(target!=GL_READ_FRAMEBUFFER) {changed|=draw_!=fbo;draw_=fbo;}
        active_->switches+=changed;
    }
    void fullscreen() { if(active_) ++active_->fullscreen; }
    void close() {
        if(!file_) return;
        unsigned pending=0;for(auto& b:banks_) {collect(b);pending+=b.pending;for(auto& p:b.passes)glDeleteQueries(2,p.q);}
        std::fprintf(stderr,"GPU profile: %llu frames, %llu skipped, %u pending banks at close\n",frame_,skipped_,pending);
        std::fclose(file_);file_=nullptr;
    }
    struct Scope {
        GpuProfile& p;int i;
        Scope(GpuProfile& p,const std::string& name,int w,int h,const char* format,float sigma=0,int taps=0):p(p),i(p.start(name,w,h,format,sigma,taps)) {}
        ~Scope() {p.stop(i);}
    };
    struct Group {
        GpuProfile& p;std::string previous;
        Group(GpuProfile& p,const char* name):p(p) {if(p.enabled()){previous=p.group;p.group=name;}}
        ~Group(){if(p.enabled())p.group=previous;}
    };
};
}
