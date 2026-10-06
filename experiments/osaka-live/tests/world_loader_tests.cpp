#include "../src/kit/world-loader.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
using namespace Journey::Kit;
namespace Journey::Kit { const OsakaWorldDescription& osakaOracle(); }
static void require(bool ok, const char* msg) { if (!ok) throw std::runtime_error(msg); }
int main(int argc, char** argv) {
    QCoreApplication app(argc,argv);
    try {
        const QString folder=QStringLiteral(OSAKA_WORLD_FOLDER);
        const auto loaded=loadOsakaWorld(folder);
        const auto& a=loaded->description(); const auto& b=osakaOracle();
        const OsakaRenderStage* aa[]={&a.backdrop,&a.coast,&a.distantTown,&a.foreground};
        const OsakaRenderStage* bb[]={&b.backdrop,&b.coast,&b.distantTown,&b.foreground};
        for (int i=0;i<4;++i) {
            require(aa[i]->count==bb[i]->count && aa[i]->events==bb[i]->events && aa[i]->id==bb[i]->id,"stage differs from compiled oracle");
            for (std::size_t j=0;j<aa[i]->count;++j) {
                const auto& x=aa[i]->entries[j]; const auto& y=bb[i]->entries[j];
                require(x.piece==y.piece && x.gate==y.gate && std::string(x.profile)==y.profile && x.id==y.id,"slot differs from compiled oracle");
            }
        }
        require(a.disc.x==b.disc.x && a.disc.y==b.disc.y && a.disc.parallax==b.disc.parallax && a.disc.radius==b.disc.radius,"disc differs");
        require(a.mountain.x==b.mountain.x && a.mountain.parallax==b.mountain.parallax && a.mountain.peak==b.mountain.peak && a.mountain.base==b.mountain.base && a.mountain.width==b.mountain.width,"mountain differs");
        const auto& f=a.finish.defaults; const auto& g=b.finish.defaults;
        require(f.bloom==g.bloom && f.threshold==g.threshold && f.vignette==g.vignette && f.grain==g.grain && f.knee==g.knee && f.paper==g.paper && f.time==g.time,"finish differs");
        require(a.parameters.disc.creamHex==b.parameters.disc.creamHex,"disc.creamHex differs from compiled oracle");
        require(a.parameters.disc.warmHex==b.parameters.disc.warmHex,"disc.warmHex differs from compiled oracle");
        require(a.parameters.disc.colorGain==b.parameters.disc.colorGain,"disc.colorGain differs from compiled oracle");
        require(a.parameters.disc.haloR==b.parameters.disc.haloR,"disc.haloR differs from compiled oracle");
        require(a.parameters.disc.haloG==b.parameters.disc.haloG,"disc.haloG differs from compiled oracle");
        require(a.parameters.disc.haloB==b.parameters.disc.haloB,"disc.haloB differs from compiled oracle");
        require(a.parameters.disc.energyBase==b.parameters.disc.energyBase,"disc.energyBase differs from compiled oracle");
        require(a.parameters.disc.energyBass==b.parameters.disc.energyBass,"disc.energyBass differs from compiled oracle");
        require(a.parameters.disc.energySurge==b.parameters.disc.energySurge,"disc.energySurge differs from compiled oracle");
        require(a.parameters.disc.energyKick==b.parameters.disc.energyKick,"disc.energyKick differs from compiled oracle");
        require(a.parameters.disc.veil==b.parameters.disc.veil,"disc.veil differs from compiled oracle");
        require(a.parameters.disc.texture==b.parameters.disc.texture,"disc.texture differs from compiled oracle");
        require(a.parameters.disc.haloA==b.parameters.disc.haloA,"disc.haloA differs from compiled oracle");
        require(a.parameters.disc.haloBRadius==b.parameters.disc.haloBRadius,"disc.haloBRadius differs from compiled oracle");
        require(a.parameters.disc.haloC==b.parameters.disc.haloC,"disc.haloC differs from compiled oracle");
        require(a.parameters.disc.haloD==b.parameters.disc.haloD,"disc.haloD differs from compiled oracle");
        require(a.parameters.disc.haloFar==b.parameters.disc.haloFar,"disc.haloFar differs from compiled oracle");
        require(a.parameters.disc.restRings==b.parameters.disc.restRings,"disc.restRings differs from compiled oracle");
        require(a.parameters.disc.ring0Offset==b.parameters.disc.ring0Offset,"disc.ring0Offset differs from compiled oracle");
        require(a.parameters.disc.ring0Energy==b.parameters.disc.ring0Energy,"disc.ring0Energy differs from compiled oracle");
        require(a.parameters.disc.ring0Alpha==b.parameters.disc.ring0Alpha,"disc.ring0Alpha differs from compiled oracle");
        require(a.parameters.disc.ring1Offset==b.parameters.disc.ring1Offset,"disc.ring1Offset differs from compiled oracle");
        require(a.parameters.disc.ring1Energy==b.parameters.disc.ring1Energy,"disc.ring1Energy differs from compiled oracle");
        require(a.parameters.disc.ring1Alpha==b.parameters.disc.ring1Alpha,"disc.ring1Alpha differs from compiled oracle");
        require(a.parameters.disc.ringAlphaBase==b.parameters.disc.ringAlphaBase,"disc.ringAlphaBase differs from compiled oracle");
        require(a.parameters.disc.hitSeconds==b.parameters.disc.hitSeconds,"disc.hitSeconds differs from compiled oracle");
        require(a.parameters.disc.hitThreshold==b.parameters.disc.hitThreshold,"disc.hitThreshold differs from compiled oracle");
        require(a.parameters.disc.hitOffset==b.parameters.disc.hitOffset,"disc.hitOffset differs from compiled oracle");
        require(a.parameters.disc.hitTravel==b.parameters.disc.hitTravel,"disc.hitTravel differs from compiled oracle");
        require(a.parameters.disc.hitAlpha==b.parameters.disc.hitAlpha,"disc.hitAlpha differs from compiled oracle");
        require(a.parameters.disc.timeOffset==b.parameters.disc.timeOffset,"disc.timeOffset differs from compiled oracle");
        require(a.parameters.mountain.topR==b.parameters.mountain.topR,"mountain.topR differs from compiled oracle");
        require(a.parameters.mountain.topG==b.parameters.mountain.topG,"mountain.topG differs from compiled oracle");
        require(a.parameters.mountain.topB==b.parameters.mountain.topB,"mountain.topB differs from compiled oracle");
        require(a.parameters.mountain.bottomR==b.parameters.mountain.bottomR,"mountain.bottomR differs from compiled oracle");
        require(a.parameters.mountain.bottomG==b.parameters.mountain.bottomG,"mountain.bottomG differs from compiled oracle");
        require(a.parameters.mountain.bottomB==b.parameters.mountain.bottomB,"mountain.bottomB differs from compiled oracle");
        require(a.parameters.mountain.foot==b.parameters.mountain.foot,"mountain.foot differs from compiled oracle");
        require(a.parameters.mountain.samples==b.parameters.mountain.samples,"mountain.samples differs from compiled oracle");
        require(a.parameters.mountain.span==b.parameters.mountain.span,"mountain.span differs from compiled oracle");
        require(a.parameters.mountain.shapePower==b.parameters.mountain.shapePower,"mountain.shapePower differs from compiled oracle");
        require(a.parameters.mountain.rippleGain==b.parameters.mountain.rippleGain,"mountain.rippleGain differs from compiled oracle");
        require(a.parameters.mountain.ripplePeriod==b.parameters.mountain.ripplePeriod,"mountain.ripplePeriod differs from compiled oracle");
        require(a.parameters.mountain.summitWidth==b.parameters.mountain.summitWidth,"mountain.summitWidth differs from compiled oracle");
        require(a.parameters.mountain.summitOffset==b.parameters.mountain.summitOffset,"mountain.summitOffset differs from compiled oracle");
        require(a.parameters.mountain.summitCurve==b.parameters.mountain.summitCurve,"mountain.summitCurve differs from compiled oracle");
        require(a.parameters.mountain.summitHeight==b.parameters.mountain.summitHeight,"mountain.summitHeight differs from compiled oracle");
        require(a.parameters.mountain.gradientStop==b.parameters.mountain.gradientStop,"mountain.gradientStop differs from compiled oracle");
        require(a.parameters.mountain.gradientMix==b.parameters.mountain.gradientMix,"mountain.gradientMix differs from compiled oracle");
        require(a.parameters.haze.noiseX==b.parameters.haze.noiseX,"haze.noiseX differs from compiled oracle");
        require(a.parameters.haze.noiseY==b.parameters.haze.noiseY,"haze.noiseY differs from compiled oracle");
        require(a.parameters.haze.cullSigma==b.parameters.haze.cullSigma,"haze.cullSigma differs from compiled oracle");
        require(a.parameters.sky.energyBase==b.parameters.sky.energyBase,"sky.energyBase differs from compiled oracle");
        require(a.parameters.sky.energyBass==b.parameters.sky.energyBass,"sky.energyBass differs from compiled oracle");
        require(a.parameters.sky.energySurge==b.parameters.sky.energySurge,"sky.energySurge differs from compiled oracle");
        require(a.parameters.sky.timeOffset==b.parameters.sky.timeOffset,"sky.timeOffset differs from compiled oracle");
        require(a.parameters.windows.alwaysOnCutoff==b.parameters.windows.alwaysOnCutoff,"windows.alwaysOnCutoff differs from compiled oracle");
        require(a.parameters.windows.base==b.parameters.windows.base,"windows.base differs from compiled oracle");
        require(a.parameters.windows.hush==b.parameters.windows.hush,"windows.hush differs from compiled oracle");
        require(a.parameters.windows.band==b.parameters.windows.band,"windows.band differs from compiled oracle");
        require(a.parameters.windows.lift==b.parameters.windows.lift,"windows.lift differs from compiled oracle");
        require(a.parameters.windows.kick==b.parameters.windows.kick,"windows.kick differs from compiled oracle");
        require(a.parameters.signs.stutterRate==b.parameters.signs.stutterRate,"signs.stutterRate differs from compiled oracle");
        require(a.parameters.signs.stutterProbability==b.parameters.signs.stutterProbability,"signs.stutterProbability differs from compiled oracle");
        require(a.parameters.signs.stutterLevel==b.parameters.signs.stutterLevel,"signs.stutterLevel differs from compiled oracle");
        require(a.parameters.signs.boardX==b.parameters.signs.boardX,"signs.boardX differs from compiled oracle");
        require(a.parameters.signs.boardY==b.parameters.signs.boardY,"signs.boardY differs from compiled oracle");
        require(a.parameters.signs.boardW==b.parameters.signs.boardW,"signs.boardW differs from compiled oracle");
        require(a.parameters.signs.boardH==b.parameters.signs.boardH,"signs.boardH differs from compiled oracle");
        require(a.parameters.signs.boardR==b.parameters.signs.boardR,"signs.boardR differs from compiled oracle");
        require(a.parameters.signs.boardG==b.parameters.signs.boardG,"signs.boardG differs from compiled oracle");
        require(a.parameters.signs.boardB==b.parameters.signs.boardB,"signs.boardB differs from compiled oracle");
        require(a.parameters.signs.tubeR==b.parameters.signs.tubeR,"signs.tubeR differs from compiled oracle");
        require(a.parameters.signs.tubeG==b.parameters.signs.tubeG,"signs.tubeG differs from compiled oracle");
        require(a.parameters.signs.tubeB==b.parameters.signs.tubeB,"signs.tubeB differs from compiled oracle");
        require(a.parameters.signs.tubeMix==b.parameters.signs.tubeMix,"signs.tubeMix differs from compiled oracle");
        require(a.parameters.signs.magHex==b.parameters.signs.magHex,"signs.magHex differs from compiled oracle");
        require(a.parameters.signs.outlineAlpha==b.parameters.signs.outlineAlpha,"signs.outlineAlpha differs from compiled oracle");
        require(a.parameters.signs.outlineX==b.parameters.signs.outlineX,"signs.outlineX differs from compiled oracle");
        require(a.parameters.signs.outlineY==b.parameters.signs.outlineY,"signs.outlineY differs from compiled oracle");
        require(a.parameters.signs.outlineW==b.parameters.signs.outlineW,"signs.outlineW differs from compiled oracle");
        require(a.parameters.signs.outlineH==b.parameters.signs.outlineH,"signs.outlineH differs from compiled oracle");
        require(a.parameters.signs.outlineWidth==b.parameters.signs.outlineWidth,"signs.outlineWidth differs from compiled oracle");
        require(a.parameters.signs.glyphCount==b.parameters.signs.glyphCount,"signs.glyphCount differs from compiled oracle");
        require(a.parameters.signs.glyphX==b.parameters.signs.glyphX,"signs.glyphX differs from compiled oracle");
        require(a.parameters.signs.glyphY==b.parameters.signs.glyphY,"signs.glyphY differs from compiled oracle");
        require(a.parameters.signs.glyphStep==b.parameters.signs.glyphStep,"signs.glyphStep differs from compiled oracle");
        require(a.parameters.signs.glyphSize==b.parameters.signs.glyphSize,"signs.glyphSize differs from compiled oracle");
        require(a.parameters.signs.glowX==b.parameters.signs.glowX,"signs.glowX differs from compiled oracle");
        require(a.parameters.signs.glowY==b.parameters.signs.glowY,"signs.glowY differs from compiled oracle");
        require(a.parameters.signs.glowRadius==b.parameters.signs.glowRadius,"signs.glowRadius differs from compiled oracle");
        require(a.parameters.signs.glowBase==b.parameters.signs.glowBase,"signs.glowBase differs from compiled oracle");
        require(a.parameters.signs.glowKick==b.parameters.signs.glowKick,"signs.glowKick differs from compiled oracle");
        require(a.parameters.signs.levelBase==b.parameters.signs.levelBase,"signs.levelBase differs from compiled oracle");
        require(a.parameters.signs.levelSine==b.parameters.signs.levelSine,"signs.levelSine differs from compiled oracle");
        require(a.parameters.signs.levelRate==b.parameters.signs.levelRate,"signs.levelRate differs from compiled oracle");
        require(a.parameters.signs.levelKick==b.parameters.signs.levelKick,"signs.levelKick differs from compiled oracle");
        require(a.parameters.signs.overGain==b.parameters.signs.overGain,"signs.overGain differs from compiled oracle");
        require(a.parameters.signs.addGain==b.parameters.signs.addGain,"signs.addGain differs from compiled oracle");
        require(a.parameters.signs.addBlur==b.parameters.signs.addBlur,"signs.addBlur differs from compiled oracle");
        require(a.parameters.windows.near[0].x==b.parameters.windows.near[0].x,"windows.near[0].x differs");
        require(a.parameters.windows.near[0].y==b.parameters.windows.near[0].y,"windows.near[0].y differs");
        require(a.parameters.windows.near[0].w==b.parameters.windows.near[0].w,"windows.near[0].w differs");
        require(a.parameters.windows.near[0].h==b.parameters.windows.near[0].h,"windows.near[0].h differs");
        require(a.parameters.windows.near[0].cols==b.parameters.windows.near[0].cols,"windows.near[0].cols differs");
        require(a.parameters.windows.near[0].rows==b.parameters.windows.near[0].rows,"windows.near[0].rows differs");
        require(a.parameters.windows.near[0].on==b.parameters.windows.near[0].on,"windows.near[0].on differs");
        require(a.parameters.windows.near[0].band==b.parameters.windows.near[0].band,"windows.near[0].band differs");
        require(a.parameters.windows.near[1].x==b.parameters.windows.near[1].x,"windows.near[1].x differs");
        require(a.parameters.windows.near[1].y==b.parameters.windows.near[1].y,"windows.near[1].y differs");
        require(a.parameters.windows.near[1].w==b.parameters.windows.near[1].w,"windows.near[1].w differs");
        require(a.parameters.windows.near[1].h==b.parameters.windows.near[1].h,"windows.near[1].h differs");
        require(a.parameters.windows.near[1].cols==b.parameters.windows.near[1].cols,"windows.near[1].cols differs");
        require(a.parameters.windows.near[1].rows==b.parameters.windows.near[1].rows,"windows.near[1].rows differs");
        require(a.parameters.windows.near[1].on==b.parameters.windows.near[1].on,"windows.near[1].on differs");
        require(a.parameters.windows.near[1].band==b.parameters.windows.near[1].band,"windows.near[1].band differs");
        require(a.parameters.windows.near[2].x==b.parameters.windows.near[2].x,"windows.near[2].x differs");
        require(a.parameters.windows.near[2].y==b.parameters.windows.near[2].y,"windows.near[2].y differs");
        require(a.parameters.windows.near[2].w==b.parameters.windows.near[2].w,"windows.near[2].w differs");
        require(a.parameters.windows.near[2].h==b.parameters.windows.near[2].h,"windows.near[2].h differs");
        require(a.parameters.windows.near[2].cols==b.parameters.windows.near[2].cols,"windows.near[2].cols differs");
        require(a.parameters.windows.near[2].rows==b.parameters.windows.near[2].rows,"windows.near[2].rows differs");
        require(a.parameters.windows.near[2].on==b.parameters.windows.near[2].on,"windows.near[2].on differs");
        require(a.parameters.windows.near[2].band==b.parameters.windows.near[2].band,"windows.near[2].band differs");
        require(a.parameters.windows.near[3].x==b.parameters.windows.near[3].x,"windows.near[3].x differs");
        require(a.parameters.windows.near[3].y==b.parameters.windows.near[3].y,"windows.near[3].y differs");
        require(a.parameters.windows.near[3].w==b.parameters.windows.near[3].w,"windows.near[3].w differs");
        require(a.parameters.windows.near[3].h==b.parameters.windows.near[3].h,"windows.near[3].h differs");
        require(a.parameters.windows.near[3].cols==b.parameters.windows.near[3].cols,"windows.near[3].cols differs");
        require(a.parameters.windows.near[3].rows==b.parameters.windows.near[3].rows,"windows.near[3].rows differs");
        require(a.parameters.windows.near[3].on==b.parameters.windows.near[3].on,"windows.near[3].on differs");
        require(a.parameters.windows.near[3].band==b.parameters.windows.near[3].band,"windows.near[3].band differs");
        require(a.parameters.windows.upper[0].wx==b.parameters.windows.upper[0].wx,"windows.upper[0].wx differs");
        require(a.parameters.windows.upper[0].ww==b.parameters.windows.upper[0].ww,"windows.upper[0].ww differs");
        require(a.parameters.windows.upper[0].cyan==b.parameters.windows.upper[0].cyan,"windows.upper[0].cyan differs");
        require(a.parameters.windows.upper[0].on==b.parameters.windows.upper[0].on,"windows.upper[0].on differs");
        require(a.parameters.windows.upper[0].band==b.parameters.windows.upper[0].band,"windows.upper[0].band differs");
        require(a.parameters.windows.upper[1].wx==b.parameters.windows.upper[1].wx,"windows.upper[1].wx differs");
        require(a.parameters.windows.upper[1].ww==b.parameters.windows.upper[1].ww,"windows.upper[1].ww differs");
        require(a.parameters.windows.upper[1].cyan==b.parameters.windows.upper[1].cyan,"windows.upper[1].cyan differs");
        require(a.parameters.windows.upper[1].on==b.parameters.windows.upper[1].on,"windows.upper[1].on differs");
        require(a.parameters.windows.upper[1].band==b.parameters.windows.upper[1].band,"windows.upper[1].band differs");
        require(a.parameters.windows.upper[2].wx==b.parameters.windows.upper[2].wx,"windows.upper[2].wx differs");
        require(a.parameters.windows.upper[2].ww==b.parameters.windows.upper[2].ww,"windows.upper[2].ww differs");
        require(a.parameters.windows.upper[2].cyan==b.parameters.windows.upper[2].cyan,"windows.upper[2].cyan differs");
        require(a.parameters.windows.upper[2].on==b.parameters.windows.upper[2].on,"windows.upper[2].on differs");
        require(a.parameters.windows.upper[2].band==b.parameters.windows.upper[2].band,"windows.upper[2].band differs");
        require(a.parameters.windows.upper[3].wx==b.parameters.windows.upper[3].wx,"windows.upper[3].wx differs");
        require(a.parameters.windows.upper[3].ww==b.parameters.windows.upper[3].ww,"windows.upper[3].ww differs");
        require(a.parameters.windows.upper[3].cyan==b.parameters.windows.upper[3].cyan,"windows.upper[3].cyan differs");
        require(a.parameters.windows.upper[3].on==b.parameters.windows.upper[3].on,"windows.upper[3].on differs");
        require(a.parameters.windows.upper[3].band==b.parameters.windows.upper[3].band,"windows.upper[3].band differs");
        require(a.parameters.windows.right[0].x==b.parameters.windows.right[0].x,"windows.right[0].x differs");
        require(a.parameters.windows.right[0].y==b.parameters.windows.right[0].y,"windows.right[0].y differs");
        require(a.parameters.windows.right[0].w==b.parameters.windows.right[0].w,"windows.right[0].w differs");
        require(a.parameters.windows.right[0].h==b.parameters.windows.right[0].h,"windows.right[0].h differs");
        require(a.parameters.windows.right[0].on==b.parameters.windows.right[0].on,"windows.right[0].on differs");
        require(a.parameters.windows.right[1].x==b.parameters.windows.right[1].x,"windows.right[1].x differs");
        require(a.parameters.windows.right[1].y==b.parameters.windows.right[1].y,"windows.right[1].y differs");
        require(a.parameters.windows.right[1].w==b.parameters.windows.right[1].w,"windows.right[1].w differs");
        require(a.parameters.windows.right[1].h==b.parameters.windows.right[1].h,"windows.right[1].h differs");
        require(a.parameters.windows.right[1].on==b.parameters.windows.right[1].on,"windows.right[1].on differs");
        require(a.parameters.windows.right[2].x==b.parameters.windows.right[2].x,"windows.right[2].x differs");
        require(a.parameters.windows.right[2].y==b.parameters.windows.right[2].y,"windows.right[2].y differs");
        require(a.parameters.windows.right[2].w==b.parameters.windows.right[2].w,"windows.right[2].w differs");
        require(a.parameters.windows.right[2].h==b.parameters.windows.right[2].h,"windows.right[2].h differs");
        require(a.parameters.windows.right[2].on==b.parameters.windows.right[2].on,"windows.right[2].on differs");
        QFile source(QDir(folder).filePath("scene.json")); require(source.open(QIODevice::ReadOnly),"fixture missing");
        const auto original=QJsonDocument::fromJson(source.readAll()).object();
        QTemporaryDir tmp; require(tmp.isValid(),"temporary folder failed");
        require(QFile::copy(folder+"/art.svg",tmp.path()+"/art.svg"),"art fixture copy failed");
        const QString file=QDir(tmp.path()).filePath("scene.json");
        auto invalid=[&](QJsonObject root, const QString& expected) {
            QFile output(file); require(output.open(QIODevice::WriteOnly),"write fixture failed");
            output.write(QJsonDocument(root).toJson()); output.close();
            try { loadOsakaWorld(tmp.path()); throw std::runtime_error("invalid fixture accepted"); }
            catch (const std::runtime_error& e) { require(QString::fromUtf8(e.what())==file+": "+expected,e.what()); }
        };
        auto badArt=original;auto art=badArt["art"].toObject();auto elements=art["elements"].toObject();elements["near-house-shell"]="absent";art["elements"]=elements;badArt["art"]=art;
        invalid(badArt,"$.art.elements['near-house-shell']: expected SVG element ID compatible with Canvas replay");
        badArt=original;art=badArt["art"].toObject();elements=art["elements"].toObject();elements.remove("near-house-shell");art["elements"]=elements;badArt["art"]=art;
        invalid(badArt,"$.art.elements['near-house-shell']: expected required field");
        auto root=original; root["surprise"]=1;
        invalid(root,"$.surprise: expected known field (unknown field)");
        root=original; root["schema"]=2; invalid(root,"$.schema: expected schema version 1");
        root=original; root.remove("disc"); invalid(root,"$.disc: expected required field");
        root=original; auto disc=root["disc"].toObject();disc["x"]="nan";root["disc"]=disc;
        invalid(root,"$.disc.x: expected finite number");
        root=original; disc=root["disc"].toObject();disc["radius"]=0;root["disc"]=disc;
        invalid(root,"$.disc.radius: expected positive radius");
        auto changeSlot=[&](const QString& key,const QJsonValue& value) {
            auto r=original;auto stages=r["stages"].toArray();auto stage=stages[0].toObject();auto entries=stage["slots"].toArray();auto slot=entries[0].toObject();slot[key]=value;entries[0]=slot;stage["slots"]=entries;stages[0]=stage;r["stages"]=stages;return r;
        };
        invalid(changeSlot("profile","osaka-unknown-v1"),"$.stages[0].slots[0].profile: expected 'osaka-sky-v1'");
        invalid(changeSlot("id","backdrop"),"$.stages[0].slots[0].id: expected unique ID (duplicate 'backdrop')");
        invalid(changeSlot("piece","Alien"),"$.stages[0].slots[0].piece: expected known v1 piece");
        invalid(changeSlot("gate","Maybe"),"$.stages[0].slots[0].gate: expected known Osaka gate");
        invalid(changeSlot("extra",0),"$.stages[0].slots[0].extra: expected known field (unknown field)");
        root=original; auto stages=root["stages"].toArray();auto stage=stages[3].toObject();stage["events"]="Life";stages[3]=stage;root["stages"]=stages;
        invalid(root,"$.stages[3].events: expected LifeAndFlock for network or birds");
        root=original; auto profiles=root["profiles"].toObject(); profiles["unknown-v1"]=QJsonObject{}; root["profiles"]=profiles;
        invalid(root,"$.profiles['unknown-v1']: expected known field (unknown field)");
        auto changeParameter=[&](const QString& profile, const QString& key, const QJsonValue& value) {
            auto r=original;auto profiles=r["profiles"].toObject();auto settings=profiles[profile].toObject();settings[key]=value;profiles[profile]=settings;r["profiles"]=profiles;return r;
        };
        invalid(changeParameter("osaka-mountain-v1","samples",160.5),"$.profiles['osaka-mountain-v1'].samples: expected integer in 1..4096");
        invalid(changeParameter("osaka-mountain-v1","samples",4097),"$.profiles['osaka-mountain-v1'].samples: expected integer in 1..4096");
        invalid(changeParameter("osaka-finish-v1","bloom",1e100),"$.profiles['osaka-finish-v1'].bloom: expected finite float");
        invalid(changeParameter("osaka-disc-v1","hitSeconds",0),"$.profiles['osaka-disc-v1'].hitSeconds: expected positive number");
        invalid(changeParameter("osaka-neon-v1","glyphCount",4),"$.profiles['osaka-neon-v1'].glyphCount: expected integer in 1..3");
        invalid(changeParameter("osaka-haze-v1","noiseX",QJsonValue()),"$.profiles['osaka-haze-v1'].noiseX: expected finite number");
        root=original;profiles=root["profiles"].toObject();auto sky=profiles["osaka-sky-v1"].toObject();sky.remove("timeOffset");profiles["osaka-sky-v1"]=sky;root["profiles"]=profiles;
        invalid(root,"$.profiles['osaka-sky-v1'].timeOffset: expected required field");
        root=original;profiles=root["profiles"].toObject();auto windows=profiles["osaka-pane-v1"].toObject();auto near=windows["near"].toArray();auto pane=near[0].toObject();pane["band"]=6;near[0]=pane;windows["near"]=near;profiles["osaka-pane-v1"]=windows;root["profiles"]=profiles;
        invalid(root,"$.profiles['osaka-pane-v1'].near[0].band: expected integer in 0..5");
        // A finite, non-default double survives parsing without rounding to float.
        root=original; disc=root["disc"].toObject();disc["x"]=1.0000000000000002;root["disc"]=disc;
        { QFile output(file);require(output.open(QIODevice::WriteOnly),"write round-trip fixture failed");output.write(QJsonDocument(root).toJson()); }
        const auto roundTrip=loadOsakaWorld(tmp.path());
        require(roundTrip->description().disc.x==1.0000000000000002,"double failed to round-trip exactly");
        require(a.disc.x==b.disc.x,"loading a second world changed the first immutable description");
        // Generic standing-wave parameters round-trip, while invalid bounds
        // and every attempt to configure a crash fail closed.
        auto withWave=[&](const QJsonObject& settings){
            auto r=original;auto stages=r["stages"].toArray();auto stage=stages[0].toObject();
            auto entries=stage["slots"].toArray();entries.append(QJsonObject{{"id","test-wave"},{"piece","GreatWave"},
                {"profile","great-wave-v1"},{"gate","Always"},{"params",settings}});
            stage["slots"]=entries;stages[0]=stage;r["stages"]=stages;return r;
        };
        root=withWave(QJsonObject{{"anchorSide","right"},{"x",1890},{"maxRise",123},{"seed",9}});
        {QFile out(file);require(out.open(QIODevice::WriteOnly),"write wave fixture failed");out.write(QJsonDocument(root).toJson());}
        const auto waveWorld=loadOsakaWorld(tmp.path());const auto& stageWave=waveWorld->description().backdrop;
        const auto& parsedWave=stageWave.entries[stageWave.count-1].params->greatWave;
        require(parsedWave.anchorRight && parsedWave.x==1890 && parsedWave.maxRise==123 && parsedWave.seed==9,"wave settings lost");
        const QString wavePath="$.stages[0].slots["+QString::number(a.backdrop.count)+"].params";
        invalid(withWave(QJsonObject{{"anchorSide","up"}}),wavePath+".anchorSide: expected left or right");
        invalid(withWave(QJsonObject{{"maxRise",401}}),wavePath+".maxRise: expected number in 0..400");
        invalid(withWave(QJsonObject{{"baseHeight",900},{"maxRise",200}}),wavePath+": expected baseHeight + maxRise at most 1050");
        invalid(withWave(QJsonObject{{"clawCount",18.5}}),wavePath+".clawCount: expected integer in 6..30");
        invalid(withWave(QJsonObject{{"breakEnabled",true}}),wavePath+".breakEnabled: expected known field (unknown field)");
        invalid(withWave(QJsonObject{{"crashAt",60}}),wavePath+".crashAt: expected known field (unknown field)");
        auto withBoat=[&](const QJsonObject& settings){
            auto r=original;auto stages=r["stages"].toArray();auto stage=stages[0].toObject();
            auto entries=stage["slots"].toArray();
            entries.append(QJsonObject{{"id","test-boat"},{"piece","BoatOnWater"},{"profile","boat-on-water-v1"},{"gate","Always"},{"params",settings}});
            // Forward reference also proves resolution is independent of order.
            entries.append(QJsonObject{{"id","test-swell"},{"piece","SwellLines"},{"profile","swell-lines-v1"},{"gate","Always"},
                {"params",QJsonObject{{"seed",123},{"rows",12},{"amplitude",.63},{"driftSpeed",.37}}}});
            stage["slots"]=entries;stages[0]=stage;r["stages"]=stages;return r;
        };
        const QJsonObject boatSettings{{"waterInstance","test-swell"},{"row",7.3},{"length",290},{"scale",.5},{"crewCount",6},{"oarCount",4},{"seed",82}};
        root=withBoat(boatSettings);
        {QFile out(file);require(out.open(QIODevice::WriteOnly),"write boat fixture failed");out.write(QJsonDocument(root).toJson());}
        const auto boatWorld=loadOsakaWorld(tmp.path());const auto& parsedBoat=boatWorld->description().backdrop.entries[a.backdrop.count].params->boat;
        require(parsedBoat.row==7.3 && parsedBoat.length==290 && parsedBoat.scale==.5 && parsedBoat.crewCount==6 && parsedBoat.oarCount==4 && parsedBoat.seed==82,"boat parameters lost");
        require(parsedBoat.swell.seed==123 && parsedBoat.swell.rows==12 && parsedBoat.swell.amplitude==.63 && parsedBoat.swell.driftSpeed==.37,"boat does not share the named swell field");
        auto missing=boatSettings;missing["waterInstance"]="absent";
        invalid(withBoat(missing),"$.boat[test-boat].params.waterInstance: expected id of a SwellLines slot");
        missing["waterInstance"]="test-boat";
        invalid(withBoat(missing),"$.boat[test-boat].params.waterInstance: expected id of a SwellLines slot");
        missing=boatSettings;missing["row"]=11;
        invalid(withBoat(missing),"$.boat[test-boat].params.row: expected row and driftRows within the named surface");
        missing=boatSettings;missing["oarCount"]=7;
        invalid(withBoat(missing),wavePath+".oarCount: expected at most crewCount");
        missing=boatSettings;missing["row"]=7.2;missing["boardingAt"]=35;
        invalid(withBoat(missing),wavePath+".boardingAt: expected known field (unknown field)");
        std::cout<<"PASS: loaded Osaka equals compiled oracle; exact invalid diagnostics\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
