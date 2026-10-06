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
        QFile source(QDir(folder).filePath("scene.json")); require(source.open(QIODevice::ReadOnly),"fixture missing");
        const auto original=QJsonDocument::fromJson(source.readAll()).object();
        QTemporaryDir tmp; require(tmp.isValid(),"temporary folder failed");
        const QString file=QDir(tmp.path()).filePath("scene.json");
        auto invalid=[&](QJsonObject root, const QString& expected) {
            QFile output(file); require(output.open(QIODevice::WriteOnly),"write fixture failed");
            output.write(QJsonDocument(root).toJson()); output.close();
            try { loadOsakaWorld(tmp.path()); throw std::runtime_error("invalid fixture accepted"); }
            catch (const std::runtime_error& e) { require(QString::fromUtf8(e.what())==file+": "+expected,e.what()); }
        };
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
        std::cout<<"PASS: loaded Osaka equals compiled oracle; exact invalid diagnostics\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
