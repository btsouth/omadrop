#include "../src/headless.h"
#include "../src/kit/world-loader.h"
#include "../src/scene.h"
#include "../src/score.h"
#include "../src/schedule.h"
#include "../src/world.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace Journey;
using namespace Journey::Kit;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void expectLabelError(const QString& label, const QString& expected) {
    try {
        parseWindowLabel("art.svg", "probe", label);
        throw std::runtime_error("invalid label accepted");
    } catch (const std::runtime_error& error) {
        require(QString::fromUtf8(error.what()) == expected, error.what());
    }
}

std::vector<unsigned char> render(int width, int height, const Audio& audio, int seed) {
    HeadlessContext context;
    QString error;
    require(context.create(error), error.toUtf8().constData());
    World world(seed);
    world.setScale(1);
    require(world.init(error), error.toUtf8().constData());
    Score score;
    Schedule schedule(seed);
    world.render(width, height, 1.0, audio, score, schedule);
    std::vector<unsigned char> rgb;
    world.gpu().readRgb(rgb);
    return rgb;
}

double meanWindowBrightness(const std::vector<unsigned char>& rgb, int width, int height,
                            const OsakaWorldDescription& world) {
    const QTransform screen = QTransform().scale(double(width) / 1920.0,
                                                 double(height) / 1080.0);
    double total = 0;
    std::size_t samples = 0;
    for (int index = 0; index < 6; ++index) {
        const QString id = "window-" + QString::number(index);
        const SvgElement* element = nullptr;
        for (const auto& candidate : world.art->elements())
            if (candidate.id == id) element = &candidate;
        require(element != nullptr, "window element missing");
        const QPainterPath shape = screen.map(element->transform.map(element->geometry));
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (!shape.contains(QPointF(x + 0.5, y + 0.5))) continue;
                const std::size_t pixel = std::size_t(y * width + x) * 3;
                total += (0.2126 * rgb[pixel] + 0.7152 * rgb[pixel + 1]
                          + 0.0722 * rgb[pixel + 2]) / 255.0;
                ++samples;
            }
        }
    }
    require(samples > 0, "window mask was empty");
    return total / samples;
}

void grammar() {
    const auto node = parseWindowLabel("art.svg", "window-0",
                                       "window.band5.kick.onset.always");
    require(node.id == "window-0" && node.piece == "window"
                && node.profile == "generic-window-v1" && node.band == 5,
            "valid label did not expand correctly");
    require(node.kick && node.onset && node.always, "valid flags did not expand correctly");
    expectLabelError("door.band0",
                     "art.svg: element id 'probe' label 'door.band0': unknown piece 'door'");
    expectLabelError("window.band0.glow",
                     "art.svg: element id 'probe' label 'window.band0.glow': unknown token 'glow'");
    expectLabelError("window.always",
                     "art.svg: element id 'probe' label 'window.always': missing band0..band5");
    expectLabelError("window.band0.band1",
                     "art.svg: element id 'probe' label 'window.band0.band1': duplicate band 'band1'");
    expectLabelError("window.band0.kick.kick",
                     "art.svg: element id 'probe' label 'window.band0.kick.kick': duplicate token 'kick'");

    const auto duplicate = compileSvg(
        "<svg viewBox='0 0 1920 1080'>"
        "<rect id='dup' data-name='window.band0' width='10' height='10'/>"
        "<rect id='dup' data-name='window.band1' width='10' height='10'/>"
        "</svg>",
        "art.svg");
    require(!duplicate, "duplicate SVG id was accepted");
    require(duplicate.diagnostic
                == "art.svg:1: element <rect> id='dup' label='window.band1': duplicate id 'dup'",
            duplicate.diagnostic.toUtf8().constData());
    std::cout << "PASS: grammar errors include file, element id, label, and reason\n";
}

void expansion() {
    const auto loaded = loadOsakaWorld(QStringLiteral(LIT_WINDOWS_FOLDER));
    const auto& world = loaded->description();
    require(world.windows.size() == 6, "lit-windows did not expand six windows");
    for (int index = 0; index < 6; ++index) {
        const auto& node = world.windows[index];
        require(node.id == "window-" + std::to_string(index), "window order changed");
        require(node.piece == "window" && node.profile == "generic-window-v1",
                "window identity changed");
        require(node.band == index && !node.explicitNode, "shorthand band or source changed");
    }
    require(world.windows[1].kick && !world.windows[1].onset && !world.windows[1].always,
            "window-1 flags changed");
    require(world.windows[3].always && world.windows[3].kick && !world.windows[3].onset,
            "window-3 flags changed");
    require(world.windows[5].kick && world.windows[5].onset && !world.windows[5].always,
            "window-5 flags changed");
    std::cout << "PASS: six labelled SVG layers expanded in source order\n";
}

void explicitOverride() {
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory failed");
    const QString folder = temporary.filePath("lit-windows");
    require(QDir().mkpath(folder), "temporary world directory failed");
    require(QFile::copy(QStringLiteral(LIT_WINDOWS_FOLDER) + "/scene.json",
                        folder + "/scene.json"), "scene copy failed");
    require(QFile::copy(QStringLiteral(LIT_WINDOWS_FOLDER) + "/art.svg",
                        folder + "/art.svg"), "art copy failed");

    QFile file(folder + "/scene.json");
    require(file.open(QIODevice::ReadOnly), "scene read failed");
    auto root = QJsonDocument::fromJson(file.readAll()).object();
    file.close();
    QJsonObject overrideNode;
    overrideNode["id"] = "window-1";
    overrideNode["piece"] = "window";
    overrideNode["profile"] = "generic-window-v1";
    overrideNode["band"] = 5;
    overrideNode["kick"] = false;
    overrideNode["onset"] = false;
    overrideNode["always"] = false;
    root["nodes"] = QJsonArray{overrideNode};
    require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "scene write failed");
    file.write(QJsonDocument(root).toJson());
    file.close();

    const auto loaded = loadOsakaWorld(folder);
    const auto& windows = loaded->description().windows;
    require(windows.size() == 6, "override changed window count");
    const auto overridden = std::find_if(windows.begin(), windows.end(), [](const auto& window) {
        return window.id == "window-1";
    });
    require(overridden != windows.end() && overridden->band == 5
                && overridden->explicitNode, "explicit node did not override shorthand");
    require(loaded->notes().size() == 1
                && loaded->notes().front().contains("explicit window node overrides shorthand"),
            "override was not noted");
    std::cout << "PASS: explicit scene node overrode shorthand and was noted\n";
}

void renderBrightness() {
    qputenv("OMADROP_WORLDS", QByteArray(WORLDS_FOLDER));
    initializeOsakaWorld(QStringLiteral("examples/lit-windows"));
    const auto& world = osakaWorld();

    Audio silent;
    Audio loud;
    loud.bands.fill(1.0);
    loud.bass = 1.0;
    const auto silentRgb = render(640, 360, silent, 1);
    const auto loudRgb = render(640, 360, loud, 1);
    const double silentMean = meanWindowBrightness(silentRgb, 640, 360, world);
    const double loudMean = meanWindowBrightness(loudRgb, 640, 360, world);
    std::cout << "window mean brightness silent " << silentMean
              << " loud " << loudMean << " delta " << (loudMean - silentMean) << '\n';
    require(loudMean > silentMean + 0.08, "loud bands did not brighten the windows");
    std::cout << "PASS: loud bands brightened the windows\n";
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        grammar();
        expansion();
        explicitOverride();
        renderBrightness();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
