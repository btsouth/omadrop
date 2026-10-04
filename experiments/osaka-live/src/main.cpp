#include "session.h"
#include "world.h"
#include "headless.h"
#include "preview.h"
#include <QCoreApplication>
#include <QGuiApplication>
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
#include <cmath>

int main(int argc,char** argv) {
    bool headless=false;
    for(int i=1;i<argc;++i) if(QString::fromLocal8Bit(argv[i])=="--record"
        || QString::fromLocal8Bit(argv[i])=="--probe" || QString::fromLocal8Bit(argv[i])=="--bench"
        || QString::fromLocal8Bit(argv[i])=="--verify-render") headless=true;
    QSurfaceFormat format; format.setVersion(3,3); format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    std::unique_ptr<QCoreApplication> app;
    if(headless) app=std::make_unique<QCoreApplication>(argc,argv);
    else app=std::make_unique<QGuiApplication>(argc,argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Osaka Jade live milestone. Existing system capture; no file analysis or film chapters.");
    parser.addHelpOption();
    parser.addOptions({{"record","Real-time headless capture to MP4.","path"},
        {"probe","Measure streaming response without rendering."},
        {"bench","Measure rendering without encoding or framebuffer readback."},
        {"verify-render","Compare optimized drawing with accepted canvas output."},
        {"seconds","Bounded recording/probe length; preview loops when omitted.","number","60"},
        {"fps","Recording frames per second.","number","30"},
        {"width","Frame width.","number","1920"},{"height","Frame height.","number","1080"},
        {"stats","Per-frame response and timing CSV.","path"}});
    parser.process(*app);
    bool ok=false;
    const double seconds=parser.value("seconds").toDouble(&ok);
    if(!ok || !std::isfinite(seconds) || seconds<=0 || seconds>300) return 2;
    const int fps=parser.value("fps").toInt(&ok);
    if(!ok || fps<1 || fps>60) return 2;
    const int w=parser.value("width").toInt(&ok);
    if(!ok || w<320 || w>3840) return 2;
    const int h=parser.value("height").toInt(&ok);
    if(!ok || h<180 || h>2160) return 2;
    Journey::LiveSession session(1);
    if(!headless) {
        OsakaItem::session=&session;
        qmlRegisterType<OsakaItem>("Osaka",1,0,"OsakaItem");
        QQmlApplicationEngine engine;
        engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
        if(engine.rootObjects().isEmpty()) return 1;
        if(parser.isSet("seconds")) QTimer::singleShot(int(seconds*1000),app.get(),&QCoreApplication::quit);
        return app->exec();
    }
    Journey::HeadlessContext context;
    Journey::World world(1);
    if(parser.isSet("verify-render")) {
        QString error;
        if(!context.create(error) || !world.init(error)) { QTextStream(stderr)<<error<<'\n'; return 1; }
        for(double t:{8.0,14.0,28.0}) {
            Journey::LiveFrame f;
            f.schedule.advance(t,{},f.score);
            f.schedule.fireworks=t-1.4;
            f.schedule.fireworkStrength=0.8;
            f.schedule.finale={{t-0.4,0.8,1}};
            std::vector<unsigned char> before,after;
            Journey::Canvas::useKnownConvex=false;
            world.render(w,h,t,f.audio,f.score,f.schedule); world.gpu().readRgb(before);
            Journey::Canvas::useKnownConvex=true;
            world.render(w,h,t,f.audio,f.score,f.schedule); world.gpu().readRgb(after);
            if(before!=after) {
                std::size_t changed=0; int maximum=0;
                for(std::size_t i=0;i<before.size();++i) {
                    changed+=before[i]!=after[i]; maximum=std::max(maximum,std::abs(int(before[i])-int(after[i])));
                }
                QTextStream(stderr)<<"canvas image differs: "<<changed<<" channels, max "<<maximum<<'\n';
                return 1;
            }
            std::uint64_t hash=14695981039346656037ull;
            for(auto byte:after) { hash^=byte; hash*=1099511628211ull; }
            QTextStream(stdout)<<t<<" RGB hash "<<QString::number(hash,16)<<'\n';
        }
        QTextStream(stdout)<<"PASS: optimized canvas matches accepted RGB exactly at 8, 14 and 28 seconds\n";
        return 0;
    }
    const bool record=parser.isSet("record");
    const bool render=record || parser.isSet("bench");
    QString error;
    if(render && (!context.create(error) || !world.init(error))) { QTextStream(stderr)<<error<<'\n'; return 1; }
    if(render) QTextStream(stderr)<<"GPU: "<<context.renderer()<<'\n';
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
    if(stats.isOpen()) csv<<"seconds,render_ms,submit_ms,gain,bass,accent,surge,band0,band1,band2,band3,band4,band5,onsets,bass_hits,mid_peaks,firework_at,finale_count,train_age,cyclist_age\n";
    const auto start=std::chrono::steady_clock::now();
    std::vector<unsigned char> rgb;
    const int frames=int(std::ceil(seconds*fps));
    double maxMs=0,totalMs=0;
    for(int i=0;i<frames;++i) {
        const auto deadline=start+std::chrono::microseconds(std::llround(i*1e6/fps));
        std::this_thread::sleep_until(deadline);
        const auto f=session.snapshot();
        QElapsedTimer timer; timer.start();
        double submit=0;
        if(render) {
            world.render(w,h,f.seconds,f.audio,f.score,f.schedule);
            submit=timer.nsecsElapsed()/1e6;
            glFinish();
        }
        const double ms=timer.nsecsElapsed()/1e6;
        maxMs=std::max(maxMs,ms); totalMs+=ms;
        if(stats.isOpen()) {
            csv<<f.seconds<<','<<ms<<','<<submit<<','<<f.gain<<','<<f.audio.bass<<','<<f.audio.accent<<','<<f.audio.surge;
            for(double b:f.audio.bands) csv<<','<<b;
            csv<<','<<f.score.onsets.size()<<','<<f.score.bassHits.size()<<','<<f.score.midPeaks.size()
                <<','<<f.schedule.fireworks<<','<<f.schedule.finale.size()
                <<','<<f.schedule.age(Journey::Moment::Train,f.seconds)<<','<<f.schedule.age(Journey::Moment::Cyclist,f.seconds)<<'\n';
        }
        if(record) {
            world.gpu().readRgb(rgb);
            if(encoder.write(reinterpret_cast<const char*>(rgb.data()),rgb.size())<0) return 1;
            while(encoder.bytesToWrite()>0) if(!encoder.waitForBytesWritten(5000)) return 1;
        }
    }
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    QTextStream(stderr)<<QString::asprintf("%d frames, %.3f wall seconds; render mean %.3f max %.3f ms\n",frames,elapsed,totalMs/frames,maxMs);
    if(record) {
        encoder.closeWriteChannel();
        if(!encoder.waitForFinished(30000) || encoder.exitCode()!=0) return 1;
        // A capture slower than real time must not be reported as live proof.
        if(elapsed>seconds+2) { QTextStream(stderr)<<"capture did not sustain real-time pacing\n"; return 1; }
    }
    return 0;
}
