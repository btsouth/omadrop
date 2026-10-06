#include "world-loader.h"
#include "piece-label.h"
#include "window-label.h"
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
    QJsonObject object(const QJsonValue& v, const QString& p, const QStringList& fields,
                       const QStringList& optional = {}) const {
        if (!v.isObject()) fail(p, "object");
        const auto o = v.toObject();
        for (auto it=o.begin(); it!=o.end(); ++it)
            if (!fields.contains(it.key()) && !optional.contains(it.key())) fail(fieldPath(p,it.key()), "known field (unknown field)");
        for (const auto& f : fields) if (!o.contains(f)) fail(fieldPath(p,f), "required field");
        return o;
    }
    Col color(const QJsonValue& v, const QString& p) const {
        const auto text = v.isString() ? v.toString() : QString();
        bool ok = text.size() == 7 && text[0] == '#';
        const int value = ok ? text.mid(1).toInt(&ok, 16) : 0;
        if (!ok) fail(p, "color string like '#1a2b3c'");
        return hex(value);
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
    {"Haze", OsakaOp::Haze, "osaka-haze-v1"},
    {"Sky", OsakaOp::Sky, "osaka-sky-v1"},
    {"GradientSky", OsakaOp::GradientSky, "gradient-sky-v1"},
    {"WaterSurface", OsakaOp::WaterSurface, "water-surface-v1"},
    {"SwellLines", OsakaOp::SwellLines, "swell-lines-v1"},
    {"FoamFlecks", OsakaOp::FoamFlecks, "foam-flecks-v1"},
    {"GreatWave", OsakaOp::GreatWave, "great-wave-v1"},
    {"BoatOnWater", OsakaOp::BoatOnWater, "boat-on-water-v1"},
    {"SmokePlume", OsakaOp::SmokePlume, "smoke-plume-v1"},
    {"PrintMoments", OsakaOp::PrintMoments, "print-moments-v1"},
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
    if (!doc.isObject()) r.fail("$","object");
    auto rootObject=doc.object();
    const bool hasNodes=rootObject.contains("nodes");
    if (!hasNodes) rootObject.insert("nodes",QJsonArray{});
    const auto root=r.object(rootObject, "$", {"schema","world","profile","stages","art","nodes"},
                             {"finish","disc","mountain","profiles"});
    if (r.number(root["schema"],"$.schema")!=1) r.fail("$.schema","schema version 1");
    const auto worldName=r.string(root["world"],"$.world");
    r.literal(root["profile"],"$.profile","osaka-world-v1");
    auto loaded=std::unique_ptr<LoadedOsakaWorld>(new LoadedOsakaWorld);
    auto& w=loaded->world_;
    const bool osaka=worldName=="osaka-jade";
    const auto art=osaka ? r.object(root["art"],"$.art",{"file","elements"})
                         : r.object(root["art"],"$.art",{"file"},{"elements"});
    const auto file=r.string(art["file"],"$.art.file");
    if(file!="art.svg")r.fail("$.art.file","art.svg in the world folder");
    const auto imported=importSvg(QDir(folder).filePath(file),1.0,pieceReplacesArt);
    if(!imported)r.fail("$.art.file",imported.diagnostic);
    w.art=imported.art;
    QSet<QString> explicitWindowIds;
    if (hasNodes) {
        if (!root["nodes"].isArray() || root["nodes"].toArray().size()>256)
            r.fail("$.nodes","array of at most 256 window nodes");
        const auto nodes=root["nodes"].toArray();
        for (int i=0;i<nodes.size();++i) {
            const QString p="$.nodes["+QString::number(i)+"]";
            const auto node=r.object(nodes[i],p,{"id","piece","profile","band","kick","onset","always"});
            const auto id=r.string(node["id"],p+".id");
            r.id(node["id"],p+".id");
            r.literal(node["piece"],p+".piece","window");
            r.literal(node["profile"],p+".profile","generic-window-v1");
            OsakaWindowNodeV1 window;
            window.id=id.toStdString();
            window.band=r.integer(node["band"],p+".band",0,5);
            window.kick=r.boolean(node["kick"],p+".kick");
            window.onset=r.boolean(node["onset"],p+".onset");
            window.always=r.boolean(node["always"],p+".always");
            window.explicitNode=true;
            bool found=false;
            for(const auto& element:w.art->elements()) if(element.id==id) {found=true;break;}
            if(!found)r.fail(p+".id","SVG element ID in "+file);
            explicitWindowIds.insert(id);
            w.windows.push_back(window);
        }
    }
    for(std::size_t index=0;index<w.art->elements().size();++index) {
        const auto& element=w.art->elements()[index];
        if(isPieceLabelCandidate(element.label)) {
            auto node=parsePieceLabel(file,element.id,element.label);
            checkPieceElement(file,element,node);
            node.element=index;
            w.pieces.push_back(node);
            continue;
        }
        if(!isWindowLabelCandidate(element.label))continue;
        if(explicitWindowIds.contains(element.id)) {
            loaded->notes_.push_back(QString("%1: element id '%2' label '%3': explicit window node overrides shorthand")
                .arg(file,element.id,element.label));
            continue;
        }
        w.windows.push_back(parseWindowLabel(file,element.id,element.label));
    }
    if(!art["elements"].isObject() && (osaka || art.contains("elements")))r.fail("$.art.elements","element binding object");
    const auto bindings=osaka
        ? r.object(art["elements"],"$.art.elements",{"near-house-shell","right-house-2-shell","right-house-3-shell","near-house-roof","near-house-eaves","right-house-2-roof","right-house-3-roof","right-house-3-eaves","near-house-lattice","izakaya-lattice","near-house-deck","street-railing","yatai-frame","izakaya-counter","laundry-line","near-house-lamp-hanger","sign-glyph-0","sign-glyph-1","sign-glyph-2","sign-glyph-3","sign-glyph-4","sign-glyph-5","sign-glyph-6","near-house-mask","shamisen-mask"})
        : art["elements"].toObject();
    for(auto it=bindings.begin();it!=bindings.end();++it) {
        const auto id=r.string(it.value(),fieldPath("$.art.elements",it.key()));
        bool found=false;for(const auto& e:w.art->elements())if(e.id==id)found=bool(e.replay)&&(e.tag=="g"||e.tag=="path"||e.tag=="rect"||e.tag=="circle");
        if(!found)r.fail(fieldPath("$.art.elements",it.key()),"SVG element ID compatible with Canvas replay");
        w.artwork.emplace(it.key().toStdString(),id);
    }
    // A world can leave out what it does not draw. A block that is present must be
    // complete. A profile block that is left out keeps the library defaults. The
    // disc and mountain placements are checked against the slots further down.
    if (root.contains("finish")) {
        const auto finish=r.object(root["finish"],"$.finish",{"profile"});
        r.literal(finish["profile"],"$.finish.profile","osaka-finish-v1");
    }
    const bool hasDisc=root.contains("disc"), hasMountain=root.contains("mountain");
    if (hasDisc) {
        const auto disc=r.object(root["disc"],"$.disc",{"profile","x","y","parallax","radius"});
        r.literal(disc["profile"],"$.disc.profile","osaka-disc-v1");
        w.disc={r.number(disc["x"],"$.disc.x"),r.number(disc["y"],"$.disc.y"),r.number(disc["parallax"],"$.disc.parallax"),r.number(disc["radius"],"$.disc.radius")};
    }
    if (hasMountain) {
        const auto mountain=r.object(root["mountain"],"$.mountain",{"profile","x","parallax","peak","base","width"});
        r.literal(mountain["profile"],"$.mountain.profile","osaka-mountain-v1");
        w.mountain={r.number(mountain["x"],"$.mountain.x"),r.number(mountain["parallax"],"$.mountain.parallax"),r.number(mountain["peak"],"$.mountain.peak"),r.number(mountain["base"],"$.mountain.base"),r.number(mountain["width"],"$.mountain.width")};
    }
    const auto profiles=root.contains("profiles")
        ? r.object(root["profiles"],"$.profiles",{},{"osaka-finish-v1","osaka-disc-v1","osaka-mountain-v1","osaka-haze-v1","osaka-sky-v1","osaka-pane-v1","osaka-neon-v1"})
        : QJsonObject{};
    if (profiles.contains("osaka-finish-v1")) {
        const QString p="$.profiles['osaka-finish-v1']";
        const auto data=r.object(profiles["osaka-finish-v1"],p,{"bloom","threshold","vignette","grain","knee","paper"});
        w.finish.defaults.bloom=r.scalar(data["bloom"],p+".bloom");
        w.finish.defaults.threshold=r.scalar(data["threshold"],p+".threshold");
        w.finish.defaults.vignette=r.scalar(data["vignette"],p+".vignette");
        w.finish.defaults.grain=r.scalar(data["grain"],p+".grain");
        w.finish.defaults.knee=r.scalar(data["knee"],p+".knee");
        w.finish.defaults.paper=r.scalar(data["paper"],p+".paper");
    }
    if (profiles.contains("osaka-disc-v1")) {
        const QString p="$.profiles['osaka-disc-v1']";
        const auto data=r.object(profiles["osaka-disc-v1"],p,{"creamHex","warmHex","colorGain","haloR","haloG","haloB","energyBase","energyBass","energySurge","energyKick","veil","texture","haloA","haloBRadius","haloC","haloD","haloFar","restRings","ring0Offset","ring0Energy","ring0Alpha","ring1Offset","ring1Energy","ring1Alpha","ringAlphaBase","hitSeconds","hitThreshold","hitOffset","hitTravel","hitAlpha","timeOffset"},{"color2Hex","ringHex","energyLift"});
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
        if (data.contains("color2Hex")) w.parameters.disc.color2Hex=r.integer(data["color2Hex"],p+".color2Hex",0,16777215);
        if (data.contains("ringHex")) w.parameters.disc.ringHex=r.integer(data["ringHex"],p+".ringHex",0,16777215);
        if (data.contains("energyLift")) w.parameters.disc.energyLift=r.number(data["energyLift"],p+".energyLift");
    }
    if (profiles.contains("osaka-mountain-v1")) {
        const QString p="$.profiles['osaka-mountain-v1']";
        const auto data=r.object(profiles["osaka-mountain-v1"],p,{"topR","topG","topB","bottomR","bottomG","bottomB","foot","samples","span","shapePower","rippleGain","ripplePeriod","summitWidth","summitOffset","summitCurve","summitHeight","gradientStop","gradientMix"},{"snow","snowScale","snowR","snowG","snowB"});
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
        auto& m=w.parameters.mountain;
        if (data.contains("snow")) m.snow=r.number(data["snow"],p+".snow");
        if (data.contains("snowScale")) m.snowScale=r.number(data["snowScale"],p+".snowScale");
        if (data.contains("snowR")) m.snowR=r.number(data["snowR"],p+".snowR");
        if (data.contains("snowG")) m.snowG=r.number(data["snowG"],p+".snowG");
        if (data.contains("snowB")) m.snowB=r.number(data["snowB"],p+".snowB");
        if (m.snow<0 || m.snow>1) r.fail(p+".snow","number in 0..1");
        if (m.snowScale<=0) r.fail(p+".snowScale","positive number");
    }
    if (profiles.contains("osaka-haze-v1")) {
        const QString p="$.profiles['osaka-haze-v1']";
        const auto data=r.object(profiles["osaka-haze-v1"],p,{"noiseX","noiseY","cullSigma"});
        w.parameters.haze.noiseX=r.number(data["noiseX"],p+".noiseX");
        if (w.parameters.haze.noiseX<=0) r.fail(p+".noiseX","positive number");
        w.parameters.haze.noiseY=r.number(data["noiseY"],p+".noiseY");
        if (w.parameters.haze.noiseY<=0) r.fail(p+".noiseY","positive number");
        w.parameters.haze.cullSigma=r.number(data["cullSigma"],p+".cullSigma");
        if (w.parameters.haze.cullSigma<=0) r.fail(p+".cullSigma","positive number");
    }
    if (profiles.contains("osaka-sky-v1")) {
        const QString p="$.profiles['osaka-sky-v1']";
        const auto data=r.object(profiles["osaka-sky-v1"],p,{"energyBase","energyBass","energySurge","timeOffset"});
        w.parameters.sky.energyBase=r.number(data["energyBase"],p+".energyBase");
        w.parameters.sky.energyBass=r.number(data["energyBass"],p+".energyBass");
        w.parameters.sky.energySurge=r.number(data["energySurge"],p+".energySurge");
        w.parameters.sky.timeOffset=r.number(data["timeOffset"],p+".timeOffset");
    }
    if (profiles.contains("osaka-pane-v1")) {
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
    if (profiles.contains("osaka-neon-v1")) {
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
    // A world lists the stages it draws, in render order. Stages left out are empty.
    if (!root["stages"].isArray() || root["stages"].toArray().isEmpty() || root["stages"].toArray().size()>4)
        r.fail("$.stages","array of 1..4 ordered stages");
    const auto stages=root["stages"].toArray();
    const QStringList phases{"Backdrop","Coast","DistantTown","Foreground"};
    OsakaRenderStage* targets[]={&w.backdrop,&w.coast,&w.distantTown,&w.foreground};
    int nextPhase=0;
    bool usesDisc=false, usesMountain=false;
    for (int i=0;i<stages.size();++i) {
        const QString p="$.stages["+QString::number(i)+"]";
        const auto stage=r.object(stages[i],p,{"id","phase","slots"},{"events"});
        r.id(stage["id"],p+".id");
        const auto phaseName=r.string(stage["phase"],p+".phase");
        if (stages.size()==4) r.literal(stage["phase"],p+".phase",phases[i]);
        const int phase=phases.indexOf(phaseName);
        if (phase<nextPhase) r.fail(p+".phase","Backdrop, Coast, DistantTown or Foreground, each at most once and in that order");
        nextPhase=phase+1;
        const auto events=stage.contains("events") ? r.string(stage["events"],p+".events") : QString("Life");
        if (events!="Life" && events!="LifeAndFlock") r.fail(p+".events","Life or LifeAndFlock");
        if (!stage["slots"].isArray() || stage["slots"].toArray().size()>128) r.fail(p+".slots","array of at most 128 render slots");
        const auto slotsArray=stage["slots"].toArray();
        auto& entries=loaded->entries_[phase]; entries.reserve(slotsArray.size());
        for (int j=0;j<slotsArray.size();++j) {
            const QString q=p+".slots["+QString::number(j)+"]";
            const auto slot=r.object(slotsArray[j],q,{"id","piece","gate","profile"},{"params"});
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
            usesDisc|=piece->op==OsakaOp::Disc;
            usesMountain|=piece->op==OsakaOp::Mountain;
            std::shared_ptr<const OsakaSlotParamsV1> params;
            const QString paramsPath=fieldPath(q,"params");
            if (piece->op==OsakaOp::GradientSky) {
                if (!slot.contains("params")) r.fail(paramsPath,"required field");
                const auto data=r.object(slot["params"],paramsPath,{"stops","paperTop","paperBottom","printGrade","grain"});
                auto value=std::make_shared<OsakaSlotParamsV1>();
                auto& sky=value->gradientSky;
                const auto list=data["stops"].toArray();
                if (!data["stops"].isArray() || list.size()<2 || list.size()>8)
                    r.fail(paramsPath+".stops","array of 2..8 ordered color stops");
                for (int k=0;k<list.size();++k) {
                    const QString q=paramsPath+".stops["+QString::number(k)+"]";
                    const auto item=r.object(list[k],q,{"y","color"});
                    const double y=r.number(item["y"],q+".y");
                    if (y<0 || y>1080 || (!sky.stops.empty() && y<=sky.stops.back().y))
                        r.fail(q+".y","strictly increasing position in 0..1080");
                    sky.stops.push_back({y,r.color(item["color"],q+".color")});
                }
                sky.paperTop=r.color(data["paperTop"],paramsPath+".paperTop");
                sky.paperBottom=r.color(data["paperBottom"],paramsPath+".paperBottom");
                sky.printGrade=r.number(data["printGrade"],paramsPath+".printGrade");
                sky.grain=r.number(data["grain"],paramsPath+".grain");
                if (sky.printGrade<0 || sky.printGrade>1) r.fail(paramsPath+".printGrade","number in 0..1");
                if (sky.grain<0 || sky.grain>0.1) r.fail(paramsPath+".grain","number in 0..0.1");
                params=value;
            } else if (piece->op==OsakaOp::WaterSurface) {
                if (!slot.contains("params")) r.fail(paramsPath,"required field");
                const auto data=r.object(slot["params"],paramsPath,{},
                    {"horizon","nearY","x0","x1","rows","textureRows","glints","seed","sampleStep",
                     "amplitude","wavelength","drift","phase","top","bottom","crest","texture","foam",
                     "underprint","glint","hotGlint","opacity","bandGain","liftGain","kickGain",
                     "capDensity","capScale","glintX","glintDepth","innerLines","crestOpacity","swellSeed","amplitudeGain","surgeEnabled"});
                auto value=std::make_shared<OsakaSlotParamsV1>();
                auto& water=value->water;
                if(data.contains("surgeEnabled"))water.surgeEnabled=r.boolean(data["surgeEnabled"],paramsPath+".surgeEnabled");
                auto scalar=[&](const char* name,double& target,double lo,double hi) {
                    if (!data.contains(name)) return;
                    target=r.number(data[name],paramsPath+"."+name);
                    if (target<lo || target>hi) r.fail(paramsPath+"."+name,"number in "+QString::number(lo)+".."+QString::number(hi));
                };
                scalar("amplitudeGain",water.amplitudeGain,0,4); scalar("horizon",water.horizon,0,1079); scalar("nearY",water.nearY,1,1200);
                scalar("x0",water.x0,-2000,3840); scalar("x1",water.x1,-2000,3840);
                scalar("sampleStep",water.sampleStep,8,128); scalar("amplitude",water.amplitude,0,1);
                scalar("wavelength",water.wavelength,.5,4); scalar("drift",water.drift,0,2);
                scalar("phase",water.phase,-1000,1000); scalar("opacity",water.opacity,0,1);
                scalar("bandGain",water.bandGain,0,3); scalar("liftGain",water.liftGain,0,1);
                scalar("kickGain",water.kickGain,0,.5); scalar("capDensity",water.capDensity,0,1);
                scalar("capScale",water.capScale,0,1.5); scalar("glintX",water.glintX,-2000,3840);
                scalar("glintDepth",water.glintDepth,1,1080);scalar("crestOpacity",water.crestOpacity,0,1);
                if (water.nearY<=water.horizon+2) r.fail(paramsPath+".nearY","position below horizon + 2");
                if (water.x1<=water.x0) r.fail(paramsPath+".x1","position right of x0");
                auto integer=[&](const char* name,int& target,int lo,int hi) {
                    if (data.contains(name)) target=r.integer(data[name],paramsPath+"."+name,lo,hi);
                };
                integer("rows",water.rows,3,24);integer("swellSeed",water.swellSeed,0,1000000);integer("innerLines",water.innerLines,0,3); integer("textureRows",water.textureRows,0,65);
                integer("glints",water.glints,0,115); integer("seed",water.seed,0,1000000);
                auto color=[&](const char* name,Col& target) { if (data.contains(name)) target=r.color(data[name],paramsPath+"."+name); };
                color("top",water.top); color("bottom",water.bottom); color("crest",water.crest);
                color("texture",water.texture); color("foam",water.foam); color("underprint",water.underprint);
                color("glint",water.glint); color("hotGlint",water.hotGlint);
                params=value;
            } else if (piece->op==OsakaOp::PrintMoments || piece->op==OsakaOp::SmokePlume) {
                const auto data=r.object(slot["params"],paramsPath,{},
                    {"x","y","width","height","scale","gain","speed","count","seed","band","surgeEnabled","waterInstance","color","accent","ink"});
                auto value=std::make_shared<OsakaSlotParamsV1>();auto& life=value->life;
                auto scalar=[&](const char* key,double& target,double lo,double hi){if(data.contains(key)){target=r.number(data[key],paramsPath+"."+key);if(target<lo || target>hi)r.fail(paramsPath+"."+key,"bounded print piece parameter");}};
                scalar("x",life.x,-1920,3840);scalar("y",life.y,0,1200);scalar("width",life.width,1,3840);scalar("height",life.height,1,1200);
                scalar("scale",life.scale,.1,3);scalar("gain",life.gain,0,4);scalar("speed",life.speed,.1,4);
                if(data.contains("count"))life.count=r.integer(data["count"],paramsPath+".count",1,30);
                if(data.contains("seed"))life.seed=r.integer(data["seed"],paramsPath+".seed",0,1000000);
                if(data.contains("band"))life.band=r.integer(data["band"],paramsPath+".band",0,5);
                if(data.contains("surgeEnabled"))life.surgeEnabled=r.boolean(data["surgeEnabled"],paramsPath+".surgeEnabled");
                if(data.contains("waterInstance"))life.waterInstance=r.string(data["waterInstance"],paramsPath+".waterInstance").toStdString();
                for(auto entry:{std::pair<const char*,Col*>{"color",&life.color},{"accent",&life.accent},{"ink",&life.ink}})
                    if(data.contains(entry.first))*entry.second=r.color(data[entry.first],paramsPath+"."+entry.first);
                params=value;
            } else if (piece->op==OsakaOp::BoatOnWater) {
                if(!slot.contains("params"))r.fail(paramsPath,"required field");
                const auto data=r.object(slot["params"],paramsPath,{"waterInstance"},
                    {"x","row","length","scale","driftX","driftRows","driftSpeed","crewCount","oarCount","seed","band",
                     "rowingTempo","tempoGain","splashGain","kickGain","hull","trim","ink","foam","surgeEnabled"});
                auto value=std::make_shared<OsakaSlotParamsV1>();auto& boat=value->boat;
                if(data.contains("surgeEnabled"))boat.surgeEnabled=r.boolean(data["surgeEnabled"],paramsPath+".surgeEnabled");
                boat.waterInstance=r.string(data["waterInstance"],paramsPath+".waterInstance").toStdString();
                auto scalar=[&](const char* name,double& target,double lo,double hi){
                    if(!data.contains(name))return;target=r.number(data[name],paramsPath+"."+name);
                    if(target<lo || target>hi)r.fail(paramsPath+"."+name,"number in "+QString::number(lo)+".."+QString::number(hi));
                };
                scalar("x",boat.x,-1920,3840);scalar("row",boat.row,0,23);scalar("length",boat.length,80,600);
                scalar("scale",boat.scale,.15,1.5);scalar("driftX",boat.driftX,0,500);scalar("driftRows",boat.driftRows,0,2);
                scalar("driftSpeed",boat.driftSpeed,0,.05);scalar("rowingTempo",boat.rowingTempo,.1,.6);
                scalar("tempoGain",boat.tempoGain,0,.5);scalar("splashGain",boat.splashGain,0,.5);scalar("kickGain",boat.kickGain,0,.8);
                auto integer=[&](const char* name,int& target,int lo,int hi){if(data.contains(name))target=r.integer(data[name],paramsPath+"."+name,lo,hi);};
                integer("crewCount",boat.crewCount,0,10);integer("oarCount",boat.oarCount,0,10);
                integer("seed",boat.seed,0,1000000);integer("band",boat.band,0,5);
                if(boat.oarCount>boat.crewCount)r.fail(paramsPath+".oarCount","at most crewCount");
                for(auto entry:{std::pair<const char*,Col*>{"hull",&boat.hull},{"trim",&boat.trim},{"ink",&boat.ink},{"foam",&boat.foam}})
                    if(data.contains(entry.first))*entry.second=r.color(data[entry.first],paramsPath+"."+entry.first);
                params=value;
            } else if (piece->op==OsakaOp::GreatWave) {
                if(!slot.contains("params"))r.fail(paramsPath,"required field");
                const auto data=r.object(slot["params"],paramsPath,{},
                    {"anchorSide","x","y","width","baseHeight","maxRise","curlAmount","clawCount","clawSize","seed",
                     "lowGain","swellGain","kickGain","onsetGain","body","bottom","underprint","foam","lines","surgeEnabled"});
                auto value=std::make_shared<OsakaSlotParamsV1>();auto& wave=value->greatWave;
                if(data.contains("surgeEnabled"))wave.surgeEnabled=r.boolean(data["surgeEnabled"],paramsPath+".surgeEnabled");
                if(data.contains("anchorSide")) {
                    const auto side=r.string(data["anchorSide"],paramsPath+".anchorSide");
                    if(side!="left" && side!="right")r.fail(paramsPath+".anchorSide","left or right");
                    wave.anchorRight=side=="right";
                }
                auto scalar=[&](const char* name,double& target,double lo,double hi){
                    if(!data.contains(name))return;target=r.number(data[name],paramsPath+"."+name);
                    if(target<lo || target>hi)r.fail(paramsPath+"."+name,"number in "+QString::number(lo)+".."+QString::number(hi));
                };
                scalar("x",wave.x,-1920,3840);scalar("y",wave.y,400,1200);scalar("width",wave.width,300,1800);
                scalar("baseHeight",wave.baseHeight,150,900);scalar("maxRise",wave.maxRise,0,800);
                if(wave.baseHeight+wave.maxRise>1050)r.fail(paramsPath,"baseHeight + maxRise at most 1050");
                scalar("curlAmount",wave.curlAmount,0,1);scalar("clawSize",wave.clawSize,.25,1.5);
                scalar("lowGain",wave.lowGain,0,4);scalar("swellGain",wave.swellGain,0,1);
                scalar("kickGain",wave.kickGain,0,.5);scalar("onsetGain",wave.onsetGain,0,.5);
                if(data.contains("clawCount"))wave.clawCount=r.integer(data["clawCount"],paramsPath+".clawCount",6,30);
                if(data.contains("seed"))wave.seed=r.integer(data["seed"],paramsPath+".seed",0,1000000);
                for(auto entry:{std::pair<const char*,Col*>{"body",&wave.body},{"bottom",&wave.bottom},
                    {"underprint",&wave.underprint},{"foam",&wave.foam},{"lines",&wave.lines}})
                    if(data.contains(entry.first))*entry.second=r.color(data[entry.first],paramsPath+"."+entry.first);
                params=value;
            } else if (piece->op==OsakaOp::SwellLines || piece->op==OsakaOp::FoamFlecks) {
                if (!slot.contains("params")) r.fail(paramsPath,"required field");
                QStringList keys={"region","exclusions","count","rows","seed","depthFalloff","widthMin","widthMax",
                     "lengthMin","lengthMax","driftSpeed","amplitude","opacity","bandGain","liftGain","kickGain","color","highlight","amplitudeGain","surgeEnabled"};
                const bool foam=piece->op==OsakaOp::FoamFlecks;
                if(foam)keys.append({"sizeMin","sizeMax","onsetGain","underprint","responseGain"});
                const auto data=r.object(slot["params"],paramsPath,{},keys);
                auto value=std::make_shared<OsakaSlotParamsV1>(); auto& swell=foam?value->foam.swell:value->swell;
                if(data.contains("surgeEnabled"))swell.surgeEnabled=r.boolean(data["surgeEnabled"],paramsPath+".surgeEnabled");
                auto box=[&](const QJsonValue& item,const QString& path) {
                    const auto v=r.object(item,path,{"x","y","width","height"});
                    const double x=r.number(v["x"],path+".x"),y=r.number(v["y"],path+".y");
                    const double w=r.number(v["width"],path+".width"),h=r.number(v["height"],path+".height");
                    if(x<-2000 || x>3840 || y<0 || y>1200 || w<=0 || w>5840 || h<=0 || h>1200)
                        r.fail(path,"bounded rectangle with positive width and height");
                    return QRectF(x,y,w,h);
                };
                if(data.contains("region"))swell.region=box(data["region"],paramsPath+".region");
                if(data.contains("exclusions")) {
                    if(!data["exclusions"].isArray() || data["exclusions"].toArray().size()>8)r.fail(paramsPath+".exclusions","array of at most 8 rectangles");
                    const auto list=data["exclusions"].toArray();
                    for(int k=0;k<list.size();++k)swell.exclusions.push_back(box(list[k],paramsPath+".exclusions["+QString::number(k)+"]"));
                }
                auto scalar=[&](const char* name,double& target,double lo,double hi) {
                    if(!data.contains(name))return;target=r.number(data[name],paramsPath+"."+name);
                    if(target<lo || target>hi)r.fail(paramsPath+"."+name,"number in "+QString::number(lo)+".."+QString::number(hi));
                };
                auto integer=[&](const char* name,int& target,int lo,int hi) {if(data.contains(name))target=r.integer(data[name],paramsPath+"."+name,lo,hi);};
                integer("count",foam?value->foam.count:swell.count,0,foam?300:600);integer("rows",swell.rows,6,24);integer("seed",swell.seed,0,1000000);
                scalar("amplitudeGain",swell.amplitudeGain,0,4);scalar("depthFalloff",swell.depthFalloff,1,3);scalar("widthMin",swell.widthMin,.2,4);scalar("widthMax",swell.widthMax,.2,6);
                scalar("lengthMin",swell.lengthMin,20,1200);scalar("lengthMax",swell.lengthMax,20,1600);
                scalar("driftSpeed",swell.driftSpeed,0,2);scalar("amplitude",swell.amplitude,0,1.5);scalar("opacity",swell.opacity,0,1);
                scalar("bandGain",swell.bandGain,0,2);scalar("liftGain",swell.liftGain,0,1);scalar("kickGain",swell.kickGain,0,.5);
                if(swell.widthMax<swell.widthMin || swell.lengthMax<swell.lengthMin)r.fail(paramsPath,"ordered width and length ranges");
                if(data.contains("color"))(foam?value->foam.color:swell.color)=r.color(data["color"],paramsPath+".color");
                if(data.contains("highlight"))swell.highlight=r.color(data["highlight"],paramsPath+".highlight");
                if(foam) {
                    scalar("responseGain",value->foam.responseGain,0,3);scalar("sizeMin",value->foam.sizeMin,.1,2);scalar("sizeMax",value->foam.sizeMax,.1,2);
                    scalar("onsetGain",value->foam.onsetGain,0,.5);
                    if(value->foam.sizeMax<value->foam.sizeMin)r.fail(paramsPath,"ordered size range");
                    if(data.contains("underprint"))value->foam.underprint=r.color(data["underprint"],paramsPath+".underprint");
                }
                params=value;
            } else if (piece->op==OsakaOp::Haze) {
                if (!slot.contains("params")) r.fail(paramsPath,"required field");
                const auto data=r.object(slot["params"],paramsPath,{"y","sigma","lo","hi","color","gain"},{"shift","drift","seed"});
                auto value=std::make_shared<OsakaSlotParamsV1>();
                auto& haze=value->haze;
                haze.y=r.number(data["y"],paramsPath+".y");
                haze.sigma=r.number(data["sigma"],paramsPath+".sigma");
                if (haze.sigma<=0) r.fail(paramsPath+".sigma","positive number");
                haze.lo=r.number(data["lo"],paramsPath+".lo");
                haze.hi=r.number(data["hi"],paramsPath+".hi");
                haze.color=r.color(data["color"],paramsPath+".color");
                haze.gain=r.number(data["gain"],paramsPath+".gain");
                if (data.contains("shift")) haze.shift=r.number(data["shift"],paramsPath+".shift");
                if (data.contains("drift")) haze.drift=r.number(data["drift"],paramsPath+".drift");
                if (data.contains("seed")) haze.seed=r.number(data["seed"],paramsPath+".seed");
                params=value;
            } else if (piece->op==OsakaOp::Ridges && slot.contains("params")) {
                const auto data=r.object(slot["params"],paramsPath,{"ridges"});
                const auto listPath=paramsPath+".ridges";
                if (!data["ridges"].isArray() || data["ridges"].toArray().isEmpty() || data["ridges"].toArray().size()>8)
                    r.fail(listPath,"array of 1..8 ridges");
                auto value=std::make_shared<OsakaSlotParamsV1>();
                const auto list=data["ridges"].toArray();
                for (int k=0;k<list.size();++k) {
                    const QString rp=listPath+"["+QString::number(k)+"]";
                    const auto ridge=r.object(list[k],rp,{"seed","base","amp","scale","parallax","top","bottom"});
                    OsakaRidgeSpecV1 spec{r.integer(ridge["seed"],rp+".seed",0,1000000),
                        r.number(ridge["base"],rp+".base"),r.number(ridge["amp"],rp+".amp"),r.number(ridge["scale"],rp+".scale"),
                        r.number(ridge["parallax"],rp+".parallax"),r.color(ridge["top"],rp+".top"),r.color(ridge["bottom"],rp+".bottom")};
                    if (spec.scale<=0) r.fail(rp+".scale","positive number");
                    value->ridges.push_back(spec);
                }
                params=value;
            } else if (slot.contains("params")) {
                r.fail(paramsPath,"known field (unknown field)");
            }
            entries.push_back({piece->op,gate->gate,piece->profile,slot["id"].toString().toStdString(),params});
        }
        *targets[phase]={entries.data(),entries.size(),events=="Life" ? OsakaEventRef::Life : OsakaEventRef::LifeAndFlock,stage["id"].toString().toStdString()};
    }
    // Resolve after all slots exist: references may cross stage/order boundaries.
    // Copy the immutable field description, not independently authored settings.
    for(auto& stage:loaded->entries_)for(auto& slot:stage)if(slot.piece==OsakaOp::BoatOnWater){
        const OsakaRenderSlot* surface=nullptr;
        for(const auto& sources:loaded->entries_)for(const auto& candidate:sources)
            if(candidate.id==slot.params->boat.waterInstance && candidate.piece==OsakaOp::SwellLines)surface=&candidate;
        const QString path="$.boat["+QString::fromStdString(slot.id)+"].params";
        if(!surface)r.fail(path+".waterInstance","id of a SwellLines slot");
        auto value=std::make_shared<OsakaSlotParamsV1>(*slot.params);
        value->boat.swell=surface->params->swell;
        if(value->boat.row-value->boat.driftRows<0 || value->boat.row+value->boat.driftRows>value->boat.swell.rows-1)
            r.fail(path+".row","row and driftRows within the named surface");
        slot.params=value;
    }
    if (usesDisc && !hasDisc) r.fail("$.disc","required field");
    if (usesMountain && !hasMountain) r.fail("$.mountain","required field");
    return loaded;
}
}
