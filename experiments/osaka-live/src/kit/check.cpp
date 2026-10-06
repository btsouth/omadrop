#include "check.h"
#include "check-analysis.h"
#include "check-signal.h"
#include "label-pieces.h"
#include "world-loader.h"
#include "../audio.h"
#include "../glcore.h"
#include "../headless.h"
#include "../world.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPainterPath>
#include <QTransform>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>

namespace Journey::Kit::Check {
namespace {
constexpr int HopFrames = 735;           // production hop: 1/60 s at 44100 Hz
constexpr double HopsPerSecond = 60;
constexpr double ReactionDifference = 0.002;   // music vs silence, share of full brightness
constexpr double NodeResponse = 0.02;          // a bound layer must differ this much from silence
constexpr int NodeResponseFrames = 3;          // in at least this many frames
constexpr double BudgetMargin = 1.10;          // Osaka Jade plus 10%
constexpr int DeterminismMoments = 6;

QString seconds(double s) { return QString::number(s, 'f', 1) + " s"; }
QString percent(double fraction, int digits = 1) { return QString::number(fraction * 100, 'f', digits) + "%"; }

struct Fixture {
    std::vector<float> samples;
    QString description;
    int hops = 0;
    double length = 0;
};

Fixture loadFixture(const Options& options) {
    Fixture f;
    if (options.fixture.isEmpty()) {
        f.samples = makeCheckSignal(SignalKind::Music);
        f.description = "built-in test music (60 s)";
    } else {
        QFile file(options.fixture);
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error(("Cannot read the music file " + options.fixture + ": " + file.errorString()).toStdString());
        constexpr qint64 Limit = 44100LL * 8 * 20 * 60;
        if (file.size() < qint64(HopFrames) * 8 || file.size() % 8 || file.size() > Limit)
            throw std::runtime_error(("The music file " + options.fixture
                + " must be raw stereo float32 at 44100 Hz, at least one frame and at most 20 minutes").toStdString());
        f.samples.resize(std::size_t(file.size() / 4));
        if (file.read(reinterpret_cast<char*>(f.samples.data()), file.size()) != file.size())
            throw std::runtime_error(("Cannot read the music file " + options.fixture).toStdString());
        f.description = QFileInfo(options.fixture).fileName();
    }
    f.hops = int(f.samples.size() / 2 / HopFrames);
    f.length = f.hops / HopsPerSecond;
    return f;
}

// Audio, score and schedule advanced the way the live renderer does it.
struct Pipeline {
    StreamingAudio analyzer;
    Audio audio;
    Score score;
    Schedule schedule;
    explicit Pipeline(int seed) : schedule(seed) {}
    void hop(const float* stereo, int index) {
        const double t = (index + 1) / HopsPerSecond;
        analyzer.push(stereo, HopFrames, [&](const Audio& a, double dt) {
            audio = a;
            score.advance(a, t, dt);
            schedule.advance(t, a, score);
        });
    }
};

// The window with the most sudden loudness changes: where a world is most
// likely to flash.
int busiestStart(const Fixture& f, int windowHops, int align) {
    if (f.hops <= windowHops) return 0;
    std::vector<double> flux(f.hops, 0.0);
    double previous = 0;
    for (int i = 0; i < f.hops; ++i) {
        double power = 0;
        const float* s = &f.samples[std::size_t(i) * HopFrames * 2];
        for (int k = 0; k < HopFrames * 2; ++k) power += double(s[k]) * s[k];
        const double rms = std::sqrt(power / (HopFrames * 2));
        flux[i] = std::max(0.0, rms - previous);
        previous = rms;
    }
    double sum = 0;
    for (int i = 0; i < windowHops; ++i) sum += flux[i];
    double best = sum;
    int bestStart = 0;
    for (int s = 1; s + windowHops <= f.hops; ++s) {
        sum += flux[s + windowHops - 1] - flux[s - 1];
        if (s % align == 0 && sum > best + 1e-12) { best = sum; bestStart = s; }
    }
    return bestStart;
}

struct Region { std::vector<std::uint32_t> pixels; };

double regionBrightness(const std::vector<unsigned char>& rgb, const Region& r) {
    double total = 0;
    for (auto p : r.pixels)
        total += 0.2126 * rgb[3 * std::size_t(p)] + 0.7152 * rgb[3 * std::size_t(p) + 1] + 0.0722 * rgb[3 * std::size_t(p) + 2];
    return r.pixels.empty() ? 0 : total / r.pixels.size() / 255.0;
}

struct NodeResult {
    QString id, label, piece = "window";
    int band = 0;
    bool kick = false, onset = false, always = false, sway = false, flicker = false, pulse = false;
    double peak = 0;
    int responding = 0;
    bool responds() const { return responding >= NodeResponseFrames; }
    QString tokens() const {
        QString t = "band" + QString::number(band);
        if (always) t += ".always";
        if (kick) t += ".kick";
        if (onset) t += ".onset";
        if (sway) t += ".sway";
        if (flicker) t += ".flicker";
        if (pulse) t += ".pulse";
        return t;
    }
};

struct Analysis {
    int startHop = 0, hops = 0, frames = 0;
    double musicChange = 0, silentChange = 0, difference = 0, changedFraction = 0;
    std::vector<NodeResult> nodes;
    FlashResult general, red;
};

Analysis analyze(World& world, const Fixture& fixture, const Options& options, int startHop, int windowHops) {
    const int w = options.analysisWidth, h = options.analysisHeight;
    const int perFrame = int(HopsPerSecond) / options.fps;
    const std::size_t pixels = std::size_t(w) * h;
    Analysis out;
    out.startHop = startHop;
    out.hops = windowHops;
    const auto& windows = osakaWorld().windows;
    const auto& pieces = osakaWorld().pieces;
    std::vector<Region> regions(windows.size() + pieces.size());
    const QTransform screen = QTransform().scale(double(w) / 1920.0, double(h) / 1080.0);
    auto fillRegion = [&](Region& region, const QPainterPath& shape) {
        const QRectF box = shape.boundingRect();
        for (int y = std::max(0, int(box.top())); y < std::min(h, int(box.bottom()) + 1); ++y)
            for (int x = std::max(0, int(box.left())); x < std::min(w, int(box.right()) + 1); ++x)
                if (shape.contains(QPointF(x + 0.5, y + 0.5))) region.pixels.push_back(std::uint32_t(y * w + x));
    };
    for (std::size_t n = 0; n < windows.size(); ++n) {
        const SvgElement* element = nullptr;
        for (const auto& e : osakaWorld().art->elements())
            if (e.id.toStdString() == windows[n].id) element = &e;
        NodeResult node;
        node.id = QString::fromStdString(windows[n].id);
        node.label = element ? element->label : QString();
        node.band = windows[n].band; node.kick = windows[n].kick;
        node.onset = windows[n].onset; node.always = windows[n].always;
        out.nodes.push_back(node);
        if (!element) continue;
        fillRegion(regions[n], screen.map(element->transform.map(element->geometry)));
    }
    // Lanterns, lamps, neon signs, glows and wires: measure where each one's light lands.
    for (std::size_t n = 0; n < pieces.size(); ++n) {
        const auto& piece = pieces[n];
        const SvgElement& element = osakaWorld().art->elements()[piece.element];
        NodeResult node;
        node.id = element.id;
        node.label = element.label;
        node.piece = QString::fromStdString(piece.piece);
        node.band = piece.band; node.kick = piece.kick; node.onset = piece.onset; node.always = piece.always;
        node.sway = piece.sway; node.flicker = piece.flicker; node.pulse = piece.pulse;
        out.nodes.push_back(node);
        fillRegion(regions[windows.size() + n], screen.map(LabelPiecesV1::area(piece, element)));
    }
    Pipeline music(options.seed), silence(options.seed);
    const std::vector<float> quiet(std::size_t(HopFrames) * 2, 0.f);
    FlashAnalyzer general(FlashKind::General, w, h, options.fps), red(FlashKind::Red, w, h, options.fps);
    std::vector<unsigned char> m, s, previousMusic, previousSilence;
    double musicChange = 0, silentChange = 0, difference = 0;
    int changed = 0, transitions = 0;
    for (int i = 0; i < startHop + windowHops; ++i) {
        music.hop(&fixture.samples[std::size_t(i) * HopFrames * 2], i);
        silence.hop(quiet.data(), i);
        if (i < startHop || (i + 1) % perFrame) continue;
        const double t = (i + 1) / HopsPerSecond;
        world.render(w, h, t, music.audio, music.score, music.schedule);
        world.gpu().readRgb(m);
        world.render(w, h, t, silence.audio, silence.score, silence.schedule);
        world.gpu().readRgb(s);
        general.add(m.data());
        red.add(m.data());
        const double d = meanBrightnessDifference(m.data(), s.data(), pixels);
        difference += d;
        changed += d > ReactionDifference;
        if (!previousMusic.empty()) {
            musicChange += meanBrightnessDifference(previousMusic.data(), m.data(), pixels);
            silentChange += meanBrightnessDifference(previousSilence.data(), s.data(), pixels);
            ++transitions;
        }
        for (std::size_t n = 0; n < regions.size(); ++n) {
            const double gap = std::abs(regionBrightness(m, regions[n]) - regionBrightness(s, regions[n]));
            out.nodes[n].peak = std::max(out.nodes[n].peak, gap);
            out.nodes[n].responding += gap >= NodeResponse;
        }
        previousMusic = m;
        previousSilence = s;
        ++out.frames;
    }
    if (out.frames) { out.difference = difference / out.frames; out.changedFraction = double(changed) / out.frames; }
    if (transitions) { out.musicChange = musicChange / transitions; out.silentChange = silentChange / transitions; }
    out.general = general.result();
    out.red = red.result();
    return out;
}

std::vector<std::uint64_t> frameHashes(const Fixture& fixture, const Options& options, const std::vector<int>& moments,
                                       QString& error) {
    World world(options.seed);
    world.setScale(1);
    if (!world.init(error)) return {};
    Pipeline pipeline(options.seed);
    std::vector<std::uint64_t> hashes;
    std::vector<unsigned char> rgb;
    std::size_t next = 0;
    for (int i = 0; next < moments.size() && i < fixture.hops; ++i) {
        pipeline.hop(&fixture.samples[std::size_t(i) * HopFrames * 2], i);
        if (i != moments[next]) continue;
        world.render(options.analysisWidth, options.analysisHeight, (i + 1) / HopsPerSecond,
                     pipeline.audio, pipeline.score, pipeline.schedule);
        world.gpu().readRgb(rgb);
        std::uint64_t hash = 14695981039346656037ull;
        for (auto b : rgb) { hash ^= b; hash *= 1099511628211ull; }
        hashes.push_back(hash);
        ++next;
    }
    return hashes;
}

bool softwareRenderer(const QString& renderer) {
    const QString r = renderer.toLower();
    return r.contains("llvmpipe") || r.contains("softpipe") || r.contains("swrast") || r.contains("software");
}

// GPU milliseconds per 1080p frame for the world that is currently loaded.
std::vector<double> measureBudget(const Fixture& fixture, const Options& options, int startHop, int warmup, int frames,
                                  bool gpuTimer, QString& error) {
    World world(options.seed);
    world.setScale(1);
    if (!world.init(error)) return {};
    Pipeline pipeline(options.seed);
    GLuint queries[2] = {0, 0};
    if (gpuTimer) glGenQueries(2, queries);
    const int perFrame = int(HopsPerSecond) / options.fps;
    std::vector<double> ms;
    int rendered = 0;
    for (int i = 0; rendered < warmup + frames; ++i) {
        if (i >= fixture.hops) { error = "The music is too short to measure the frame cost."; break; }
        pipeline.hop(&fixture.samples[std::size_t(i) * HopFrames * 2], i);
        if (i < startHop || (i + 1) % perFrame) continue;
        const double t = (i + 1) / HopsPerSecond;
        const auto wall = std::chrono::steady_clock::now();
        if (gpuTimer) glQueryCounter(queries[0], GL_TIMESTAMP);
        world.render(1920, 1080, t, pipeline.audio, pipeline.score, pipeline.schedule);
        if (gpuTimer) glQueryCounter(queries[1], GL_TIMESTAMP);
        glFinish();
        double elapsed;
        if (gpuTimer) {
            GLuint64 a = 0, z = 0;
            glGetQueryObjectui64v(queries[0], GL_QUERY_RESULT, &a);
            glGetQueryObjectui64v(queries[1], GL_QUERY_RESULT, &z);
            elapsed = double(z - a) / 1e6;
        } else {
            elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - wall).count();
        }
        if (rendered++ >= warmup) ms.push_back(elapsed);
    }
    if (gpuTimer) glDeleteQueries(2, queries);
    return ms;
}

double mean(const std::vector<double>& v) {
    double s = 0;
    for (double x : v) s += x;
    return v.empty() ? 0 : s / v.size();
}
double p95(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, std::size_t(std::ceil(0.95 * v.size())) - 1)];
}

QString timeRange(const FlashResult& r, double offset) {
    return seconds(offset + r.worstStartSeconds) + " to " + seconds(offset + r.worstEndSeconds);
}

QString referenceFolder(const Options& options) {
    QStringList candidates;
    if (!options.reference.isEmpty()) candidates << options.reference;
    else candidates << QDir(osakaWorldsRoot()).filePath("osaka-jade");
    for (const auto& c : candidates)
        if (QFileInfo::exists(QDir(c).filePath("scene.json"))) return c;
    return QString();
}

QString statusName(Status s) {
    switch (s) {
    case Status::Pass: return "pass";
    case Status::Fail: return "fail";
    case Status::Informational: return "info";
    case Status::Skipped: return "skipped";
    }
    return "skipped";
}
}

bool Report::ready() const {
    for (const auto& i : items)
        if (i.required && i.status != Status::Pass) return false;
    return !items.empty();
}

const Item* Report::find(const QString& id) const {
    for (const auto& i : items) if (i.id == id) return &i;
    return nullptr;
}

QJsonObject Report::json() const {
    QJsonArray checks;
    for (const auto& i : items) {
        QJsonObject o{{"id", i.id}, {"title", i.title}, {"status", statusName(i.status)},
                      {"required", i.required}, {"message", i.message}, {"details", i.details}};
        checks.append(o);
    }
    QJsonObject root = info;
    root["world"] = world;
    root["gpu"] = gpu;
    root["softwareRendering"] = softwareRendering;
    root["fixture"] = fixture;
    root["ready"] = ready();
    root["checks"] = checks;
    return root;
}

QString Report::summary() const {
    QString text = "World check: " + world + "\n";
    if (!gpu.isEmpty()) text += "GPU: " + gpu + "\n";
    if (!fixture.isEmpty()) text += "Music: " + fixture + "\n";
    text += "\n";
    int number = 0;
    for (const auto& i : items) {
        ++number;
        QString tag;
        switch (i.status) {
        case Status::Pass: tag = "PASS"; break;
        case Status::Fail: tag = "FAIL"; break;
        case Status::Informational: tag = "INFO"; break;
        case Status::Skipped: tag = "SKIP"; break;
        }
        text += QString("%1  %2. %3\n").arg(tag, -4).arg(number).arg(i.title);
        const auto lines = i.message.split('\n');
        for (const auto& line : lines) text += "        " + line + "\n";
    }
    text += "\n";
    text += ready() ? "READY: every required check passed.\n" : "NOT READY: fix the failed checks above.\n";
    return text;
}

Report runWorldCheck(const Options& options) {
    Report report;
    report.world = options.world;
    Item loads{"loads", "Loads", Status::Skipped, true, {}, {}};
    Item same{"deterministic", "Opens the same way", Status::Skipped, true, {}, {}};
    Item reacts{"reacts", "Reacts to music", Status::Skipped, true, {}, {}};
    Item budget{"budget", "Frame budget", Status::Skipped, true, {}, {}};
    Item flashing{"flashing", "No harsh flashing", Status::Skipped, true, {}, {}};
    auto finish = [&] {
        report.items = {loads, same, reacts, budget, flashing};
        return report;
    };
    const QString notLoaded = "Not run, because the world did not load.";

    const Fixture fixture = loadFixture(options);
    report.fixture = fixture.description;
    QString folder;
    try {
        folder = osakaWorldFolder(options.world);
        initializeOsakaWorldAt(folder);
        const std::size_t layers = osakaWorld().windows.size() + osakaWorld().pieces.size();
        loads.status = Status::Pass;
        loads.message = "The scene and artwork load cleanly";
        loads.message += layers == 0 ? "." : layers == 1 ? " (1 layer reacts to music)."
            : QString(" (%1 layers react to music).").arg(layers);
        loads.details["folder"] = folder;
        loads.details["musicLayers"] = int(layers);
    } catch (const std::exception& e) {
        loads.status = Status::Fail;
        loads.message = QString::fromUtf8(e.what()) + "\nThe other checks need a world that loads, so they were not run.";
        for (Item* i : {&same, &reacts, &budget, &flashing}) i->message = notLoaded;
        return finish();
    }

    HeadlessContext context;
    QString error;
    if (!context.create(error)) throw std::runtime_error(("No GPU is available for the check: " + error).toStdString());
    report.gpu = context.renderer();
    report.softwareRendering = softwareRenderer(report.gpu) || qEnvironmentVariableIntValue("LIBGL_ALWAYS_SOFTWARE") == 1;
    GLint timerBits = 0;
    glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &timerBits);

    const int perFrame = int(HopsPerSecond) / options.fps;
    const int windowHops = std::max(perFrame, std::min(fixture.hops, int(options.analysisSeconds * HopsPerSecond)) / perFrame * perFrame);
    const int startHop = busiestStart(fixture, windowHops, perFrame);
    const double startSeconds = startHop / HopsPerSecond;
    report.info["analysis"] = QJsonObject{{"startSeconds", startSeconds}, {"seconds", windowHops / HopsPerSecond},
        {"fps", options.fps}, {"width", options.analysisWidth}, {"height", options.analysisHeight}, {"seed", options.seed}};
    report.fixture += QString(" (checked %1 from %2)").arg(seconds(windowHops / HopsPerSecond), seconds(startSeconds));

    // 3 and 5: one pass over the busiest stretch of the music.
    Analysis a;
    {
        World world(options.seed);
        world.setScale(1);
        if (!world.init(error)) throw std::runtime_error(error.toStdString());
        a = analyze(world, fixture, options, startHop, windowHops);
    }

    // 3. Reacts to music.
    {
        QJsonArray nodes;
        QStringList silent;
        for (const auto& n : a.nodes) {
            nodes.append(QJsonObject{{"id", n.id}, {"label", n.label}, {"piece", n.piece}, {"binding", n.tokens()},
                {"peakDifference", n.peak}, {"respondingFrames", n.responding}, {"responds", n.responds()}});
            if (!n.responds()) silent.append(n.id + " (" + n.piece + "." + n.tokens() + ")");
        }
        reacts.details["musicFrameChange"] = a.musicChange;
        reacts.details["silenceFrameChange"] = a.silentChange;
        reacts.details["musicVersusSilence"] = a.difference;
        reacts.details["framesDifferentFromSilence"] = a.changedFraction;
        reacts.details["boundLayers"] = nodes;
        const QString numbers = QString("With music the picture is %1 different from silence on average, and changes %2 from frame to frame (%3 in silence).")
            .arg(percent(a.difference, 2), percent(a.musicChange, 2), percent(a.silentChange, 2));
        if (a.difference < ReactionDifference) {
            reacts.status = Status::Fail;
            reacts.message = "This world looks the same with and without music (" + percent(a.difference, 3)
                + " different).\nLabel some SVG layers (window, lantern, lamp, neon, glow or wire, with band0 to band5)\nso they react to the sound; see \"Make layers react to music\" in worlds/README.md.";
        } else if (!silent.isEmpty()) {
            reacts.status = Status::Fail;
            reacts.message = numbers + "\nThese music layers never visibly respond: " + silent.join(", ")
                + ".\nTry another band number, or check that the layer is not hidden behind other art.";
        } else {
            reacts.status = Status::Pass;
            reacts.message = numbers;
            if (!a.nodes.empty())
                reacts.message += a.nodes.size() == 1 ? "\nThe music layer responds." : QString("\nAll %1 music layers respond.").arg(a.nodes.size());
        }
    }

    // 5. No harsh flashing.
    {
        auto describe = [&](const FlashResult& r) {
            return QJsonObject{{"failed", r.failed}, {"worstWindowStart", startSeconds + r.worstStartSeconds},
                {"worstWindowEnd", startSeconds + r.worstEndSeconds}, {"worstAreaOverLimit", r.worstArea},
                {"flashesPerSecondOverQuarterOfFrame", r.worstFlashes}, {"frames", r.frames}, {"fps", r.fps}};
        };
        flashing.details["general"] = describe(a.general);
        flashing.details["red"] = describe(a.red);
        flashing.details["definition"] = "WCAG 2.3.1: more than three flashes in one second over more than 25% of the frame";
        auto worst = [&](const FlashResult& r, const QString& none) {
            if (r.worstArea > 0)
                return QString("worst second %1, where %2 of the picture flashed more than three times (the limit is 25%)")
                    .arg(timeRange(r, startSeconds), percent(r.worstArea));
            if (r.worstFlashes > 0)
                return QString("worst second %1, with up to %2 flashes over a quarter of the picture")
                    .arg(timeRange(r, startSeconds)).arg(r.worstFlashes, 0, 'f', 1);
            return none;
        };
        if (a.general.failed || a.red.failed) {
            flashing.status = Status::Fail;
            QStringList lines;
            if (a.general.failed)
                lines << QString("Harsh flashing from %1: %2 of the picture changes brightness more than three times in one second.")
                    .arg(timeRange(a.general, startSeconds), percent(a.general.worstArea));
            if (a.red.failed)
                lines << QString("Harsh red flashing from %1: %2 of the picture flashes saturated red more than three times in one second.")
                    .arg(timeRange(a.red, startSeconds), percent(a.red.worstArea));
            lines << "Soften the layers that flash: lower what drives them (kick or onset), or light them more gently.";
            flashing.message = lines.join('\n');
        } else {
            flashing.status = Status::Pass;
            flashing.message = "No more than three flashes in any second over a quarter of the picture.\nBrightness: "
                + worst(a.general, "no brightness flashes at all") + ".\nRed: " + worst(a.red, "no saturated red flashes") + ".";
        }
    }

    // 2. Opens the same way.
    {
        std::vector<int> moments;
        const int frames = windowHops / perFrame;
        for (int k = 0; k < DeterminismMoments; ++k)
            moments.push_back(startHop + (std::min(frames - 1, k * frames / DeterminismMoments + 1) + 1) * perFrame - 1);
        moments.erase(std::unique(moments.begin(), moments.end()), moments.end());
        const auto first = frameHashes(fixture, options, moments, error);
        const auto second = frameHashes(fixture, options, moments, error);
        if (first.size() != moments.size() || second.size() != moments.size()) throw std::runtime_error(error.toStdString());
        QStringList different;
        QJsonArray moment;
        for (std::size_t k = 0; k < moments.size(); ++k) {
            const double at = (moments[k] + 1) / HopsPerSecond;
            moment.append(QJsonObject{{"seconds", at}, {"identical", first[k] == second[k]}});
            if (first[k] != second[k]) different.append(seconds(at));
        }
        same.details["moments"] = moment;
        if (different.isEmpty()) {
            same.status = Status::Pass;
            same.message = QString("Two separate runs with the same seed and music drew identical frames at %1 moments.").arg(moments.size());
        } else {
            same.status = Status::Fail;
            same.message = QString("Two runs with the same seed and music drew different frames at %1.").arg(different.join(", "))
                + "\nSomething in the world changes between runs, such as the clock or an unseeded random value.";
        }
    }

    // 4. Frame budget at 1080p against Osaka Jade, measured the same way in this run.
    {
        const bool gpuTimer = timerBits > 0 && !report.softwareRendering;
        const int warmup = options.budgetWarmupFrames ? options.budgetWarmupFrames : (report.softwareRendering ? 2 : 20);
        const int frames = options.budgetFrames ? options.budgetFrames : (report.softwareRendering ? 4 : 60);
        const int rounds = options.budgetRounds ? options.budgetRounds : (report.softwareRendering ? 1 : 2);
        budget.required = !report.softwareRendering;
        const int budgetStart = std::max(0, std::min(startHop, fixture.hops - (warmup + frames + 1) * perFrame));
        const QString reference = referenceFolder(options);
        const bool isReference = !reference.isEmpty() && QFileInfo(reference).canonicalFilePath() == QFileInfo(folder).canonicalFilePath();
        if (reference.isEmpty()) {
            budget.status = Status::Fail;
            budget.message = "Osaka Jade was not found to compare against. Set OMADROP_WORLDS to the worlds folder\nthat contains osaka-jade, or pass --reference with its folder.";
        } else {
            std::vector<double> mine, ref;
            for (int r = 0; r < rounds && error.isEmpty(); ++r) {
                initializeOsakaWorldAt(folder);
                auto m = measureBudget(fixture, options, budgetStart, warmup, frames, gpuTimer, error);
                mine.insert(mine.end(), m.begin(), m.end());
                if (isReference) continue;
                initializeOsakaWorldAt(reference);
                auto o = measureBudget(fixture, options, budgetStart, warmup, frames, gpuTimer, error);
                ref.insert(ref.end(), o.begin(), o.end());
            }
            initializeOsakaWorldAt(folder);
            if (!error.isEmpty()) throw std::runtime_error(error.toStdString());
            const double mineMean = mean(mine), refMean = isReference ? mineMean : mean(ref);
            const double limit = refMean * BudgetMargin;
            budget.details["gpuTimer"] = gpuTimer;
            budget.details["worldMeanMs"] = mineMean;
            budget.details["worldP95Ms"] = p95(mine);
            budget.details["osakaJadeMeanMs"] = refMean;
            budget.details["osakaJadeP95Ms"] = isReference ? p95(mine) : p95(ref);
            budget.details["limitMs"] = limit;
            budget.details["frames"] = int(mine.size());
            budget.details["resolution"] = "1920x1080";
            const QString what = gpuTimer ? "GPU time" : "time (wall clock, no GPU timer)";
            const QString numbers = QString("At 1080p this world takes %1 ms per frame on %2.\nOsaka Jade, measured the same way in this run, takes %3 ms (limit %4 ms: Osaka Jade plus 10%).")
                .arg(mineMean, 0, 'f', 1).arg(report.gpu).arg(refMean, 0, 'f', 1).arg(limit, 0, 'f', 1);
            const bool within = isReference || mineMean <= limit;
            if (report.softwareRendering) {
                budget.status = Status::Informational;
                budget.message = QString("Software rendering: frame %1 is only informational here.\n").arg(what) + numbers;
            } else if (isReference) {
                budget.status = Status::Pass;
                budget.message = QString("This is Osaka Jade, the reference: %1 ms per frame on %2.").arg(mineMean, 0, 'f', 1).arg(report.gpu);
            } else if (within) {
                budget.status = Status::Pass;
                budget.message = numbers;
            } else {
                budget.status = Status::Fail;
                budget.message = numbers + "\nSimplify the heaviest layers (large blurs and glows cost the most) to bring it under the limit.";
            }
        }
    }
    return finish();
}

int runCheckCommand(const Options& options, QTextStream& out, QTextStream& err) {
    Report report;
    try {
        report = runWorldCheck(options);
    } catch (const std::exception& e) {
        err << e.what() << '\n';
        return 2;
    }
    out << report.summary();
    out.flush();
    if (!options.jsonPath.isEmpty()) {
        QFile file(options.jsonPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            err << "Cannot write the report to " << options.jsonPath << ": " << file.errorString() << '\n';
            return 2;
        }
        file.write(QJsonDocument(report.json()).toJson());
    }
    return report.ready() ? 0 : 1;
}
}
