#include "world-loader.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace Journey::Kit {
namespace {
QString fieldPath(const QString& path, const QString& field) {
    auto alpha=[](QChar c) { return (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_'; };
    bool identifier=!field.isEmpty() && alpha(field[0]);
    for (const auto c:field) identifier=identifier && (alpha(c) || (c>='0' && c<='9'));
    if (identifier) return path+"."+field;
    QString escaped=field;
    escaped.replace("\\","\\\\").replace("'","\\'").replace("\n","\\n").replace("\r","\\r").replace("\t","\\t");
    return path+"['"+escaped+"']";
}
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
            if (!fields.contains(it.key())) fail(fieldPath(p,it.key()), "known field (unknown field)");
        for (const auto& f : fields) if (!o.contains(f)) fail(fieldPath(p,f), "required field");
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
    int integer(const QJsonValue& v, const QString& p, int lo, int hi) const {
        const double n=number(v,p);
        if (n!=std::floor(n) || n<lo || n>hi) fail(p, "integer in "+QString::number(lo)+".."+QString::number(hi));
        return int(n);
    }
    float scalar(const QJsonValue& v, const QString& p) const {
        const double n=number(v,p);
        if (std::abs(n)>std::numeric_limits<float>::max()) fail(p,"finite float");
        return float(n);
    }
    bool boolean(const QJsonValue& v, const QString& p) const {
        if (!v.isBool()) fail(p,"boolean");
        return v.toBool();
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
                             {"schema","world","profile","stages","finish","disc","mountain","profiles","art"});
    if (r.number(root["schema"],"$.schema")!=1) r.fail("$.schema","schema version 1");
    r.literal(root["world"],"$.world","osaka-jade");
    r.literal(root["profile"],"$.profile","osaka-world-v1");
    auto loaded=std::unique_ptr<LoadedOsakaWorld>(new LoadedOsakaWorld);
    auto& w=loaded->world_;
    const auto art=r.object(root["art"],"$.art",{"file","elements"});
    const auto file=r.string(art["file"],"$.art.file");
    if(file!="art.svg")r.fail("$.art.file","art.svg in the world folder");
    const auto imported=importSvg(QDir(folder).filePath(file));
    if(!imported)r.fail("$.art.file",imported.diagnostic);
    w.art=imported.art;
    if(!art["elements"].isObject())r.fail("$.art.elements","element binding object");
    const auto bindings=r.object(art["elements"],"$.art.elements",{"near-house-shell","right-house-2-shell","right-house-3-shell","near-house-roof","near-house-eaves","right-house-2-roof","right-house-3-roof","right-house-3-eaves","near-house-lattice","izakaya-lattice","near-house-deck","street-railing","yatai-frame","izakaya-counter","laundry-line","near-house-lamp-hanger"});
    for(auto it=bindings.begin();it!=bindings.end();++it) {
        const auto id=r.string(it.value(),fieldPath("$.art.elements",it.key()));
        bool found=false;for(const auto& e:w.art->elements())if(e.id==id)found=bool(e.replay)&&(e.tag=="g"||e.tag=="path"||e.tag=="rect"||e.tag=="circle");
        if(!found)r.fail(fieldPath("$.art.elements",it.key()),"SVG element ID compatible with Canvas replay");
        w.artwork.emplace(it.key().toStdString(),id);
    }
    const auto finish=r.object(root["finish"],"$.finish",{"profile"});
    r.literal(finish["profile"],"$.finish.profile","osaka-finish-v1");
    const auto disc=r.object(root["disc"],"$.disc",{"profile","x","y","parallax","radius"});
    r.literal(disc["profile"],"$.disc.profile","osaka-disc-v1");
    w.disc={r.number(disc["x"],"$.disc.x"),r.number(disc["y"],"$.disc.y"),r.number(disc["parallax"],"$.disc.parallax"),r.number(disc["radius"],"$.disc.radius")};
    const auto mountain=r.object(root["mountain"],"$.mountain",{"profile","x","parallax","peak","base","width"});
    r.literal(mountain["profile"],"$.mountain.profile","osaka-mountain-v1");
    w.mountain={r.number(mountain["x"],"$.mountain.x"),r.number(mountain["parallax"],"$.mountain.parallax"),r.number(mountain["peak"],"$.mountain.peak"),r.number(mountain["base"],"$.mountain.base"),r.number(mountain["width"],"$.mountain.width")};
    const auto profiles=r.object(root["profiles"],"$.profiles",{"osaka-finish-v1","osaka-disc-v1","osaka-mountain-v1","osaka-haze-v1","osaka-sky-v1","osaka-pane-v1","osaka-neon-v1"});
    {
        const QString p="$.profiles['osaka-finish-v1']";
        const auto data=r.object(profiles["osaka-finish-v1"],p,{"bloom","threshold","vignette","grain","knee","paper"});
        w.finish.defaults.bloom=r.scalar(data["bloom"],p+".bloom");
        w.finish.defaults.threshold=r.scalar(data["threshold"],p+".threshold");
        w.finish.defaults.vignette=r.scalar(data["vignette"],p+".vignette");
        w.finish.defaults.grain=r.scalar(data["grain"],p+".grain");
        w.finish.defaults.knee=r.scalar(data["knee"],p+".knee");
        w.finish.defaults.paper=r.scalar(data["paper"],p+".paper");
    }
    {
        const QString p="$.profiles['osaka-disc-v1']";
        const auto data=r.object(profiles["osaka-disc-v1"],p,{"creamHex","warmHex","colorGain","haloR","haloG","haloB","energyBase","energyBass","energySurge","energyKick","veil","texture","haloA","haloBRadius","haloC","haloD","haloFar","restRings","ring0Offset","ring0Energy","ring0Alpha","ring1Offset","ring1Energy","ring1Alpha","ringAlphaBase","hitSeconds","hitThreshold","hitOffset","hitTravel","hitAlpha","timeOffset"});
        w.parameters.disc.creamHex=r.integer(data["creamHex"],p+".creamHex",0,16777215);
        w.parameters.disc.warmHex=r.integer(data["warmHex"],p+".warmHex",0,16777215);
        w.parameters.disc.colorGain=r.number(data["colorGain"],p+".colorGain");
        w.parameters.disc.haloR=r.number(data["haloR"],p+".haloR");
        w.parameters.disc.haloG=r.number(data["haloG"],p+".haloG");
        w.parameters.disc.haloB=r.number(data["haloB"],p+".haloB");
        w.parameters.disc.energyBase=r.number(data["energyBase"],p+".energyBase");
        w.parameters.disc.energyBass=r.number(data["energyBass"],p+".energyBass");
        w.parameters.disc.energySurge=r.number(data["energySurge"],p+".energySurge");
        w.parameters.disc.energyKick=r.number(data["energyKick"],p+".energyKick");
        w.parameters.disc.veil=r.number(data["veil"],p+".veil");
        w.parameters.disc.texture=r.number(data["texture"],p+".texture");
        w.parameters.disc.haloA=r.number(data["haloA"],p+".haloA");
        w.parameters.disc.haloBRadius=r.number(data["haloBRadius"],p+".haloBRadius");
        w.parameters.disc.haloC=r.number(data["haloC"],p+".haloC");
        w.parameters.disc.haloD=r.number(data["haloD"],p+".haloD");
        w.parameters.disc.haloFar=r.number(data["haloFar"],p+".haloFar");
        w.parameters.disc.restRings=r.number(data["restRings"],p+".restRings");
        w.parameters.disc.ring0Offset=r.number(data["ring0Offset"],p+".ring0Offset");
        w.parameters.disc.ring0Energy=r.number(data["ring0Energy"],p+".ring0Energy");
        w.parameters.disc.ring0Alpha=r.number(data["ring0Alpha"],p+".ring0Alpha");
        w.parameters.disc.ring1Offset=r.number(data["ring1Offset"],p+".ring1Offset");
        w.parameters.disc.ring1Energy=r.number(data["ring1Energy"],p+".ring1Energy");
        w.parameters.disc.ring1Alpha=r.number(data["ring1Alpha"],p+".ring1Alpha");
        w.parameters.disc.ringAlphaBase=r.number(data["ringAlphaBase"],p+".ringAlphaBase");
        w.parameters.disc.hitSeconds=r.number(data["hitSeconds"],p+".hitSeconds");
        if (w.parameters.disc.hitSeconds<=0) r.fail(p+".hitSeconds","positive number");
        w.parameters.disc.hitThreshold=r.number(data["hitThreshold"],p+".hitThreshold");
        w.parameters.disc.hitOffset=r.number(data["hitOffset"],p+".hitOffset");
        w.parameters.disc.hitTravel=r.number(data["hitTravel"],p+".hitTravel");
        w.parameters.disc.hitAlpha=r.number(data["hitAlpha"],p+".hitAlpha");
        w.parameters.disc.timeOffset=r.number(data["timeOffset"],p+".timeOffset");
    }
    {
        const QString p="$.profiles['osaka-mountain-v1']";
        const auto data=r.object(profiles["osaka-mountain-v1"],p,{"topR","topG","topB","bottomR","bottomG","bottomB","foot","samples","span","shapePower","rippleGain","ripplePeriod","summitWidth","summitOffset","summitCurve","summitHeight","gradientStop","gradientMix"});
        w.parameters.mountain.topR=r.number(data["topR"],p+".topR");
        w.parameters.mountain.topG=r.number(data["topG"],p+".topG");
        w.parameters.mountain.topB=r.number(data["topB"],p+".topB");
        w.parameters.mountain.bottomR=r.number(data["bottomR"],p+".bottomR");
        w.parameters.mountain.bottomG=r.number(data["bottomG"],p+".bottomG");
        w.parameters.mountain.bottomB=r.number(data["bottomB"],p+".bottomB");
        w.parameters.mountain.foot=r.number(data["foot"],p+".foot");
        w.parameters.mountain.samples=r.integer(data["samples"],p+".samples",1,4096);
        w.parameters.mountain.span=r.number(data["span"],p+".span");
        if (w.parameters.mountain.span<=0) r.fail(p+".span","positive number");
        w.parameters.mountain.shapePower=r.number(data["shapePower"],p+".shapePower");
        w.parameters.mountain.rippleGain=r.number(data["rippleGain"],p+".rippleGain");
        w.parameters.mountain.ripplePeriod=r.number(data["ripplePeriod"],p+".ripplePeriod");
        if (w.parameters.mountain.ripplePeriod<=0) r.fail(p+".ripplePeriod","positive number");
        w.parameters.mountain.summitWidth=r.number(data["summitWidth"],p+".summitWidth");
        if (w.parameters.mountain.summitWidth<=0) r.fail(p+".summitWidth","positive number");
        w.parameters.mountain.summitOffset=r.number(data["summitOffset"],p+".summitOffset");
        w.parameters.mountain.summitCurve=r.number(data["summitCurve"],p+".summitCurve");
        w.parameters.mountain.summitHeight=r.number(data["summitHeight"],p+".summitHeight");
        if (w.parameters.mountain.summitHeight<=0) r.fail(p+".summitHeight","positive number");
        w.parameters.mountain.gradientStop=r.number(data["gradientStop"],p+".gradientStop");
        w.parameters.mountain.gradientMix=r.number(data["gradientMix"],p+".gradientMix");
    }
    {
        const QString p="$.profiles['osaka-haze-v1']";
        const auto data=r.object(profiles["osaka-haze-v1"],p,{"noiseX","noiseY","cullSigma"});
        w.parameters.haze.noiseX=r.number(data["noiseX"],p+".noiseX");
        if (w.parameters.haze.noiseX<=0) r.fail(p+".noiseX","positive number");
        w.parameters.haze.noiseY=r.number(data["noiseY"],p+".noiseY");
        if (w.parameters.haze.noiseY<=0) r.fail(p+".noiseY","positive number");
        w.parameters.haze.cullSigma=r.number(data["cullSigma"],p+".cullSigma");
        if (w.parameters.haze.cullSigma<=0) r.fail(p+".cullSigma","positive number");
    }
    {
        const QString p="$.profiles['osaka-sky-v1']";
        const auto data=r.object(profiles["osaka-sky-v1"],p,{"energyBase","energyBass","energySurge","timeOffset"});
        w.parameters.sky.energyBase=r.number(data["energyBase"],p+".energyBase");
        w.parameters.sky.energyBass=r.number(data["energyBass"],p+".energyBass");
        w.parameters.sky.energySurge=r.number(data["energySurge"],p+".energySurge");
        w.parameters.sky.timeOffset=r.number(data["timeOffset"],p+".timeOffset");
    }
    {
        const QString p="$.profiles['osaka-pane-v1']";
        const auto data=r.object(profiles["osaka-pane-v1"],p,{"alwaysOnCutoff","base","hush","band","lift","kick","near","upper","right"});
        w.parameters.windows.alwaysOnCutoff=r.number(data["alwaysOnCutoff"],p+".alwaysOnCutoff");
        w.parameters.windows.base=r.number(data["base"],p+".base");
        w.parameters.windows.hush=r.number(data["hush"],p+".hush");
        w.parameters.windows.band=r.number(data["band"],p+".band");
        w.parameters.windows.lift=r.number(data["lift"],p+".lift");
        w.parameters.windows.kick=r.number(data["kick"],p+".kick");
        if (!data["near"].isArray() || data["near"].toArray().size()!=4) r.fail(p+".near","array of 4 panes");
        for (int i=0;i<4;++i) {
            const QString q=p+".near["+QString::number(i)+"]";
            const auto item=r.object(data["near"].toArray()[i],q,{"x","y","w","h","cols","rows","on","band"});
            w.parameters.windows.near[i].x=r.number(item["x"],q+".x");
            w.parameters.windows.near[i].y=r.number(item["y"],q+".y");
            w.parameters.windows.near[i].w=r.number(item["w"],q+".w");
            w.parameters.windows.near[i].h=r.number(item["h"],q+".h");
            w.parameters.windows.near[i].cols=r.integer(item["cols"],q+".cols",1,64);
            w.parameters.windows.near[i].rows=r.integer(item["rows"],q+".rows",1,64);
            w.parameters.windows.near[i].on=r.number(item["on"],q+".on");
            w.parameters.windows.near[i].band=r.integer(item["band"],q+".band",0,5);
        }
        if (!data["upper"].isArray() || data["upper"].toArray().size()!=4) r.fail(p+".upper","array of 4 panes");
        for (int i=0;i<4;++i) {
            const QString q=p+".upper["+QString::number(i)+"]";
            const auto item=r.object(data["upper"].toArray()[i],q,{"wx","ww","cyan","on","band"});
            w.parameters.windows.upper[i].wx=r.number(item["wx"],q+".wx");
            w.parameters.windows.upper[i].ww=r.number(item["ww"],q+".ww");
            w.parameters.windows.upper[i].cyan=r.boolean(item["cyan"],q+".cyan");
            w.parameters.windows.upper[i].on=r.number(item["on"],q+".on");
            w.parameters.windows.upper[i].band=r.integer(item["band"],q+".band",0,5);
        }
        if (!data["right"].isArray() || data["right"].toArray().size()!=3) r.fail(p+".right","array of 3 panes");
        for (int i=0;i<3;++i) {
            const QString q=p+".right["+QString::number(i)+"]";
            const auto item=r.object(data["right"].toArray()[i],q,{"x","y","w","h","on"});
            w.parameters.windows.right[i].x=r.number(item["x"],q+".x");
            w.parameters.windows.right[i].y=r.number(item["y"],q+".y");
            w.parameters.windows.right[i].w=r.number(item["w"],q+".w");
            w.parameters.windows.right[i].h=r.number(item["h"],q+".h");
            w.parameters.windows.right[i].on=r.number(item["on"],q+".on");
        }
    }
    {
        const QString p="$.profiles['osaka-neon-v1']";
        const auto data=r.object(profiles["osaka-neon-v1"],p,{"stutterRate","stutterProbability","stutterLevel","boardX","boardY","boardW","boardH","boardR","boardG","boardB","tubeR","tubeG","tubeB","tubeMix","magHex","outlineAlpha","outlineX","outlineY","outlineW","outlineH","outlineWidth","glyphCount","glyphX","glyphY","glyphStep","glyphSize","glowX","glowY","glowRadius","glowBase","glowKick","levelBase","levelSine","levelRate","levelKick","overGain","addGain","addBlur"});
        w.parameters.signs.stutterRate=r.number(data["stutterRate"],p+".stutterRate");
        w.parameters.signs.stutterProbability=r.number(data["stutterProbability"],p+".stutterProbability");
        w.parameters.signs.stutterLevel=r.number(data["stutterLevel"],p+".stutterLevel");
        w.parameters.signs.boardX=r.number(data["boardX"],p+".boardX");
        w.parameters.signs.boardY=r.number(data["boardY"],p+".boardY");
        w.parameters.signs.boardW=r.number(data["boardW"],p+".boardW");
        w.parameters.signs.boardH=r.number(data["boardH"],p+".boardH");
        w.parameters.signs.boardR=r.number(data["boardR"],p+".boardR");
        w.parameters.signs.boardG=r.number(data["boardG"],p+".boardG");
        w.parameters.signs.boardB=r.number(data["boardB"],p+".boardB");
        w.parameters.signs.tubeR=r.number(data["tubeR"],p+".tubeR");
        w.parameters.signs.tubeG=r.number(data["tubeG"],p+".tubeG");
        w.parameters.signs.tubeB=r.number(data["tubeB"],p+".tubeB");
        w.parameters.signs.tubeMix=r.number(data["tubeMix"],p+".tubeMix");
        w.parameters.signs.magHex=r.integer(data["magHex"],p+".magHex",0,16777215);
        w.parameters.signs.outlineAlpha=r.number(data["outlineAlpha"],p+".outlineAlpha");
        w.parameters.signs.outlineX=r.number(data["outlineX"],p+".outlineX");
        w.parameters.signs.outlineY=r.number(data["outlineY"],p+".outlineY");
        w.parameters.signs.outlineW=r.number(data["outlineW"],p+".outlineW");
        w.parameters.signs.outlineH=r.number(data["outlineH"],p+".outlineH");
        w.parameters.signs.outlineWidth=r.number(data["outlineWidth"],p+".outlineWidth");
        w.parameters.signs.glyphCount=r.integer(data["glyphCount"],p+".glyphCount",1,3);
        w.parameters.signs.glyphX=r.number(data["glyphX"],p+".glyphX");
        w.parameters.signs.glyphY=r.number(data["glyphY"],p+".glyphY");
        w.parameters.signs.glyphStep=r.number(data["glyphStep"],p+".glyphStep");
        w.parameters.signs.glyphSize=r.number(data["glyphSize"],p+".glyphSize");
        w.parameters.signs.glowX=r.number(data["glowX"],p+".glowX");
        w.parameters.signs.glowY=r.number(data["glowY"],p+".glowY");
        w.parameters.signs.glowRadius=r.number(data["glowRadius"],p+".glowRadius");
        w.parameters.signs.glowBase=r.number(data["glowBase"],p+".glowBase");
        w.parameters.signs.glowKick=r.number(data["glowKick"],p+".glowKick");
        w.parameters.signs.levelBase=r.number(data["levelBase"],p+".levelBase");
        w.parameters.signs.levelSine=r.number(data["levelSine"],p+".levelSine");
        w.parameters.signs.levelRate=r.number(data["levelRate"],p+".levelRate");
        w.parameters.signs.levelKick=r.number(data["levelKick"],p+".levelKick");
        w.parameters.signs.overGain=r.number(data["overGain"],p+".overGain");
        w.parameters.signs.addGain=r.number(data["addGain"],p+".addGain");
        w.parameters.signs.addBlur=r.number(data["addBlur"],p+".addBlur");
    }
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
            entries.push_back({piece->op,gate->gate,piece->profile,slot["id"].toString().toStdString()});
        }
        *targets[i]={entries.data(),entries.size(),events=="Life" ? OsakaEventRef::Life : OsakaEventRef::LifeAndFlock,stage["id"].toString().toStdString()};
    }
    return loaded;
}
}
