#pragma once
#include "gpu.h"
#include "score.h"
#include "schedule.h"
#include "resolution.h"
#include <chrono>
#include <QString>
#include <array>
#include <any>

namespace Journey {
struct StaticGeometry {
    using Key = std::pair<std::string, std::vector<double>>;
    std::map<Key, Canvas> canvases;
    std::map<Key, std::vector<V2>> points;
    std::uint64_t builds = 0;
    std::map<std::string,std::any> layouts;
};
struct Ctx {
    Gpu& gpu;
    double t;
    Audio a;
    const Score* score;
    int seed;
    std::vector<std::unique_ptr<Canvas>>* canvases;
    const Schedule* schedule;
    StaticGeometry* staticGeometry = nullptr;
    int next=0;
    double cameraX=0;
    Canvas& canvas() {
        if(std::size_t(next)>=canvases->size()) canvases->push_back(std::make_unique<Canvas>());
        auto& canvas=*(*canvases)[next++];
        canvas.reset(gpu.pixelScale());canvas.translate(-cameraX,0);return canvas;
    }

    // Builders contain only time/audio-independent geometry. Parameters name
    // the camera/state values baked into a span; resolution is invalidated by World.
    Canvas* retainedBuilder(Canvas& target, const std::string& name,
                            std::initializer_list<double> parameters = {}) {
        if (!staticGeometry) return &target;
        std::vector<double> values(parameters);
        values.push_back(cameraX);
        auto result = staticGeometry->canvases.try_emplace(StaticGeometry::Key{name, values});
        auto& retained = result.first->second;
        if (result.second) {
            retained.reset(gpu.pixelScale());
            retained.translate(-cameraX, 0);
            retained.freeze();
            ++staticGeometry->builds;
        }
        // A new span is empty until its builder runs, so append even when empty.
        target.append(retained);
        return result.second ? &retained : nullptr;
    }
    template<class Builder>
    void retain(Canvas& target, const std::string& name, Builder build,
                std::initializer_list<double> parameters = {}) {
        if (Canvas* retained = retainedBuilder(target, name, parameters)) build(*retained);
    }
    template<class Builder>
    const std::vector<V2>& points(const std::string& name, Builder build,
                                 std::initializer_list<double> parameters = {}) {
        if (!staticGeometry) { temporaryPoints = build(); return temporaryPoints; }
        auto result = staticGeometry->points.try_emplace(
            StaticGeometry::Key{name, std::vector<double>(parameters)});
        if (result.second) result.first->second = build();
        return result.first->second;
    }
    std::vector<V2> temporaryPoints;
    double band(int i) const { return a.bands[std::clamp(i,0,5)]; }
    double lift(int i) const {
        const double m=score?score->mean(std::clamp(i,0,5),t):0;
        return std::clamp((band(i)-m)/(m+0.035),0.0,1.6);
    }
    double kick(double d=6) const { return score?Score::envelope(score->bassHits,t,d):0; }
    double hit(double d=8) const { return score?Score::envelope(score->onsets,t,d):0; }
    double jit(double k) const { return hash2(k,seed*7.13+0.5)*2-1; }
    double gesture(double a,double in,double b,double out) const { return schedule->gesture(t,a,in,b,out); }
};
struct OsakaState {
    double cam=5, drift=0, land=1, fog=0, fogTop=600;
    double moonDx=0, moonDy=0, moonWarm=0,wisteria=1;
    bool chapter=true;
    double outAlpha=1,harbour=1,reflection=1,harbourFeather=0;
};
struct Surge { double t=-1,strength=0; bool fallback=false; };
void drawOsaka(Ctx&,const OsakaState&);
void drawSignGlyph(Canvas&,int,double,double,double,Col,double alpha=1);
class World {
public:
    explicit World(int seed):seed_(seed) {}
    ~World();
    bool init(QString& error);
    void setScale(double fixed) { resolution_.fixed(fixed); }
    double scale() const { return renderedScale_; }
    double gpuMilliseconds() const { return gpuMs_; }
    void render(int width,int height,double time,const Audio&,const Score&,const Schedule&);
    Gpu& gpu() { return gpu_; }
    void setGeometryCacheEnabled(bool enabled);
    std::uint64_t staticBuilds() const { return staticGeometry_.builds; }
    std::size_t staticSpanCount() const { return staticGeometry_.canvases.size(); }
private:
    Gpu gpu_;
    Resolution resolution_;
    struct TimerQuery { GLuint id=0; bool pending=false; };
    std::array<TimerQuery,4> queries_{};
    unsigned queryIndex_=0;
    bool gpuTiming_=false;
    double renderedScale_=1,gpuMs_=-1;
    std::chrono::steady_clock::time_point sampledAt_{};
    int seed_;
    std::vector<std::unique_ptr<Canvas>> canvases_;
    StaticGeometry staticGeometry_;
    bool cacheGeometry_ = true;
    int cacheWidth_ = 0, cacheHeight_ = 0;
};
}
