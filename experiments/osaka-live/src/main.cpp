#include "kit/check.h"
#include "kit/world-loader.h"
#include "kit/world-reload.h"
#include "session.h"
#include "world.h"
#include "headless.h"
#include "preview.h"
#include "render_comparison.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QDir>
#include <QImage>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QTimer>
#include <chrono>
#include <thread>
#include <time.h>
#include <cmath>
#include <random>
#include <limits>

// Hyprland gives a new window pointer focus only once the pointer moves, so the
// blank cursor stays drawn until then. Nudge it one pixel and back.
// "This screen" is the focused monitor, not the compositor's first one.
static QScreen* focusedScreen() {
    QProcess query;
    query.start(qEnvironmentVariable("OMADROP_HYPRCTL","hyprctl"),{"monitors","-j"});
    if(!query.waitForFinished(500)) { query.kill(); query.waitForFinished(100); return nullptr; }
    for(const auto& value:QJsonDocument::fromJson(query.readAllStandardOutput()).array()) {
        const auto monitor=value.toObject();
        if(!monitor.value("focused").toBool()) continue;
        for(auto* screen:QGuiApplication::screens())
            if(screen->name()==monitor.value("name").toString()) return screen;
    }
    return nullptr;
}

static void settlePointer() {
    const QString hyprctl=qEnvironmentVariable("OMADROP_HYPRCTL","hyprctl");
    auto run=[&](const QStringList& args) {
        QProcess p; p.start(hyprctl,args);
        // Bounded: this runs once on the GUI thread as the first frames land.
        if(!p.waitForFinished(250)) { p.kill(); p.waitForFinished(100); return QString(); }
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    };
    const QStringList at=run({"cursorpos"}).split(", ");
    bool okX=false, okY=false;
    const int x=at.size()==2?at[0].toInt(&okX):0, y=at.size()==2?at[1].toInt(&okY):0;
    if(!okX || !okY) return;
    for(int dx:{1,0})
        run({"eval",QString("hl.dispatch(hl.dsp.cursor.move({ x = %1, y = %2 }))").arg(x+dx).arg(y)});
}

int main(int argc,char** argv) {
    bool headless=false;
    for(int i=1;i<argc;++i) if(QString::fromLocal8Bit(argv[i])=="--record"
        || QString::fromLocal8Bit(argv[i])=="--probe" || QString::fromLocal8Bit(argv[i])=="--bench"
        || QString::fromLocal8Bit(argv[i])=="--verify-render"
        || QString::fromLocal8Bit(argv[i])=="--fidelity" || QString::fromLocal8Bit(argv[i])=="--check") headless=true;
    const QString fpsOption=qEnvironmentVariable("OMADROP_OSAKA_FPS","auto");
    int fixedFps=0;
    if(fpsOption!="auto") {
        bool valid=false;fixedFps=fpsOption.toInt(&valid);
        if(!valid || (fixedFps!=30 && fixedFps!=60)) { QTextStream(stderr)<<"OMADROP_OSAKA_FPS must be auto, 30 or 60\n"; return 2; }
    }
    OsakaItem::fixedFps=fixedFps;
    QSurfaceFormat format; format.setSwapInterval(fixedFps==30 ? 2 : 1); format.setVersion(3,3); format.setProfile(QSurfaceFormat::CoreProfile);
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
        {"fidelity","Deterministic offline frame capture (developer tool).","directory"},
        {"fixture","Stereo float32 44100 Hz music for fidelity capture, --check (built-in music when omitted) or --preview (loops; live audio when omitted).","path"},
        {"preview","Open a window that reloads --world whenever scene.json or art.svg is saved (live audio, or --fixture)."},
        {"check","Check that a world is ready: loads, reacts to music, frame cost, no harsh flashing."},
        {"json","Write the full --check report as JSON.","path"},
        {"reference","Osaka Jade folder to compare --check frame cost with (default: osaka-jade under the worlds folder).","folder"},
        {"seconds","Bounded recording/probe length; preview loops when omitted.","number","60"},
        {"fps","Recording frames per second.","number","30"},
        {"width","Frame width.","number","1920"},{"height","Frame height.","number","1080"},
        {"scale","Internal resolution scale: auto, or 0.5 to 1.0.","value"},
        {"seed","Schedule seed (live default random; headless default 1).","number"},
        {"world","World folder under OMADROP_WORLDS.","name","osaka-jade"},
        {"stats","Per-frame response and timing CSV.","path"}});
    parser.process(*app);
    if(parser.isSet("check")) {
        Journey::Kit::Check::Options check;
        check.world=parser.value("world"); check.fixture=parser.value("fixture");
        check.jsonPath=parser.value("json"); check.reference=parser.value("reference");
        bool valid=false;
        if(parser.isSet("seed")) { check.seed=parser.value("seed").toInt(&valid); if(!valid || check.seed<0) return 2; }
        if(parser.isSet("seconds")) {
            check.analysisSeconds=parser.value("seconds").toDouble(&valid);
            if(!valid || !std::isfinite(check.analysisSeconds) || check.analysisSeconds<1 || check.analysisSeconds>600) return 2;
        }
        if(parser.isSet("fps")) {
            check.fps=parser.value("fps").toInt(&valid);
            if(!valid || (check.fps!=30 && check.fps!=60)) { QTextStream(stderr)<<"--check needs --fps 30 or 60\n"; return 2; }
        }
        QTextStream out(stdout), err(stderr);
        return Journey::Kit::Check::runCheckCommand(check,out,err);
    }
    try { Journey::Kit::initializeOsakaWorld(parser.value("world")); }
    catch (const std::exception& e) { QTextStream(stderr)<<e.what()<<'\n'; return 2; }
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
    OsakaItem::statsPath=parser.value("stats");
    int seed=1;
    if(parser.isSet("seed")) {
        seed=parser.value("seed").toInt(&ok); if(!ok || seed<0) return 2;
    } else if(!headless) seed=int(std::random_device{}() & 0x7fffffff);
    QTextStream(stderr)<<"Schedule seed: "<<seed<<'\n';
    if(parser.isSet("fidelity")) {
        Journey::HeadlessContext context;
        Journey::World world(1); world.setScale(1);
        QString error;
        if(!context.create(error) || !world.init(error)) { QTextStream(stderr)<<error<<'\n'; return 1; }
        QTextStream(stdout)<<"GPU: "<<context.renderer()<<'\n';
        const QString directory=parser.value("fidelity");
        if(!QDir().mkpath(directory)) return 1;
        QFile fixture(parser.value("fixture"));
        if(!fixture.open(QIODevice::ReadOnly) || fixture.size()==0 || fixture.size()%5880) return 2;
        QFile manifest(directory+"/frames.csv");
        if(!manifest.open(QIODevice::WriteOnly|QIODevice::Truncate)) return 1;
        QTextStream csv(&manifest); csv<<"frame,seconds,brightness,change,firework_at,full_show\n";
        std::vector<unsigned char> rgb,previous;
        auto save=[&](const QString& name) {
            return QImage(rgb.data(),w,h,w*3,QImage::Format_RGB888).save(directory+"/"+name+".png");
        };
        for(double t:{8.0,14.0,28.0}) {
            Journey::LiveFrame f;
            f.schedule.advance(t,{},f.score); f.schedule.fireworks=t-1.4;
            f.schedule.fireworkStrength=0.8; f.schedule.finale={{t-0.4,0.8,1}};
            world.render(w,h,t,f.audio,f.score,f.schedule); world.gpu().readRgb(rgb);
            if(!save("verify-"+QString::number(int(t)))) return 1;
            std::uint64_t hash=14695981039346656037ull;
            for(auto b:rgb) { hash^=b; hash*=1099511628211ull; }
            QTextStream(stdout)<<t<<" RGB hash "<<QString::number(hash,16)<<'\n';
        }
        // Consume exactly the production 735-frame hops, with a deterministic
        // clock. Never start capture, rewrite live time, or change the analyzer.
        Journey::StreamingAudio analyzer;
        Journey::LiveFrame f; f.schedule=Journey::Schedule(seed);
        std::array<float,1470> hop{};
        const int hops=int(std::min<qint64>(fixture.size()/5880,36000));
        for(int i=0;i<hops;++i) {
            if(fixture.read(reinterpret_cast<char*>(hop.data()),5880)!=5880) return 1;
            const double t=(i+1)/60.0;
            analyzer.push(hop.data(),735,[&](const Journey::Audio& a,double dt) {
                f.audio=a; f.score.advance(a,t,dt); f.schedule.advance(t,a,f.score);
            });
            if((i+1)%12) continue; // Fixed 5 Hz comparison cadence, including t=8/14/28.
            world.render(w,h,t,f.audio,f.score,f.schedule); world.gpu().readRgb(rgb);
            double brightness=0,change=0;
            for(std::size_t k=0;k<rgb.size();k+=3) {
                const double v=(0.2126*rgb[k]+0.7152*rgb[k+1]+0.0722*rgb[k+2])/255;
                brightness+=v;
                if(!previous.empty()) {
                    const double old=(0.2126*previous[k]+0.7152*previous[k+1]+0.0722*previous[k+2])/255;
                    change+=std::abs(v-old);
                }
            }
            csv<<(i+1)/12<<','<<t<<','<<brightness/(w*h)<<','<<change/(w*h)<<','<<f.schedule.fireworks<<','<<int(f.schedule.fullFireworkShow)<<'\n';
            if((i+1)%120==0 || i+1==1320 || i+1==1560 || i+1==2520 || i+1==3240) {
                if(!save("fixture-"+QString::number(i+1))) return 1;
            }
            previous=rgb;
        }
        QTextStream(stdout)<<"Captured "<<hops/12<<" deterministic fixture frames; 5 Hz brightness-change series\n";
        return 0;
    }
    std::vector<float> previewMusic;
    if(parser.isSet("preview") && parser.isSet("fixture")) {
        QFile file(parser.value("fixture"));
        if(!file.open(QIODevice::ReadOnly) || file.size()<5880 || file.size()%8 || file.size()>44100LL*8*20*60) {
            QTextStream(stderr)<<"--fixture must be a readable raw stereo float32 44100 Hz file, up to 20 minutes\n"; return 2;
        }
        previewMusic.resize(std::size_t(file.size()/4));
        if(file.read(reinterpret_cast<char*>(previewMusic.data()),file.size())!=file.size()) return 2;
    }
    Journey::LiveSession session(seed,std::move(previewMusic));
    if(parser.isSet("preview")) {
        OsakaItem::session=&session;
        qmlRegisterType<OsakaItem>("Osaka",1,0,"OsakaItem");
        QQmlApplicationEngine engine;
        Journey::Kit::WorldReloader reloader(parser.value("world"),Journey::Kit::osakaWorldFolder(parser.value("world")));
        QObject::connect(&reloader,&Journey::Kit::WorldReloader::finished,[](bool,const QString& message) {
            QTextStream(stderr)<<message<<'\n';
        });
        engine.rootContext()->setContextProperty("worldPreview",&reloader);
        // A normal window, not the screensaver class the compositor rules match.
        QGuiApplication::setDesktopFileName("org.omadrop.worldpreview");
        engine.load(QUrl(QStringLiteral("qrc:/PreviewMain.qml")));
        if(engine.rootObjects().isEmpty()) return 1;
        reloader.start();
        QTextStream(stderr)<<"Preview of "<<parser.value("world")<<": save scene.json or art.svg to reload, R reloads, Esc quits\n";
        if(parser.isSet("seconds")) QTimer::singleShot(int(seconds*1000),app.get(),&QCoreApplication::quit);
        return app->exec();
    }
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
        auto screens=QGuiApplication::screens();
        if(display=="single")
            if(auto* focused=focusedScreen()) screens={focused};
        const int count=display=="single"?std::min(1,int(screens.size())):int(screens.size());
        for(int i=0;i<count;++i) {
            engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
            if(engine.rootObjects().size()!=i+1) return 1;
            auto* window=qobject_cast<QQuickWindow*>(engine.rootObjects().back());
            if(!window) return 1;
            window->setScreen(screens[i]);
            window->setPosition(screens[i]->geometry().topLeft());
            // Main.qml shows the pointer only while it moves; start hidden.
            window->setCursor(Qt::BlankCursor);
            window->showFullScreen();
            // Qt takes pointer focus only once it has drawn.
            if(i==0) QObject::connect(window,&QQuickWindow::frameSwapped,app.get(),[] {
                static int frames=0;
                if(++frames==3) settlePointer();
            },Qt::QueuedConnection);
        }
        if(!count) return 1;
        if(parser.isSet("seconds")) QTimer::singleShot(int(seconds*1000),app.get(),&QCoreApplication::quit);
        return app->exec();
    }
    Journey::HeadlessContext context;
    Journey::World world(1);
    if(fixedFps>0) world.setFps(fixedFps);
    if(fixedScale>0) world.setScale(fixedScale);
    if(parser.isSet("verify-render")) {
        world.setScale(1);
        QString error;
        if(!context.create(error) || !world.init(error)) { QTextStream(stderr)<<error<<'\n'; return 1; }
        int pose=0;
        bool allExact=true;
        // RGBA16F RGB after intentional precision revert ad97946, retaining
        // b4c2984, d7b8cb0 and e98b80b filters plus 0ae962c effect fusion.
        // Both observed 8-second MSAA rounding histories retain exact hash gates.
        // Evidence: round4/golden-capture-verify-{1,2,3}.log under
        // /home/bts/Projects/_evidence/omadrop/igpu-speedups-2026-10-04/.
        const std::uint64_t accepted[][2]={
            {0xc2497d0e78e4a478ull,0x7be1177005405f5cull},
            {0x5697914307884b97ull,0x5697914307884b97ull},
            {0xb3c1e9ea42c7968ull,0xb3c1e9ea42c7968ull}};
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
            const auto comparison=Journey::compareRender(before,after);
            if(!comparison.exact()) {
                allExact=false;
                QTextStream(stderr)<<t<<" seconds: canvas image differs: "<<comparison.changed
                    <<" channels, max "<<comparison.maximum<<"/255; "
                    <<(comparison.acceptable() ? "within approved tolerance" : "FAIL")<<'\n';
                if(!comparison.acceptable()) return 1;
            }
            std::uint64_t hash=14695981039346656037ull;
            for(auto byte:after) { hash^=byte; hash*=1099511628211ull; }
            QTextStream(stdout)<<t<<" RGB hash "<<QString::number(hash,16)<<'\n';
            if(w==1920 && h==1080 && context.renderer().contains("RTX 4070 SUPER") && hash!=accepted[pose][0] && hash!=accepted[pose][1]) {QTextStream(stderr)<<"accepted M1 RGB hash differs\n";return 1;}
            ++pose;
        }
        QTextStream(stdout)<<"PASS: retained geometry and optimized canvas match uncached RGB "
            <<(allExact ? "exactly" : "within 1/255 on at most 64 channels per pose")
            <<" at 8, 14 and 28 seconds\n";
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
