// Offline wave lab. QCoreApplication + surfaceless EGL, never a desktop window.
#include "kit/wave-train.h"
#include "kit/gradient-sky.h"
#include "kit/water-surface.h"
#include "audio.h"
#include "headless.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <fstream>
#include <numeric>
#include <iostream>
using namespace Journey;using namespace Journey::Kit;
namespace {
Col color(QJsonValue v) {bool ok=false;const auto n=v.toString().mid(1).toUInt(&ok,16);if(!ok)throw std::runtime_error("invalid lab color");return hex(n);}
QJsonObject settings(const QJsonObject& world,const QString& id) {
    for(auto stage:world["stages"].toArray())for(auto slot:stage.toObject()["slots"].toArray())
        if(slot.toObject()["id"].toString()==id)return slot.toObject()["params"].toObject();
    throw std::runtime_error("missing lab background slot");
}
struct Background {
    GradientSkyParametersV1 sky;WaterSurfaceParametersV1 sea;
    explicit Background(const QString& path) {
        QFile f(path+"/scene.json");if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("cannot read Kanagawa background");
        QJsonParseError error;auto doc=QJsonDocument::fromJson(f.readAll(),&error);
        if(error.error!=QJsonParseError::NoError)throw std::runtime_error("invalid background JSON");
        auto p=settings(doc.object(),"sky");
        for(auto v:p["stops"].toArray()){auto o=v.toObject();sky.stops.push_back({o["y"].toDouble(),color(o["color"])});}
        if(sky.stops.size()<2||sky.stops.size()>8)throw std::runtime_error("invalid sky stops");
        sky.paperTop=color(p["paperTop"]);sky.paperBottom=color(p["paperBottom"]);
        sky.printGrade=p["printGrade"].toDouble();sky.grain=p["grain"].toDouble();sky.energyGrade=p["energyGrade"].toDouble();sky.cloudBands=p["cloudBands"].toBool();
        p=settings(doc.object(),"sea");
#define NUM(key) if(p.contains(#key))sea.key=p[#key].toDouble()
        NUM(horizon);NUM(nearY);NUM(rows);NUM(textureRows);NUM(glints);NUM(sampleStep);
        NUM(amplitude);NUM(wavelength);NUM(drift);NUM(capScale);NUM(bandGain);NUM(liftGain);
        NUM(kickGain);NUM(capDensity);NUM(innerLines);NUM(crestOpacity);NUM(swellSeed);NUM(amplitudeGain);
#undef NUM
#define COL(key) if(p.contains(#key))sea.key=color(p[#key])
        COL(top);COL(bottom);COL(crest);COL(texture);COL(foam);COL(underprint);COL(glint);COL(hotGlint);
#undef COL
        sea.surgeEnabled=p["surgeEnabled"].toBool();
    }
};
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);QCommandLineParser parser;parser.addHelpOption();
    parser.addOptions({{"out","Evidence directory.","directory"},{"mode","sweep, travel, response, clip or bench.","mode","sweep"},
        {"fixture","Stereo f32 44100 Hz; clip and bench consume first 60 s causally.","file"},
        {"background","Kanagawa folder.","folder",KANAGAWA_FOLDER}});parser.process(app);
    const QString dir=parser.value("out"),mode=parser.value("mode");
    if(dir.isEmpty()||!(mode=="sweep"||mode=="travel"||mode=="response"||mode=="clip"||mode=="bench"))return 2;
    QDir().mkpath(dir);
    try {
        Background background(parser.value("background"));HeadlessContext context;Gpu gpu;QString error;
        if(!context.create(error)||!gpu.init(error)){std::cerr<<error.toStdString();return 1;}
        std::cerr<<context.renderer().toStdString()<<'\n';
        if(mode=="bench" && !context.renderer().contains("UHD Graphics 770"))throw std::runtime_error("benchmark requires devbox UHD 770");
        std::vector<std::unique_ptr<Canvas>> canvases;Score score;Audio audio;WaveTrainParametersV2 params;
        QProcess encoder;const bool clip=mode=="clip",bench=mode=="bench";
        if(clip) {
            encoder.setProcessChannelMode(QProcess::ForwardedErrorChannel);
            encoder.start("ffmpeg",{"-nostdin","-v","error","-y","-f","rawvideo","-pix_fmt","rgb24","-s","1920x1080","-r","30","-i","-",
                "-f","f32le","-ar","44100","-ac","2","-i",parser.value("fixture"),"-t","60","-c:v","libx264","-preset","veryfast","-crf","20",
                "-pix_fmt","yuv420p","-c:a","aac","-b:a","192k","-movflags","+faststart",dir+"/music-60s-1080p30.mp4"});
            if(!encoder.waitForStarted(5000))throw std::runtime_error("ffmpeg did not start");
        }
        GLuint timer=0;glGenQueries(1,&timer);std::vector<unsigned char> rgb;std::vector<double> costs,cpuCosts,geometryCosts;double pendingMotionMs=0;
        std::ofstream csv((dir+"/"+mode+"-parameters.csv").toStdString());
        csv<<"seconds,height,stage,lean,lip_stage,base_width,phase_speed,throw,distance,group_center,energy,bass_level,tempo,hero_a,hero_envelope,hero_stage,hero_height,lip_count,piece_gpu_ms,piece_cpu_ms,geometry_paint_ms,motion_ms,camera_x,lip_screen_min_x,lip_screen_max_x\n";
        auto frame=[&](const WaveTrainPoseV2& s,const QString& png,int index) {
            gpu.begin(1920,1080);Ctx c{gpu,s.seconds,audio,&score,1,&canvases,nullptr};
            if(!bench){GradientSkyV1::draw(c,background.sky);WaterSurfaceV1::draw(c,background.sea);}
            const auto cpuStart=std::chrono::steady_clock::now();
            auto f=WaveTrainV2::profile(s,params);Canvas& cv=c.canvas();
            const double cameraX=clip && f.hero>=0?f.crests[f.hero].a-630:0;
            cv.save();cv.translate(-cameraX,0);WaveTrainV2::paint(cv,f,s,params);cv.restore();
            const double geometryMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-cpuStart).count();
            const double cpuMs=geometryMs+pendingMotionMs;
            // Warm the shader/upload path before starting the isolated query.
            glBeginQuery(GL_TIME_ELAPSED,timer);gpu.over(cv);glEndQuery(GL_TIME_ELAPSED);
            GLuint64 ns=0;glGetQueryObjectui64v(timer,GL_QUERY_RESULT,&ns);const double ms=ns/1e6;
            if(index>=30){costs.push_back(ms);cpuCosts.push_back(cpuMs);geometryCosts.push_back(geometryMs);}
            double heroA=-1,heroG=0,heroStage=0;int lips=0;
            if(f.hero>=0){heroA=f.crests[f.hero].a;heroG=f.crests[f.hero].envelope;heroStage=f.crests[f.hero].stage;}
            for(const auto& crest:f.crests)if(!crest.outerLip.empty())++lips;
            csv<<s.seconds<<','<<s.amplitude<<','<<s.stage<<','<<s.lean<<','<<s.lipStage<<','<<s.baseWidth<<','<<s.phaseSpeed<<','<<s.lipThrow<<','<<s.distance<<','<<params.groupOrigin+.5*s.distance
                <<','<<s.energy<<','<<audio.bassLevel<<','<<s.tempo<<','<<heroA<<','<<heroG<<','<<heroStage<<','<<s.amplitude*heroG<<','<<lips<<','<<ms<<','<<cpuMs<<','<<geometryMs<<','<<pendingMotionMs<<','<<cameraX<<',';
            double minX=1e9,maxX=-1e9;
            if(f.hero>=0)for(const auto& q:f.crests[f.hero].outerLip){minX=std::min(minX,q.x-cameraX);maxX=std::max(maxX,q.x-cameraX);}
            csv<<minX<<','<<maxX<<'\n';
            pendingMotionMs=0;
            if(!bench) {
                FinishParams finish;finish.bloom=finish.vignette=finish.grain=0;finish.paper=.08;finish.time=s.seconds;
                gpu.finish(finish,nullptr);gpu.readRgb(rgb);
                if(!png.isEmpty() && !QImage(rgb.data(),1920,1080,1920*3,QImage::Format_RGB888).save(png))throw std::runtime_error("cannot save frame");
                if(clip) {
                    if(encoder.write(reinterpret_cast<const char*>(rgb.data()),rgb.size())<0)throw std::runtime_error("encoder write failed");
                    while(encoder.bytesToWrite()>0)if(!encoder.waitForBytesWritten(10000))throw std::runtime_error("encoder stalled");
                }
            }
        };
        if(mode=="sweep")for(int i=0;i<12;++i) {
            WaveTrainPoseV2 s;s.amplitude=800;s.stage=lerp(0.,5.,i/11.);s.seconds=12;s.energy=.85;s.flow=2;
            WaveTrainFoamMotionV2 details;Audio input;Score sc;input.bands.fill(.45);
            for(int j=0;j<720;++j){s.seconds=(j+1)/60.;sc.advance(input,s.seconds,1/60.);details.advance(input,sc,s,s.seconds,1/60.);}
            sc.bassHits.push_back({s.seconds,1,12345});details.advance(input,sc,s,s.seconds,1/60.);
            frame(s,dir+QString("/sweep-%1.png").arg(i,2,10,QChar('0')),i);
        }
        if(mode=="response") {
            std::ofstream report((dir+"/element-response.csv").toStdString());
            report<<"condition,band,length_min_px,length_max_px,angle_min_rad,angle_max_rad,droplets_alive,foam_thickness_px,wobble_peak_to_peak_px,whitecap_depth_px,contour_drift_px,clumps,fingers_per_clump_min,fingers_per_clump_max,blue_gap_fraction,tip_tendrils,forward_tip_fraction,down_tip_fraction\n";
            for(int loud=0;loud<2;++loud) {
                WaveTrainPoseV2 s;s.amplitude=800;s.stage=5;s.energy=loud?.90:.04;
                Audio input;Score sc;WaveTrainFoamMotionV2 details;
                for(int i=0;i<750;++i) {
                    const double now=(i+1)/60.;s.seconds=now;s.flow=(.065+.16*s.energy)*now;
                    input.bands.fill(loud?.42:.018);
                    if(i%24==0){input.bands.fill(loud?.65:.026);if(loud)sc.bassHits.push_back({now,1,unsigned(i+1)});}
                    sc.advance(input,now,1/60.);details.advance(input,sc,s,now,1/60.);
                }
                frame(s,dir+(loud?"/s5-loud.png":"/s5-quiet.png"),loud);
                const auto f=WaveTrainV2::profile(s,params);const auto& hero=f.crests[f.hero];
                int alive=0;for(const auto& d:s.droplets)if(d.life>0&&d.age<d.life)++alive;
                std::vector<V2> low=hero.boundary,high=low;double wobble=0;
                for(int i=0;i<600;++i) {
                    auto q=s;q.seconds=i/30.;auto v=WaveTrainV2::profile(q,params).crests[f.hero].boundary;
                    for(size_t k=0;k<v.size();++k){low[k].x=std::min(low[k].x,v[k].x);low[k].y=std::min(low[k].y,v[k].y);high[k].x=std::max(high[k].x,v[k].x);high[k].y=std::max(high[k].y,v[k].y);}
                }
                for(size_t k=0;k<low.size();++k)wobble=std::max(wobble,(high[k]-low[k]).len());
                double capDepth=0;for(const auto& cap:hero.whitecaps)for(size_t i=0;i<cap.edge.size();++i)capDepth=std::max(capDepth,(cap.edge[i]-cap.inside[i]).len());
                auto next=s;next.flow+=(.065+.16*s.energy)*2;auto nf=WaveTrainV2::profile(next,params);double drift=0;
                for(size_t i=0;i<hero.contours[5].size();++i)drift=std::max(drift,(hero.contours[5][i]-nf.crests[nf.hero].contours[5][i]).len());
                int minF=99,maxF=0,tendrils=0,main=0,forward=0,down=0;
                for(const auto& clump:hero.clumps){minF=std::min(minF,clump.count);maxF=std::max(maxF,clump.count);}
                for(const auto& finger:hero.fingers)if(finger.tendril)++tendrils;else {
                    ++main;if(std::cos(finger.angle)>0)++forward;
                    if((finger.tip-finger.centre[finger.centre.size()-2]).y>0)++down;
                }
                for(int band=0;band<6;++band) {
                    double lo=1e9,hi=0,al=1e9,ah=-1e9;for(const auto& finger:hero.fingers)if(finger.band==band){lo=std::min(lo,finger.length);hi=std::max(hi,finger.length);al=std::min(al,finger.angle);ah=std::max(ah,finger.angle);}
                    report<<(loud?"loud":"quiet")<<','<<band<<','<<lo<<','<<hi<<','<<al<<','<<ah<<','<<alive<<','<<hero.foamThickness<<','<<wobble<<','<<capDepth<<','<<drift<<','<<hero.clumps.size()<<','<<minF<<','<<maxF<<','<<hero.gapFraction<<','<<tendrils<<','<<forward/double(main)<<','<<down/double(main)<<'\n';
                }
                // Native full frames establish silhouette/lip motion at frozen stage.
                for(int i=0;i<5;++i){auto q=s;q.seconds=i*2.;q.flow=(.065+.16*s.energy)*q.seconds;frame(q,dir+QString("/%1-motion-%2.png").arg(loud?"loud":"quiet").arg(i),i);}
            }
        }
        if(mode=="travel")for(int i=0;i<30;++i) {
            WaveTrainPoseV2 s;s.amplitude=800;s.stage=5;s.phaseSpeed=88;s.lipThrow=40;
            s.seconds=i;s.distance=88*i;s.energy=.8;s.flow=.19*i;
            frame(s,dir+QString("/travel-%1.png").arg(i,2,10,QChar('0')),i);
        }
        if(clip||bench) {
            QFile fixture(parser.value("fixture"));if(!fixture.open(QIODevice::ReadOnly)||fixture.size()<60ll*44100*8)throw std::runtime_error("fixture needs >=60 s of stereo f32");
            StreamingAudio analyzer;WaveTrainMotionV2 motion;
            for(int i=0;i<3600;++i) {
                auto pcm=fixture.read(735*8);if(pcm.size()!=735*8)throw std::runtime_error("short PCM hop");
                const double now=(i+1)/60.;analyzer.push(reinterpret_cast<const float*>(pcm.constData()),735,[&](const Audio& a,double dt){
                    audio=a;score.advance(a,now,dt);
                    const auto start=std::chrono::steady_clock::now();motion.advance(a,score,now,dt);
                    pendingMotionMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
                });
                if(i%2==1)frame(motion.pose(),clip && ((i+1)%120==0||i==1)?dir+QString("/music-%1.png").arg((i+1)/60,2,10,QChar('0')):QString{},i/2);
            }
        }
        if(clip){encoder.closeWriteChannel();if(!encoder.waitForFinished(30000)||encoder.exitCode()!=0)throw std::runtime_error("encoding failed");}
        glDeleteQueries(1,&timer);
        if(!costs.empty()) {
            std::sort(costs.begin(),costs.end());std::sort(cpuCosts.begin(),cpuCosts.end());std::ofstream out((dir+"/gpu.txt").toStdString());
            out<<"renderer="<<context.renderer().toStdString()<<"\n1080p isolated piece, upload + draws + layer composite; excludes background, finish, readback and encoding\n"
                <<"CPU includes 2 causal detail/body controller hops per frame plus geometry/paint; excludes shared analyzer and Score\n"
                <<"warmup_frames=30\nsamples="<<costs.size()<<"\nmean_ms="<<std::accumulate(costs.begin(),costs.end(),0.)/costs.size()
                <<"\np50_ms="<<costs[costs.size()/2]<<"\np95_ms="<<costs[int(costs.size()*.95)]<<"\nmax_ms="<<costs.back()<<"\ncpu_mean_ms="<<std::accumulate(cpuCosts.begin(),cpuCosts.end(),0.)/cpuCosts.size()<<"\ncpu_geometry_mean_ms="<<std::accumulate(geometryCosts.begin(),geometryCosts.end(),0.)/geometryCosts.size()<<"\ncpu_p95_ms="<<cpuCosts[int(cpuCosts.size()*.95)]<<"\ncpu_max_ms="<<cpuCosts.back()<<'\n';
        }
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
