#include "world-loader.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <cmath>
#include <stdexcept>
namespace Journey::Kit {
namespace {
struct Reader {
    QString file;
    QSet<QString> ids;
    [[noreturn]] void fail(const QString& path, const QString& expected) const {
        throw std::runtime_error((file + ": " + path + ": expected " + expected).toStdString());
    }
    QJsonObject object(const QJsonValue& v, const QString& p, const QStringList& fields) const {
        if (!v.isObject()) fail(p, "object");
        const auto o = v.toObject();
        for (auto it=o.begin(); it!=o.end(); ++it)
            if (!fields.contains(it.key())) fail(p+"."+it.key(), "known field (unknown field)");
        for (const auto& f : fields) if (!o.contains(f)) fail(p+"."+f, "required field");
        return o;
    }
    QString string(const QJsonValue& v, const QString& p) const {
        if (!v.isString() || v.toString().isEmpty()) fail(p, "nonempty string");
        return v.toString();
    }
    void literal(const QJsonValue& v, const QString& p, const QString& s) const {
        if (string(v,p)!=s) fail(p, "'"+s+"'");
    }
    double number(const QJsonValue& v, const QString& p) const {
        if (!v.isDouble() || !std::isfinite(v.toDouble())) fail(p, "finite number");
        return v.toDouble();
    }
    void id(const QJsonValue& v, const QString& p) {
        const auto s=string(v,p);
        if (ids.contains(s)) fail(p, "unique ID (duplicate '"+s+"')");
        ids.insert(s);
    }
};
struct Piece { const char* name; OsakaOp op; const char* profile; };
constexpr Piece pieces[] = {
    {"Sky", OsakaOp::Sky, "osaka-sky-v1"},
    {"AfterSky", OsakaOp::AfterSky, "after-sky"},
    {"Star", OsakaOp::Star, "osaka-shooting-star-v1"},
    {"DiscHook", OsakaOp::DiscHook, "disc-port"},
    {"Disc", OsakaOp::Disc, "osaka-disc-v1"},
    {"MountainHook", OsakaOp::MountainHook, "mountain-port"},
    {"Mountain", OsakaOp::Mountain, "osaka-mountain-v1"},
    {"BeforeCoast", OsakaOp::BeforeCoast, "before-coast"},
    {"CoastHook", OsakaOp::CoastHook, "coast-port"},
    {"Ridges", OsakaOp::Ridges, "osaka-ridges-v1"},
    {"City", OsakaOp::City, "osaka-valley-city-v1"},
    {"Firework", OsakaOp::Firework, "osaka-firework-v1"},
    {"AfterValley", OsakaOp::AfterValley, "after-valley"},
    {"NearRidge", OsakaOp::NearRidge, "osaka-near-ridge-v1"},
    {"Train", OsakaOp::Train, "osaka-train-v1"},
    {"SkyLanterns", OsakaOp::SkyLanterns, "osaka-sky-lanterns-v1"},
    {"Downhill", OsakaOp::Downhill, "osaka-downhill-rows-v1"},
    {"FarNetwork", OsakaOp::FarNetwork, "osaka-network-group-v1"},
    {"TownHook", OsakaOp::TownHook, "town-port"},
    {"AfterTown", OsakaOp::AfterTown, "after-town"},
    {"RightTown", OsakaOp::RightTown, "osaka-right-town-v1"},
    {"StreetSurface", OsakaOp::StreetSurface, "osaka-street-surface-group-v1"},
    {"Cart", OsakaOp::Cart, "osaka-cart-group-v1"},
    {"AfterCart", OsakaOp::AfterCart, "after-cart"},
    {"Festoon", OsakaOp::Festoon, "osaka-festoon-v1"},
    {"NearNetwork", OsakaOp::NearNetwork, "osaka-network-group-v1"},
    {"Moths", OsakaOp::Moths, "osaka-moths-v1"},
    {"AfterWires", OsakaOp::AfterWires, "after-wires"},
    {"ReflectionCapture", OsakaOp::ReflectionCapture, "osaka-reflection-v1"},
    {"AfterReflections", OsakaOp::AfterReflections, "after-reflections"},
    {"StreetActors", OsakaOp::StreetActors, "osaka-street-actors-v1"},
    {"Birds", OsakaOp::Birds, "osaka-flock-v1"},
    {"NearHouse", OsakaOp::NearHouse, "osaka-near-group-v1"},
    {"Wisteria", OsakaOp::Wisteria, "osaka-wisteria-v1"},
};
struct Gate { const char* name; OsakaGate gate; };
constexpr Gate gates[] = {
    {"Always", OsakaGate::Always},
    {"Chapter", OsakaGate::Chapter},
    {"DiscEnabled", OsakaGate::DiscEnabled},
    {"MountainEnabled", OsakaGate::MountainEnabled},
    {"Land", OsakaGate::Land},
    {"DefaultCoastLand", OsakaGate::DefaultCoastLand},
    {"DefaultCoastChapter", OsakaGate::DefaultCoastChapter},
    {"DefaultTownLand", OsakaGate::DefaultTownLand},
};
}
std::unique_ptr<const LoadedOsakaWorld> loadOsakaWorld(const QString& folder) {
    Reader r{QDir(folder).filePath("scene.json"), {}};
    QFile f(r.file);
    if (!f.open(QIODevice::ReadOnly)) r.fail("$", "readable scene.json ("+f.errorString()+")");
    if (f.size()>1024*1024) r.fail("$", "scene.json at most 1048576 bytes");
    QJsonParseError error;
    const auto doc=QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error!=QJsonParseError::NoError)
        r.fail("$", "valid JSON (byte "+QString::number(error.offset)+": "+error.errorString()+")");
    const auto root=r.object(doc.isObject() ? QJsonValue(doc.object()) : QJsonValue(doc.array()), "$",
                             {"schema","world","profile","stages","finish","disc","mountain"});
    if (r.number(root["schema"],"$.schema")!=1) r.fail("$.schema","schema version 1");
    r.literal(root["world"],"$.world","osaka-jade");
    r.literal(root["profile"],"$.profile","osaka-world-v1");
    auto loaded=std::unique_ptr<LoadedOsakaWorld>(new LoadedOsakaWorld);
    auto& w=loaded->world_;
    const auto finish=r.object(root["finish"],"$.finish",{"profile"});
    r.literal(finish["profile"],"$.finish.profile","osaka-finish-v1");
    const auto disc=r.object(root["disc"],"$.disc",{"profile","x","y","parallax","radius"});
    r.literal(disc["profile"],"$.disc.profile","osaka-disc-v1");
    w.disc={r.number(disc["x"],"$.disc.x"),r.number(disc["y"],"$.disc.y"),r.number(disc["parallax"],"$.disc.parallax"),r.number(disc["radius"],"$.disc.radius")};
    const auto mountain=r.object(root["mountain"],"$.mountain",{"profile","x","parallax","peak","base","width"});
    r.literal(mountain["profile"],"$.mountain.profile","osaka-mountain-v1");
    w.mountain={r.number(mountain["x"],"$.mountain.x"),r.number(mountain["parallax"],"$.mountain.parallax"),r.number(mountain["peak"],"$.mountain.peak"),r.number(mountain["base"],"$.mountain.base"),r.number(mountain["width"],"$.mountain.width")};
    if (w.disc.radius<=0) r.fail("$.disc.radius","positive radius");
    if (w.mountain.width<=0) r.fail("$.mountain.width","positive width");
    if (!root["stages"].isArray() || root["stages"].toArray().size()!=4) r.fail("$.stages","four ordered stages");
    const auto stages=root["stages"].toArray();
    const char* phases[]={"Backdrop","Coast","DistantTown","Foreground"};
    OsakaRenderStage* targets[]={&w.backdrop,&w.coast,&w.distantTown,&w.foreground};
    for (int i=0;i<4;++i) {
        const QString p="$.stages["+QString::number(i)+"]";
        const auto stage=r.object(stages[i],p,{"id","phase","events","slots"});
        r.id(stage["id"],p+".id"); r.literal(stage["phase"],p+".phase",phases[i]);
        const auto events=r.string(stage["events"],p+".events");
        if (events!="Life" && events!="LifeAndFlock") r.fail(p+".events","Life or LifeAndFlock");
        if (!stage["slots"].isArray() || stage["slots"].toArray().isEmpty() || stage["slots"].toArray().size()>128) r.fail(p+".slots","array of 1..128 render slots");
        const auto slotsArray=stage["slots"].toArray();
        auto& entries=loaded->entries_[i]; entries.reserve(slotsArray.size());
        for (int j=0;j<slotsArray.size();++j) {
            const QString q=p+".slots["+QString::number(j)+"]";
            const auto slot=r.object(slotsArray[j],q,{"id","piece","gate","profile"});
            r.id(slot["id"],q+".id");
            const auto name=r.string(slot["piece"],q+".piece");
            const Piece* piece=nullptr;
            for (const auto& item:pieces) if (name==item.name) { piece=&item; break; }
            if (!piece) r.fail(q+".piece","known v1 piece");
            r.literal(slot["profile"],q+".profile",piece->profile);
            const auto gateName=r.string(slot["gate"],q+".gate");
            const Gate* gate=nullptr;
            for (const auto& item:gates) if (gateName==item.name) { gate=&item; break; }
            if (!gate) r.fail(q+".gate","known Osaka gate");
            if ((piece->op==OsakaOp::FarNetwork || piece->op==OsakaOp::NearNetwork || piece->op==OsakaOp::Birds) && events!="LifeAndFlock") r.fail(p+".events","LifeAndFlock for network or birds");
            entries.push_back({piece->op,gate->gate,piece->profile});
        }
        *targets[i]={entries.data(),entries.size(),events=="Life" ? OsakaEventRef::Life : OsakaEventRef::LifeAndFlock};
    }
    return loaded;
}
}
