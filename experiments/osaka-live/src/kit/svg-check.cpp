#include "svg-art.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    app.setApplicationName("omadrop-svg-import");
    app.setApplicationVersion(QString::number(Journey::Kit::SvgSubsetVersion));
    QCommandLineParser parser;
    parser.setApplicationDescription("Validate supported SVG world art and list its stable IDs and artist labels.");
    parser.addHelpOption();parser.addVersionOption();
    parser.addOption({"import-svg","Validate and compile an SVG file (no window or audio).","file"});
    parser.process(app);
    if(!parser.isSet("import-svg") || !parser.positionalArguments().isEmpty()) {
        QTextStream(stderr)<<"Usage: omadrop-svg-import --import-svg FILE\n";return 2;
    }
    const auto result=Journey::Kit::importSvg(parser.value("import-svg"));
    if(!result) {QTextStream(stderr)<<result.diagnostic<<'\n';return 1;}
    QJsonArray elements;
    for(const auto& e:result.art->elements())elements.append(QJsonObject{{"id",e.id},{"label",e.label},{"tag",e.tag},{"line",e.line}});
    const QJsonDocument output(QJsonObject{{"subset",Journey::Kit::SvgSubsetVersion},{"elements",elements}});
    QTextStream(stdout)<<output.toJson(QJsonDocument::Indented);return 0;
}
