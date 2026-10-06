// Label pieces (lantern, lamp, neon, glow, wire), backdrop pieces from scene.json
// for a non-Osaka world, and how each piece responds to a loud band.
#include "../src/headless.h"
#include "../src/kit/label-pieces.h"
#include "../src/kit/piece-label.h"
#include "../src/kit/world-loader.h"
#include "../src/scene.h"
#include "../src/score.h"
#include "../src/schedule.h"
#include "../src/world.h"
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

using namespace Journey;
using namespace Journey::Kit;

namespace {
void require(bool condition, const QString& message) {
    if (!condition) throw std::runtime_error(message.toStdString());
}

void expectLabelError(const QString& label, const QString& reason) {
    const QString expected = "art.svg: element id 'probe' label '" + label + "': " + reason;
    try {
        parsePieceLabel("art.svg", "probe", label);
    } catch (const std::runtime_error& error) {
        require(QString::fromUtf8(error.what()) == expected,
                QString("wrong error for '%1': %2").arg(label, error.what()));
        return;
    }
    throw std::runtime_error(("invalid label accepted: " + label).toStdString());
}

QString write(const QString& path, const QByteArray& data) {
    QFile file(path);
    require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "cannot write " + path);
    file.write(data);
    return path;
}

QByteArray read(const QString& path) {
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "cannot read " + path);
    return file.readAll();
}

// A world with one empty foreground stage and the given art.
QString makeWorld(QTemporaryDir& temporary, const QString& name, const QByteArray& art) {
    const QString folder = temporary.filePath(name);
    require(QDir().mkpath(folder), "mkpath failed");
    write(folder + "/scene.json",
          R"({"schema":1,"world":"probe","profile":"osaka-world-v1",)"
          R"("stages":[{"id":"s","phase":"Foreground","slots":[]}],"art":{"file":"art.svg"}})");
    write(folder + "/art.svg", "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1920 1080'>" + art + "</svg>");
    return folder;
}

void grammar() {
    for (const char* piece : {"lantern", "lamp", "neon", "glow", "wire"})
        require(isPieceLabelCandidate(piece) && isPieceLabelCandidate(QString(piece) + ".band1"),
                QString("%1 was not recognized").arg(piece));
    require(!isPieceLabelCandidate("window.band1") && !isPieceLabelCandidate("lanterns")
                && !isPieceLabelCandidate("lamp-post") && !isPieceLabelCandidate("wires.left")
                && !isPieceLabelCandidate("glowing") && !isPieceLabelCandidate("neon-sign"),
            "ordinary artist labels were treated as pieces");
    require(pieceReplacesArt("lantern.band1") && pieceReplacesArt("neon.band1")
                && pieceReplacesArt("wire.band1") && !pieceReplacesArt("lamp.band1")
                && !pieceReplacesArt("glow.band1") && !pieceReplacesArt("window.band1"),
            "wrong set of pieces replaces the plain art");

    auto lantern = parsePieceLabel("art.svg", "a", "lantern.band2.kick.sway");
    require(lantern.piece == "lantern" && lantern.profile == "generic-lantern-v1" && lantern.band == 2
                && lantern.kick && lantern.sway && !lantern.onset && lantern.id == "a",
            "lantern did not parse");
    lantern = parsePieceLabel("art.svg", "a", "lantern.sway.band0");
    require(lantern.band == 0 && lantern.sway && !lantern.kick, "token order mattered");
    auto lamp = parsePieceLabel("art.svg", "b", "lamp.band5.kick");
    require(lamp.piece == "lamp" && lamp.profile == "generic-lamp-v1" && lamp.band == 5 && lamp.kick,
            "lamp did not parse");
    auto neon = parsePieceLabel("art.svg", "c", "neon.band3.flicker.kick");
    require(neon.piece == "neon" && neon.profile == "generic-neon-v1" && neon.flicker && neon.kick,
            "neon did not parse");
    auto glow = parsePieceLabel("art.svg", "d", "glow.band1.always.onset.kick");
    require(glow.piece == "glow" && glow.profile == "generic-glow-v1" && glow.always && glow.onset && glow.kick,
            "glow did not parse");
    auto wire = parsePieceLabel("art.svg", "e", "wire.band4.pulse");
    require(wire.piece == "wire" && wire.profile == "generic-wire-v1" && wire.band == 4 && wire.pulse,
            "wire did not parse");
    require(!parsePieceLabel("art.svg", "e", "wire.band0").pulse, "wire pulse defaulted on");

    expectLabelError("lantern", "missing band0..band5");
    expectLabelError("lantern.kick", "missing band0..band5");
    expectLabelError("lantern.band6", "unknown token 'band6'");
    expectLabelError("lantern.band0.band1", "duplicate band 'band1'");
    expectLabelError("lantern.band0.sway.sway", "duplicate token 'sway'");
    expectLabelError("lantern.band0.flicker", "unknown token 'flicker'");
    expectLabelError("lantern.band0.", "unknown token ''");
    expectLabelError("lamp.band0.sway", "unknown token 'sway'");
    expectLabelError("lamp.band1.kick.kick", "duplicate token 'kick'");
    expectLabelError("neon.band0.pulse", "unknown token 'pulse'");
    expectLabelError("neon.Band0", "unknown token 'Band0'");
    expectLabelError("neon.band0.flicker.flicker", "duplicate token 'flicker'");
    expectLabelError("glow.band0.sway", "unknown token 'sway'");
    expectLabelError("glow.band0.always.always", "duplicate token 'always'");
    expectLabelError("wire.band0.kick", "unknown token 'kick'");
    expectLabelError("wire.pulse", "missing band0..band5");
    expectLabelError("wire.band2.pulse.pulse", "duplicate token 'pulse'");
    expectLabelError("door.band0", "unknown piece 'door'");

    // The element has to be something the piece can draw.
    const auto art = compileSvg(
        "<svg viewBox='0 0 1920 1080'>"
        "<g id='group' data-name='lantern.band0'><rect id='inner' width='10' height='10'/></g>"
        "<rect id='ok' data-name='neon.band0' width='10' height='10'/>"
        "</svg>", "art.svg");
    require(bool(art), art.diagnostic);
    const SvgElement* group = nullptr;
    const SvgElement* ok = nullptr;
    for (const auto& element : art.art->elements()) {
        if (element.id == "group") group = &element;
        if (element.id == "ok") ok = &element;
    }
    require(group && ok, "elements missing");
    try {
        checkPieceElement("art.svg", *group, parsePieceLabel("art.svg", "group", "lantern.band0"));
        throw std::runtime_error("a group was accepted as a lantern");
    } catch (const std::runtime_error& error) {
        require(QString::fromUtf8(error.what())
                    == "art.svg: element id 'group' label 'lantern.band0': lantern needs a shape (path, rect, "
                       "circle, ellipse, line, polygon or polyline), not <g>",
                error.what());
    }
    checkPieceElement("art.svg", *ok, parsePieceLabel("art.svg", "ok", "neon.band0"));
    std::cout << "PASS: grammar for every piece: valid labels, every error, element checks\n";
}

void expansion() {
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory failed");
    const QString folder = makeWorld(temporary, "expand",
        "<ellipse id='l1' data-name='lantern.band1.sway' cx='300' cy='300' rx='20' ry='30'/>"
        "<rect id='back' data-name='plain.art' width='1920' height='1080' fill='#001'/>"
        "<circle id='lp' data-name='lamp.band2.kick' cx='600' cy='300' r='10'/>"
        "<path id='n1' data-name='neon.band3.flicker' d='M900 300 h100' fill='none' stroke='#f0f' stroke-width='4'/>"
        "<circle id='g1' data-name='glow.band4.always' cx='1200' cy='300' r='30' fill='#fff'/>"
        "<path id='w1' data-name='wire.band5.pulse' d='M100 800 L1800 800' fill='none' stroke='#000'/>"
        "<rect id='win' data-name='window.band0.always' x='10' y='10' width='50' height='50'/>"
        "<rect id='skip' data-name='lantern-shop' width='5' height='5'/>");
    const auto loaded = loadOsakaWorld(folder);
    const auto& world = loaded->description();
    require(world.windows.size() == 1 && world.windows[0].id == "win", "window was lost");
    require(world.pieces.size() == 5, "expected five pieces");
    const char* ids[] = {"l1", "lp", "n1", "g1", "w1"};
    const char* names[] = {"lantern", "lamp", "neon", "glow", "wire"};
    const char* profiles[] = {"generic-lantern-v1", "generic-lamp-v1", "generic-neon-v1",
                              "generic-glow-v1", "generic-wire-v1"};
    for (int i = 0; i < 5; ++i) {
        const auto& node = world.pieces[std::size_t(i)];
        require(node.id == ids[i] && node.piece == names[i] && node.profile == profiles[i],
                QString("piece %1 changed").arg(i));
        require(world.art->elements()[node.element].id == QString(ids[i]), "element index is wrong");
    }
    require(world.pieces[0].band == 1 && world.pieces[0].sway && !world.pieces[0].kick, "lantern flags");
    require(world.pieces[1].band == 2 && world.pieces[1].kick, "lamp flags");
    require(world.pieces[2].band == 3 && world.pieces[2].flicker, "neon flags");
    require(world.pieces[3].band == 4 && world.pieces[3].always && !world.pieces[3].onset, "glow flags");
    require(world.pieces[4].band == 5 && world.pieces[4].pulse, "wire flags");

    // An error in a label names the file, element, label and reason.
    const QString broken = makeWorld(temporary, "broken",
        "<rect id='bad' data-name='lamp.band1.sway' width='5' height='5'/>");
    try {
        loadOsakaWorld(broken);
        throw std::runtime_error("a bad piece label loaded");
    } catch (const std::runtime_error& error) {
        require(QString::fromUtf8(error.what())
                    == "art.svg: element id 'bad' label 'lamp.band1.sway': unknown token 'sway'", error.what());
    }
    std::cout << "PASS: labelled layers expanded into five pieces in source order\n";
}

std::vector<unsigned char> render(const Audio& audio, int width, int height) {
    HeadlessContext context;
    QString error;
    require(context.create(error), error);
    World world(1);
    world.setScale(1);
    require(world.init(error), error);
    Score score;
    Schedule schedule(1);
    world.render(width, height, 1.0, audio, score, schedule);
    std::vector<unsigned char> rgb;
    world.gpu().readRgb(rgb);
    return rgb;
}

double meanBrightness(const std::vector<unsigned char>& rgb, int width, int height, const QPainterPath& design) {
    const QPainterPath shape = QTransform().scale(double(width) / 1920.0, double(height) / 1080.0).map(design);
    double total = 0;
    std::size_t samples = 0;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            if (!shape.contains(QPointF(x + 0.5, y + 0.5))) continue;
            const std::size_t pixel = std::size_t(y * width + x) * 3;
            total += (0.2126 * rgb[pixel] + 0.7152 * rgb[pixel + 1] + 0.0722 * rgb[pixel + 2]) / 255.0;
            ++samples;
        }
    require(samples > 0, "measuring area was empty");
    return total / samples;
}

void response() {
    struct Case { const char* piece; const char* art; };
    const Case cases[] = {
        {"lantern", "<ellipse id='p' data-name='lantern.band2' cx='960' cy='540' rx='40' ry='52' fill='#e8a24f'/>"},
        {"lantern sway kick", "<ellipse id='p' data-name='lantern.band2.kick.sway' cx='960' cy='540' rx='40' ry='52' fill='#e8a24f'/>"},
        {"lamp", "<circle id='p' data-name='lamp.band2' cx='960' cy='540' r='24' fill='#f7e8b2'/>"},
        {"neon", "<rect id='p' data-name='neon.band2' x='760' y='440' width='400' height='200' fill='none' stroke='#e17fb2' stroke-width='8'/>"},
        {"glow", "<circle id='p' data-name='glow.band2' cx='960' cy='540' r='70' fill='#9fd8ff'/>"},
        {"wire", "<path id='p' data-name='wire.band2' d='M300 540 C700 400 1200 700 1600 540' fill='none' stroke='#000' stroke-width='2'/>"},
        {"wire pulse", "<path id='p' data-name='wire.band2.pulse' d='M300 540 C700 400 1200 700 1600 540' fill='none' stroke='#000' stroke-width='2'/>"},
    };
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory failed");
    Audio silent;
    Audio loud;
    loud.bands.fill(1.0);
    loud.bass = 1.0;
    int index = 0;
    for (const auto& item : cases) {
        const QString folder = makeWorld(temporary, QString("response-%1").arg(index++), item.art);
        initializeOsakaWorldAt(folder);
        const auto& world = osakaWorld();
        require(world.pieces.size() == 1, "expected one piece");
        const auto& node = world.pieces[0];
        const QPainterPath area = LabelPiecesV1::area(node, world.art->elements()[node.element]);
        const auto quiet = render(silent, 640, 360);
        const auto busy = render(loud, 640, 360);
        const double quietMean = meanBrightness(quiet, 640, 360, area);
        const double busyMean = meanBrightness(busy, 640, 360, area);
        std::cout << item.piece << ": mean brightness in its area, silence " << quietMean << ", loud band "
                  << busyMean << ", change " << (busyMean - quietMean) << '\n';
        require(busyMean > quietMean + 0.01, QString("a loud band did not brighten the %1").arg(item.piece));
    }
    std::cout << "PASS: every piece is brighter in its own area with a loud band than in silence\n";
}

void hiddenArt() {
    // A lantern, neon sign or wire is drawn by its piece, not as a flat shape too. A lamp
    // and a glow stay as the fixture and the sign.
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory failed");
    struct Case { const char* label; bool hidden; };
    const Case cases[] = {{"lamp.band0", false}, {"glow.band0", false}, {"lantern.band0", true},
                          {"neon.band0", true}, {"wire.band0", true}, {"plain-label", false}};
    int index = 0;
    for (const auto& item : cases) {
        const QString art = QString("<rect id='p' data-name='%1' x='860' y='440' width='200' height='200' fill='#ffffff' stroke='#ffffff' stroke-width='6'/>")
                                .arg(item.label);
        const QString folder = makeWorld(temporary, QString("hidden-%1").arg(index++), art.toUtf8());
        initializeOsakaWorldAt(folder);
        const auto& world = osakaWorld();
        Canvas canvas;
        world.art->draw(canvas);
        const auto bounds = canvas.bounds();
        const bool drawn = bounds[2] > bounds[0] && bounds[3] > bounds[1];
        require(drawn == !item.hidden,
                QString("'%1' plain art was %2").arg(item.label, drawn ? "drawn" : "hidden"));
    }
    std::cout << "PASS: lanterns, neon and wires replace their plain art; lamps and glows keep it\n";
}

QJsonObject readScene(const QString& folder) {
    return QJsonDocument::fromJson(read(folder + "/scene.json")).object();
}

void backdropWorld(const QString& nightStreet) {
    const auto loaded = loadOsakaWorld(nightStreet);
    const auto& world = loaded->description();
    // A world lists only the stages it draws. Expected values come from the
    // example scene itself so its art can change without editing this test.
    const auto sceneRoot = readScene(nightStreet);
    const auto stages = sceneRoot["stages"].toArray();
    require(stages.size() == 1, "night-street should list one stage");
    const QJsonObject stage = stages[0].toObject();
    const QJsonArray sceneSlots = stage.value("slots").toArray();
    require(int(world.backdrop.count) == sceneSlots.size() && world.coast.count == 0 && world.distantTown.count == 0
                && world.foreground.count == 0, "night-street stages are wrong");
    require(world.backdrop.id == "backdrop" && world.backdrop.events == OsakaEventRef::Life, "stage identity changed");
    int ridgeSlots = 0, hazeSlots = 0;
    for (int i = 0; i < sceneSlots.size(); ++i) {
        const auto slot = sceneSlots[i].toObject();
        const auto piece = slot["piece"].toString();
        const auto& entry = world.backdrop.entries[i];
        const QJsonObject params = slot.value("params").toObject();
        if (piece == "Sky") require(entry.piece == OsakaOp::Sky, "sky slot order changed");
        else if (piece == "Disc") require(entry.piece == OsakaOp::Disc, "disc slot order changed");
        else if (piece == "Ridges") {
            ++ridgeSlots;
            require(entry.piece == OsakaOp::Ridges && entry.params, "ridges did not load");
            const QJsonArray list = params.value("ridges").toArray();
            require(int(entry.params->ridges.size()) == list.size(), "ridge count did not load");
            for (int r = 0; r < list.size(); ++r) {
                const auto want = list[r].toObject();
                const auto& got = entry.params->ridges[r];
                require(got.seed == want["seed"].toInt() && got.base == want["base"].toDouble()
                            && got.amp == want["amp"].toDouble() && got.scale == want["scale"].toDouble()
                            && got.par == want["parallax"].toDouble(), "ridge numbers did not load");
                const QColor top(want["top"].toString());
                require(std::abs(got.top.r - top.redF()) < 1e-6 && std::abs(got.top.g - top.greenF()) < 1e-6,
                        "ridge color did not load");
            }
        } else if (piece == "Haze") {
            ++hazeSlots;
            require(entry.piece == OsakaOp::Haze && entry.params, "haze did not load");
            const auto& haze = entry.params->haze;
            require(haze.y == params["y"].toDouble() && haze.sigma == params["sigma"].toDouble()
                        && haze.drift == params["drift"].toDouble() && haze.seed == params["seed"].toInt()
                        && haze.gain == params["gain"].toDouble(), "haze numbers did not load");
        }
    }
    require(ridgeSlots >= 1 && hazeSlots >= 1, "night-street should use ridges and haze");
    const QJsonObject disc = sceneRoot.value("disc").toObject();
    require(world.disc.x == disc["x"].toDouble() && world.disc.radius == disc["radius"].toDouble(),
            "moon placement did not load");
    // No mountain slot, so no mountain block is needed, and unlisted profiles keep library defaults.
    require(world.mountain.width == OsakaMountainPlacementV1{}.width, "mountain default changed");
    require(world.parameters.sky.timeOffset == OsakaSkyParametersV1{}.timeOffset, "sky defaults changed");
    require(world.windows.size() == 10 && world.pieces.size() == 6, "night-street layers changed");

    // Diagnostics for missing, unknown and wrong parameters in a non-Osaka world.
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory failed");
    const QString folder = temporary.filePath("bad");
    require(QDir().mkpath(folder), "mkpath failed");
    require(QFile::copy(nightStreet + "/art.svg", folder + "/art.svg"), "art copy failed");
    const QString scene = folder + "/scene.json";
    const auto original = readScene(nightStreet);
    auto invalid = [&](const QJsonObject& root, const QString& expected) {
        write(scene, QJsonDocument(root).toJson());
        try {
            loadOsakaWorld(folder);
        } catch (const std::runtime_error& error) {
            require(QString::fromUtf8(error.what()) == scene + ": " + expected,
                    QString("wrong diagnostic: %1 (wanted %2)").arg(error.what(), expected));
            return;
        }
        throw std::runtime_error(("invalid world accepted, wanted: " + expected).toStdString());
    };
    auto changeSlot = [&](int slot, const std::function<void(QJsonObject&)>& edit) {
        auto root = original;
        auto stages = root["stages"].toArray();
        auto stage = stages[0].toObject();
        auto entries = stage["slots"].toArray();
        auto value = entries[slot].toObject();
        edit(value);
        entries[slot] = value;
        stage["slots"] = entries;
        stages[0] = stage;
        root["stages"] = stages;
        return root;
    };
    auto changeParams = [&](int slot, const std::function<void(QJsonObject&)>& edit) {
        return changeSlot(slot, [&](QJsonObject& value) {
            auto params = value["params"].toObject();
            edit(params);
            value["params"] = params;
        });
    };
    const QString haze3 = "$.stages[0].slots[3].params";
    invalid(changeParams(3, [](QJsonObject& p) { p.remove("gain"); }), haze3 + ".gain: expected required field");
    invalid(changeParams(3, [](QJsonObject& p) { p.remove("color"); }), haze3 + ".color: expected required field");
    invalid(changeParams(3, [](QJsonObject& p) { p["strength"] = 1; }), haze3 + ".strength: expected known field (unknown field)");
    invalid(changeParams(3, [](QJsonObject& p) { p["color"] = "green"; }), haze3 + ".color: expected color string like '#1a2b3c'");
    invalid(changeParams(3, [](QJsonObject& p) { p["sigma"] = 0; }), haze3 + ".sigma: expected positive number");
    invalid(changeParams(3, [](QJsonObject& p) { p["y"] = "low"; }), haze3 + ".y: expected finite number");
    invalid(changeSlot(3, [](QJsonObject& v) { v.remove("params"); }), "$.stages[0].slots[3].params: expected required field");
    invalid(changeSlot(0, [](QJsonObject& v) { v["params"] = QJsonObject{}; }), "$.stages[0].slots[0].params: expected known field (unknown field)");
    const QString ridge1 = "$.stages[0].slots[2].params.ridges[1]";
    invalid(changeParams(2, [](QJsonObject& p) {
        auto list = p["ridges"].toArray(); auto r = list[1].toObject(); r.remove("amp"); list[1] = r; p["ridges"] = list; }),
        ridge1 + ".amp: expected required field");
    invalid(changeParams(2, [](QJsonObject& p) {
        auto list = p["ridges"].toArray(); auto r = list[1].toObject(); r["scale"] = -3; list[1] = r; p["ridges"] = list; }),
        ridge1 + ".scale: expected positive number");
    invalid(changeParams(2, [](QJsonObject& p) { p["ridges"] = QJsonArray{}; }),
            "$.stages[0].slots[2].params.ridges: expected array of 1..8 ridges");
    invalid(changeSlot(2, [](QJsonObject& v) { v["piece"] = "Rubble"; }), "$.stages[0].slots[2].piece: expected known v1 piece");
    {
        auto root = original;
        root.remove("disc");
        invalid(root, "$.disc: expected required field");
        root = original;
        auto stages = root["stages"].toArray();
        auto stage = stages[0].toObject();
        auto entries = stage["slots"].toArray();
        entries.append(QJsonObject{{"id", "peak"}, {"piece", "Mountain"}, {"gate", "Always"}, {"profile", "osaka-mountain-v1"}});
        stage["slots"] = entries;
        stages[0] = stage;
        root["stages"] = stages;
        invalid(root, "$.mountain: expected required field");
        root = original;
        auto second = stage;
        second["id"] = "again";
        stages.append(second);
        root["stages"] = stages;
        invalid(root, "$.stages[1].phase: expected Backdrop, Coast, DistantTown or Foreground, each at most once and in that order");
        root = original;
        root["stages"] = QJsonArray{};
        invalid(root, "$.stages: expected array of 1..4 ordered stages");
    }
    std::cout << "PASS: a non-Osaka world loads sky, moon, ridges and haze from scene.json; bad parameters name file, path and expectation\n";
}

void backdropRenders(const QString& nightStreet) {
    // Haze and ridges draw from their scene.json parameters: changing the haze gain changes the picture.
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory failed");
    const QString folder = temporary.filePath("haze");
    require(QDir().mkpath(folder), "mkpath failed");
    require(QFile::copy(nightStreet + "/art.svg", folder + "/art.svg"), "art copy failed");
    auto root = readScene(nightStreet);
    auto stages = root["stages"].toArray();
    auto stage = stages[0].toObject();
    auto entries = stage["slots"].toArray();
    auto haze = entries[3].toObject();
    auto params = haze["params"].toObject();
    Audio silent;
    std::vector<std::vector<unsigned char>> pictures;
    for (double gain : {0.0, 0.5}) {
        params["gain"] = gain;
        haze["params"] = params;
        entries[3] = haze;
        stage["slots"] = entries;
        stages[0] = stage;
        root["stages"] = stages;
        write(folder + "/scene.json", QJsonDocument(root).toJson());
        initializeOsakaWorldAt(folder);
        pictures.push_back(render(silent, 640, 360));
    }
    double difference = 0;
    for (std::size_t i = 0; i < pictures[0].size(); ++i) difference += std::abs(int(pictures[0][i]) - int(pictures[1][i]));
    difference /= double(pictures[0].size()) * 255.0;
    std::cout << "haze gain 0 against 0.5: mean difference " << difference << '\n';
    require(difference > 0.002, "the haze parameters did not change the picture");
    std::cout << "PASS: haze parameters from scene.json change the rendered picture\n";
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        grammar();
        expansion();
        hiddenArt();
        backdropWorld(QStringLiteral(NIGHT_STREET_FOLDER));
        response();
        backdropRenders(QStringLiteral(NIGHT_STREET_FOLDER));
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
