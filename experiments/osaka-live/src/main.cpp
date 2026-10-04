#include "session.h"
#include "world.h"
#include "headless.h"
#include "preview.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QTimer>
#include <chrono>
#include <thread>
#include <time.h>
#include <cmath>
#include <random>
#include <limits>

int main(int argc,char** argv) {
    bool headless=false;
    for(int i=1;i<argc;++i) if(QString::fromLocal8Bit(argv[i])=="--record"
        || QString::fromLocal8Bit(argv[i])=="--probe" || QString::fromLocal8Bit(argv[i])=="--bench"
        || QString::fromLocal8Bit(argv[i])=="--verify-render") headless=true;
    QSurfaceFormat format; format.setVersion(3,3); format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL); // Wayland EGL on NVIDIA defaults to OpenGL ES
    QSurfaceFormat::setDefaultFormat(format);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    std::unique_ptr<QCoreApplication> app;
    if(headless) app=std::make_unique<QCoreApplication>(argc,argv);
    else app=std::make_unique<QGuiApplication>(argc,argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Osaka Jade live milestone. Existing system capture; no file analysis or film chapters.");
    parser.addHelpOption();
    parser.addOptions({{"single","Play on one display and remember the choice."},
        {"all","Play on every display and remember the choice."},{"record","Real-time headless capture to MP4.","path"},
        {"probe","Measure streaming response without rendering."},
        {"bench","Measure rendering without encoding or framebuffer readback."},
        {"uncapped","Run a bounded throughput benchmark without frame pacing."},
        {"verify-render","Compare optimized drawing with accepted canvas output."},
        {"seconds","Bounded recording/probe length; preview loops when omitted.","number","60"},
        {"fps","Recording frames per second.","number","30"},
        {"width","Frame width.","number","1920"},{"height","Frame height.","number","1080"},
        {"scale","Internal resolution scale: auto, or 0.5 to 1.0.","value"},
        {"seed","Schedule seed (live default random; headless default 1).","number"},
        {"stats","Per-frame response and timing CSV.","path"}});
    parser.process(*app);
    bool ok=false;
    const double seconds=parser.value("seconds").toDouble(&ok);
    if(!ok || !std::isfinite(seconds) || seconds<=0 || seconds>600) return 2;
    const int fps=parser.value("fps").toInt(&ok);
    if(!ok || fps<1 || fps>60) return 2;
    const int w=parser.value("width").toInt(&ok);
    if(!ok || w<320 || w>3840) return 2;
    const int h=parser.value("height").toInt(&ok);
    if(!ok || h<180 || h>2160) return 2;
    const QString scaleOption=parser.isSet("scale") ? parser.value("scale") : qEnvironmentVariable("OMADROP_OSAKA_SCALE","auto");
    double fixedScale=0;
    if(scaleOption!="auto") {
        fixedScale=scaleOption.toDouble(&ok);
        if(!ok || !std::isfinite(fixedScale) || fixedScale<0.5 || fixedScale>1) return 2;
    }
    OsakaItem::fixedScale=fixedScale;
    int seed=1;
    if(parser.isSet("seed")) {
        seed=parser.value("seed").toInt(&ok); if(!ok || seed<0) return 2;
    } else if(!headless) seed=int(std::random_device{}() & 0x7fffffff);
    QTextStream(stderr)<<"Schedule seed: "<<seed<<'\n';
    Journey::LiveSession session(seed);
    if(!headless) {
        OsakaItem::session=&session;
        qmlRegisterType<OsakaItem>("Osaka",1,0,"OsakaItem");
        QQmlApplicationEngine engine;
        const QString config=QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        QSettings preferences(config+"/omadrop/preferences.conf",QSettings::IniFormat);
        QString display=preferences.value("display","all").toString();
        if(parser.isSet("single")) display="single";
        if(parser.isSet("all")) display="all";
        // Preserve MilkDrop's forward-version guard when saving display choice.
        if((parser.isSet("single") || parser.isSet("all"))
            && preferences.value("version",0).toInt()<=4) {
            preferences.setValue("version",4); preferences.setValue("display",display);
            preferences.sync();
        }
        QGuiApplication::setDesktopFileName(qEnvironmentVariable("OMADROP_SCREENSAVER_CLASS",
                                                                "org.omadrop.screensaver"));
        const auto screens=QGuiApplication::screens();
        const int count=display=="single"?std::min(1,int(screens.size())):int(screens.size());
        for(int i=0;i<count;++i) {
            engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
            if(engine.rootObjects().size()!=i+1) return 1;
            auto* window=qobject_cast<QQuickWindow*>(engine.rootObjects().back());
            if(!window) return 1;
            window->setScreen(screens[i]);
            window->setPosition(screens[i]->geometry().topLeft());
            window->showFullScreen();
        }
        if(!count) return 1;
        if(parser.isSet("seconds")) QTimer::singleShot(int(seconds*1000),app.get(),&QCoreApplication::quit);
        return app->exec();
    }
    Journey::HeadlessContext context;
    Journey::World world(1);
    if(fixedScale>0) world.setScale(fixedScale);
    if(parser.isSet("verify-render")) {
        world.setScale(1);
        QString error;
        if(!context.create(error) || !world.init(error)) { QTextStream(stderr)<<error<<'\n'; return 1; }
        int pose=0;
        const std::uint64_t accepted[]={0xaa567941bb8e05b6ull,0x467f84c75c484f98ull,0x741e31c9603638ebull};
        for(double t:{8.0,14.0,28.0}) {
            Journey::LiveFrame f;
            f.schedule.advance(t,{},f.score);
            f.schedule.fireworks=t-1.4;
            f.schedule.fireworkStrength=0.8;
            f.schedule.finale={{t-0.4,0.8,1}};
            std::vector<unsigned char> before,after;
            world.setGeometryCacheEnabled(false);
            Journey::Canvas::useKnownConvex=false;
            world.render(w,h,t,f.audio,f.score,f.schedule); world.gpu().readRgb(before);
            world.setGeometryCacheEnabled(true);
            Journey::Canvas::useKnownConvex=true;
            world.render(w,h,t,f.audio,f.score,f.schedule); world.gpu().readRgb(after);
            if(before!=after) {
                std::size_t changed=0; int maximum=0;
                for(std::size_t i=0;i<before.size();++i) {
                    if(before[i]!=after[i] && changed<8) QTextStream(stderr)<<"diff pixel "<<(i/3)%w<<","<<(i/3)/w<<" channel "<<i%3<<" "<<int(before[i])<<"/"<<int(after[i])<<"\n";
                    changed+=before[i]!=after[i]; maximum=std::max(maximum,std::abs(int(before[i])-int(after[i])));
                }
                QTextStream(stderr)<<"canvas image differs: "<<changed<<" channels, max "<<maximum<<'\n';
                return 1;
            }
            std::uint64_t hash=14695981039346656037ull;
            for(auto byte:after) { hash^=byte; hash*=1099511628211ull; }
            QTextStream(stdout)<<t<<" RGB hash "<<QString::number(hash,16)<<'\n';
            if(w==1920 && h==1080 && context.renderer().contains("RTX 4070 SUPER") && hash!=accepted[pose]) {QTextStream(stderr)<<"accepted M1 RGB hash differs\n";return 1;}
            ++pose;
        }
        QTextStream(stdout)<<"PASS: retained geometry and optimized canvas match uncached RGB exactly at 8, 14 and 28 seconds\n";
        return 0;
    }
    const bool record=parser.isSet("record");
    const bool render=record || parser.isSet("bench");
    QString error;
    if(render && (!context.create(error) || !world.init(error))) { QTextStream(stderr)<<error<<'\n'; return 1; }
    if(render) QTextStream(stderr)<<"GPU: "<<context.renderer()<<"; OpenGL "<<reinterpret_cast<const char*>(glGetString(GL_VERSION))<<'\n';
    QProcess encoder;
    if(record) {
        encoder.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        encoder.start("ffmpeg",{"-nostdin","-v","error","-y","-f","rawvideo","-pix_fmt","rgb24",
            "-s",QString::asprintf("%dx%d",w,h),"-r",QString::number(fps),"-i","-",
            "-an","-c:v","libx264","-preset","veryfast","-crf","18","-pix_fmt","yuv420p",
            "-movflags","+faststart",parser.value("record")});
        if(!encoder.waitForStarted(5000)) return 1;
    }
    QFile stats(parser.value("stats"));
    if(parser.isSet("stats") && !stats.open(QIODevice::WriteOnly|QIODevice::Truncate)) return 1;
    QTextStream csv(&stats);
    if(stats.isOpen()) csv<<"seconds,render_ms,submit_ms,thread_cpu_ms,gain,bass,accent,surge,band0,band1,band2,band3,band4,band5,onsets,bass_hits,mid_peaks,firework_at,finale_count,train_age,cyclist_age,static_builds,static_uploads,vertex_upload_bytes,pre_gain_level,full_firework_show,train_cycle,cyclist_cycle,train_speed,cyclist_speed,combinations,scale,gpu_ms\n";
    const auto start=std::chrono::steady_clock::now();
    std::vector<unsigned char> rgb;
    const int frames=int(std::ceil(seconds*fps));
    const bool uncapped=parser.isSet("uncapped") && parser.isSet("bench");
    int renderedFrames=0;
    double maxMs=0,totalMs=0;
    for(int i=0;uncapped ? std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<seconds : i<frames;++i) {
        ++renderedFrames;
        const auto deadline=start+std::chrono::microseconds(std::llround(i*1e6/fps));
        if(!uncapped)std::this_thread::sleep_until(deadline);
        const auto f=session.snapshot();
        timespec cpuStart{},cpuEnd{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&cpuStart);
        const auto previousBytes=world.gpu().geometryStats().vertexBytes;
        QElapsedTimer timer; timer.start();
        double submit=0;
        if(render) {
            world.render(w,h,f.seconds,f.audio,f.score,f.schedule);
            submit=timer.nsecsElapsed()/1e6;
            glFinish();
        }
        const double ms=timer.nsecsElapsed()/1e6;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID,&cpuEnd);
        const double cpuMs=(cpuEnd.tv_sec-cpuStart.tv_sec)*1000.0+(cpuEnd.tv_nsec-cpuStart.tv_nsec)/1e6;
        maxMs=std::max(maxMs,ms); totalMs+=ms;
        if(stats.isOpen()) {
            csv<<f.seconds<<','<<ms<<','<<submit<<','<<cpuMs<<','<<f.gain<<','<<f.audio.bass<<','<<f.audio.accent<<','<<f.audio.surge;
            for(double b:f.audio.bands) csv<<','<<b;
            csv<<','<<f.score.onsets.size()<<','<<f.score.bassHits.size()<<','<<f.score.midPeaks.size()
                <<','<<f.schedule.fireworks<<','<<f.schedule.finale.size()
                <<','<<f.schedule.age(Journey::Moment::Train,f.seconds)<<','<<f.schedule.age(Journey::Moment::Cyclist,f.seconds)
                <<','<<world.staticBuilds()<<','<<world.gpu().geometryStats().staticUploads
                <<','<<world.gpu().geometryStats().vertexBytes-previousBytes<<','<<f.audio.preGainLevel
                <<','<<int(f.schedule.fullFireworkShow)
                <<','<<f.schedule.moments[0].cycle<<','<<f.schedule.moments[1].cycle
                <<','<<f.schedule.parameter(Journey::Moment::Train,0,0.85,1.15,1)
                <<','<<f.schedule.parameter(Journey::Moment::Cyclist,0,0.85,1.15,1)
                <<','<<f.schedule.combinations<<','<<world.scale()<<','<<world.gpuMilliseconds()<<'\n';
        }
        if(record) {
            world.gpu().readRgb(rgb);
            if(encoder.write(reinterpret_cast<const char*>(rgb.data()),rgb.size())<0) return 1;
            while(encoder.bytesToWrite()>0) if(!encoder.waitForBytesWritten(5000)) return 1;
        }
    }
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    QTextStream(stderr)<<QString::asprintf("%d frames, %.3f wall seconds; render mean %.3f max %.3f ms\n",renderedFrames,elapsed,totalMs/renderedFrames,maxMs);
    if(record) {
        encoder.closeWriteChannel();
        if(!encoder.waitForFinished(30000) || encoder.exitCode()!=0) return 1;
        // A capture slower than real time must not be reported as live proof.
        if(elapsed>seconds+2) { QTextStream(stderr)<<"capture did not sustain real-time pacing\n"; return 1; }
    }
    return 0;
}
