#include "kit/svg-art.h"
#include "gpu.h"
#include "headless.h"
#include <QCoreApplication>
#include <QImage>
#include <QDir>
#include <QFile>
#include <iomanip>
#include <QProcess>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstring>
using namespace Journey;
using namespace Journey::Kit;
static int checks=0;
static void check(bool ok,const std::string& message) {++checks;if(!ok){std::cerr<<"FAIL "<<message<<'\n';std::exit(1);}}
static QByteArray wrap(const QByteArray& body) {return "<svg xmlns='http://www.w3.org/2000/svg' xmlns:inkscape='http://www.inkscape.org/namespaces/inkscape' viewBox='0 0 1920 1080'>\n"+body+"\n</svg>";}
static std::shared_ptr<const SvgArt> good(const QByteArray& body) {
    const auto r=compileSvg(wrap(body),"fixture.svg");check(bool(r),r.diagnostic.toStdString());return r.art;
}
static const SvgElement& element(const SvgArt& art,const QString& id) {for(const auto& e:art.elements())if(e.id==id)return e;std::cerr<<"missing id\n";std::exit(1);}
static const Canvas& painted(const Canvas& c) {if(!c.gradients().empty() || !c.vertices().empty())return c;for(const auto& cmd:c.commands())if(cmd.kind==Canvas::CmdKind::Cached)return painted(c.retained(cmd.first));std::cerr<<"empty paint\n";std::exit(1);}
static void rejected(const QByteArray& body,const QString& tag,const QString& reason,bool unsupported=true) {
    const auto r=compileSvg(wrap(body),"bad.svg");
    const auto expected=QString("bad.svg:2: element <%1> id='bad': %2%3%4").arg(tag,unsupported?"unsupported feature '":"",reason,unsupported?"'":"");
    check(!r && r.diagnostic==expected,"diagnostic: expected "+expected.toStdString()+", got "+r.diagnostic.toStdString());
}
static void parsing() {
    QString error;QPainterPath p;
    check(svgPath("M10 20 l5 -5 h10 v20 q5 5 10 0 t10 0 c5 -5 5 5 10 0 s5 5 10 0 a10 20 30 0 1 10 10z M0,0 H10 V10 Q15 15 20 10 T30 10 C35 0 35 20 40 10 S45 20 50 10 A10 10 0 1 0 60 10 Z",p,error),"all absolute/relative commands");
    check(svgPath("M1e1-.5 20+2",p,error)&&p.currentPosition()==QPointF(20,2),"number grammar and implicit lineto");
    check(svgPath("M0 0 Q3 6 6 0 T12 0",p,error)&&p.elementAt(4).y==-4,"quadratic reflection");
    check(svgPath("M0 0 C1 2 3 4 5 6 S7 8 9 10",p,error)&&p.elementAt(4).x==7&&p.elementAt(4).y==8,"cubic reflection");
    check(svgPath("M1 0 A1 1 0 0 1 0 1",p,error)&&p.elementCount()==4,"quarter-circle cubic count");
    const double k=4./3.*std::tan(Pi/8);
    check(std::abs(p.elementAt(1).x-1)<1e-12&&std::abs(p.elementAt(1).y-k)<1e-12&&std::abs(p.elementAt(2).x-k)<1e-12&&p.elementAt(3).x==0&&p.elementAt(3).y==1,"known arc-to-cubic control points");
    check(svgPath("M0 0 A1 1 0 0 1 4 0",p,error)&&p.elementCount()==7,"undersized radii corrected to semicircle");
    check(svgPath("M0 0 A0 1 0 0 1 4 0 A1 1 0 1 1 4 0",p,error)&&p.elementCount()==2,"zero radius and coincident endpoints");
    check(!svgPath("L1 2",p,error)&&!svgPath("M0 0 A1 1 0 2 1 3 4",p,error)&&!svgPath("Mnan 0",p,error),"malformed path rejection");
    check(svgPath("M1 0 A1 1 0 0 0 0 -1",p,error)&&std::abs(p.elementAt(1).y+k)<1e-12,"negative sweep arc control");
    check(svgPath("M2 1 A2 1 90 1 1 3 2",p,error)&&p.currentPosition()==QPointF(3,2),"rotated elliptical large arc endpoint");
    check(svgPath("M10 10 l5 0z m5 5",p,error)&&p.currentPosition()==QPointF(15,15),"relative move after close");
    QTransform t;check(svgTransform("translate(10 20) scale(2 3)",t,error)&&t.map(QPointF(1,1))==QPointF(12,23),"SVG transform list order");
    check(svgTransform("matrix(1 0 0 1 10 0) rotate(90 1 1) skewX(45) skewY(0)",t,error)&&std::abs(t.map(QPointF(0,0)).x()-12)<1e-10,"matrix rotate-pivot skew composition");
    auto art=good("<g id='group' transform='translate(10 20)' fill='#123456'><rect id='r' inkscape:label='artist' data-name='fallback' x='1' y='2' width='3' height='4' transform='scale(2)'/><circle id='c' cx='40' cy='40' r='8'/><ellipse id='e' rx='4' ry='6'/><polygon id='p' points='0,0 10,0 10,10'/><polyline id='pl' points='0,0 10,0 10,10'/><line id='l' x2='10'/></g>");
    check(art->elements().size()==8 && element(*art,"r").label=="artist","all shapes, nested group, Inkscape label priority");
    check(element(*art,"r").transform.map(QPointF(1,2))==QPointF(12,24),"nested parent/local transforms");
    check(element(*art,"r").geometry.boundingRect()==QRectF(1,2,3,4),"rectangle conversion");
    Canvas selected;check(art->draw(selected,"group")&&!art->draw(selected,"absent")&&!selected.empty(),"group by id drawing");
    art.reset();check(!selected.retained(0).empty(),"compiled ownership survives art handle release");
    good("<rect id='round' rx='3' width='10' height='10'/><path d='M0 0 L10 0 L10 10Z' style='fill:rgb(10%,20%,30%);fill-rule:evenodd;fill-opacity:.5;stroke:blue;stroke-opacity:50%;stroke-width:2' opacity='.8'/>");
    for(const char* cap:{"round","butt","square"})for(const char* join:{"round","miter","bevel"})good(QByteArray("<polyline id='stroke' points='0 0 10 0 10 10' fill='none' stroke='red' stroke-width='4' stroke-linecap='")+cap+"' stroke-linejoin='"+join+"'/>");
    auto g=good("<defs><linearGradient id='base'><stop offset='0' stop-color='red'/><stop offset='100%' style='stop-color:blue;stop-opacity:.5'/></linearGradient><linearGradient id='user' href='#base' gradientUnits='userSpaceOnUse' x1='0' x2='100' gradientTransform='translate(10)'/><radialGradient id='rad' cx='.4' cy='.6' r='.8' fx='.3' fy='.5' fr='.1'/></defs><rect id='object' x='20' y='30' width='100' height='50' fill='url(#base)'/><rect id='u' x='20' y='30' width='100' height='50' fill='url(#user)'/>");
    const auto& object=painted(*element(*g,"object").canvas).gradients()[0].data;
    const auto& user=painted(*element(*g,"u").canvas).gradients()[0].data;
    check(std::abs(object[4]-.01)<1e-8&&std::abs(object[8]+.2)<1e-7&&object[12]==1,"objectBoundingBox gradient coordinates");
    check(user[4]==1&&user[8]==-10&&user[12]==100,"userSpaceOnUse gradient and gradientTransform");
    QByteArray stops="<defs><linearGradient id='many'>";for(int i=0;i<12;++i)stops+="<stop offset='"+QByteArray::number(i/11.)+"' stop-color='red'/>";
    good(stops+"</linearGradient></defs><rect width='10' height='10' fill='url(#many)'/>");
    auto uses=good("<defs><symbol id='s' viewBox='0 0 10 10'><rect id='part' data-name='named' width='10' height='10'/></symbol></defs><use id='copy' href='#s' x='20' y='30' width='100' height='50' fill='red'/>");
    check(element(*uses,"part").label=="named"&&!element(*uses,"copy").canvas->empty(),"defs/symbol/use and data-name");
    good("<defs><path id='shape' d='M0 0h10v10z'/></defs><use xmlns:xlink='http://www.w3.org/1999/xlink' xlink:href='#shape'/>");
    good("<defs><radialGradient id='rad' gradientUnits='userSpaceOnUse' cx='960' cy='540' r='120' fx='940' fy='540' fr='10' gradientTransform='matrix(1 0 .2 1 0 0)'><stop offset='0' stop-color='white'/><stop offset='1' stop-color='black'/></radialGradient></defs><circle cx='960' cy='540' r='100' fill='url(#rad)'/>");
    good("<defs><linearGradient id='zero' x1='0' x2='0'><stop offset='1' stop-color='red'/></linearGradient><radialGradient id='point' r='0'><stop offset='1' stop-color='blue'/></radialGradient></defs><rect width='10' height='10' fill='url(#zero)'/><rect width='10' height='10' fill='url(#point)'/>");
    auto clipped=good("<defs><symbol id='clip' viewBox='0 0 10 10'><rect x='-10' y='-10' width='30' height='30'/></symbol></defs><use id='viewport' href='#clip' x='20' y='30' width='100' height='100'/>");
    const auto bounds=element(*clipped,"viewport").canvas->bounds();
    check(bounds[0]==20 && bounds[1]==30 && bounds[2]==120 && bounds[3]==130,"symbol viewport clips overflowing geometry");
    auto solid=good("<g fill='red' fill-opacity='.5'><rect id='alpha' width='10' height='10'/></g>");
    check(painted(*element(*solid,"alpha").canvas).vertices().back().a==.5f,"inherited fill opacity");
    auto hole=good("<path id='hole' d='M0 0h10v10h-10z M2 2h6v6h-6z' fill-rule='evenodd'/>");
    check(painted(*element(*hole,"hole").canvas).commands()[0].kind==Canvas::CmdKind::StencilEvenOdd,"explicit evenodd stencil command");
    auto winding=good("<path id='winding' d='M0 0h10v10h-10z M2 2h6v6h-6z' fill-rule='nonzero'/>");
    check(painted(*element(*winding,"winding").canvas).commands()[0].kind==Canvas::CmdKind::StencilFill,"explicit nonzero stencil command");
    std::cout<<"PASS parsing and numerical arc/transform/gradient assertions\n";
}
static void rejections() {
    rejected("<text id='bad'/>","text","text (outline text before export)");
    for(const char* tag:{"filter","mask","clipPath","image","style","pattern","foreignObject","animate"})rejected(QByteArray("<")+tag+" id='bad'/>",tag,QString("element '")+tag+"'");
    for(const char* key:{"filter","mask","clip-path","class","stroke-dasharray"})rejected(QByteArray("<path id='bad' ")+key+"='x'/>","path",QString("attribute '")+key+"'");
    rejected("<use id='bad' href='other.svg#x'/>","use","external reference 'other.svg#x'");
    rejected("<path id='bad' d='M0 0h10v10z' fill='url(https://example.test/a)'/>","path","external or invalid paint reference 'url(https://example.test/a)'");
    rejected("<g id='bad' style='filter:none'/>","g","style 'filter'");
    rejected("<g id='bad' transform='perspective(1)'/>","g","unsupported transform 'perspective'",false);
    rejected("<path id='bad' d='M0 0L'/>","path","invalid path at offset 5",false);
    rejected("<use id='bad' href='#missing'/>","use","missing reference '#missing'",false);
    rejected("<g id='bad'><use href='#bad'/></g>","g","cyclic use reference",false);
    rejected("<path id='bad'/><path id='bad'/>","path","label='': duplicate id 'bad'",false);
    rejected("<rect id='bad' width='-1'/>","rect","invalid rectangle dimensions",false);
    rejected("<rect id='bad' width='nan'/>","rect","invalid width",false);
    rejected("<linearGradient id='bad' spreadMethod='repeat'/>","linearGradient","spreadMethod 'repeat'");
    rejected("<linearGradient id='bad' href='#bad'/>","linearGradient","cyclic gradient reference",false);
    rejected("<linearGradient id='bad' gradientUnits='bogus'/>","linearGradient","gradientUnits 'bogus'");
    rejected("<radialGradient id='bad' r='-1'/>","radialGradient","negative radial radius",false);
    rejected("<path id='bad' fill='url(other.svg#x)'/>","path","external or invalid paint reference 'url(other.svg#x)'");
    rejected("<svg id='bad'/>","svg","nested svg");
    rejected("<radialGradient id='bad' fr='-1'/>","radialGradient","negative radial focal radius",false);
    rejected("<path id='bad' fill-rule='bogus'/>","path","fill-rule 'bogus'");
    rejected("<path id='bad' stroke-linecap='bogus'/>","path","stroke-linecap 'bogus'");
    rejected("<path id='bad' stroke-linejoin='bogus'/>","path","stroke-linejoin 'bogus'");
    rejected("<g id='bad' transform='scale(100000000)'/>","g","transform exceeds coordinate limit",false);
    const auto pi=compileSvg("<?xml-stylesheet href='style.css'?><svg viewBox='0 0 1920 1080'/>","pi.svg");
    check(!pi&&pi.diagnostic=="pi.svg:1: unsupported processing instruction 'xml-stylesheet'","CSS processing instruction diagnostic");
    const auto dtd=compileSvg("<!DOCTYPE svg [<!ENTITY x 'hi'>]><svg viewBox='0 0 1920 1080'/>","dtd.svg");check(!dtd&&dtd.diagnostic=="dtd.svg: DTD/entities are unsupported","DTD diagnostic");
    const auto root=compileSvg("<svg viewBox='0 0 100 100'/>","root.svg");check(!root&&root.diagnostic=="root.svg:1: element <svg> id='': viewBox must be '0 0 1920 1080'","design viewBox rejection");
    check(!compileSvg("<svg>","broken.svg"),"malformed XML");
    std::cout<<"PASS exact source-node rejection diagnostics\n";
}
static QImage render(Gpu& gpu,const SvgArt& art) {
    gpu.begin(320,180);Canvas c(1./6);art.draw(c);gpu.draw(c);
    FinishParams f;f.bloom=f.vignette=f.grain=0;f.knee=.9999;gpu.finish(f,nullptr);
    std::vector<unsigned char> rgb;gpu.readRgb(rgb);
    return QImage(rgb.data(),320,180,320*3,QImage::Format_RGB888).copy();
}
static void images() {
    HeadlessContext context;QString error;check(context.create(error),error.toStdString());
    std::cout<<"GPU: "<<context.renderer().toStdString()<<'\n';
    Gpu gpu;check(gpu.init(error),error.toStdString());
    const QString directory=SVG_FIXTURES;const bool update=qEnvironmentVariableIsSet("SVG_UPDATE_GOLDENS");
    QFile rendererFile(directory+"/renderer.txt");
    check(rendererFile.open(QIODevice::ReadOnly),"read golden renderer");
    const QString goldenRenderer=QString::fromUtf8(rendererFile.readAll()).trimmed();
    check(!goldenRenderer.isEmpty(),"nonempty golden renderer");
    const bool exact=context.renderer()==goldenRenderer;
    check(!update||exact,"golden updates require recorded renderer "+goldenRenderer.toStdString());
    // Measured on devbox llvmpipe (LLVM 20.1.2, 256 bits), against Intel PNGs:
    // shapes: 78, 563/57600; paints: 52, 507/57600; instances: 48, 162/57600.
    // Bound both MSAA edge rounding magnitude and the fraction of affected pixels.
    struct GoldenLimit {const char* name;int maximum;double fraction;};
    for(const auto& limit:{GoldenLimit{"shapes",79,.0098},GoldenLimit{"paints",53,.0089},GoldenLimit{"instances",49,.0029}}) {
        const QString name=QString::fromLatin1(limit.name);
        const auto imported=importSvg(directory+"/"+name+".svg",1./6);check(bool(imported),imported.diagnostic.toStdString());
        const auto image=render(gpu,*imported.art),repeat=render(gpu,*imported.art);
        check(image==repeat,"repeat render "+name.toStdString());
        const QString golden=directory+"/"+name+".png";
        if(update)check(image.save(golden),"write golden");else {
            const auto reference=QImage(golden).convertToFormat(QImage::Format_RGB888);
            check(!reference.isNull()&&reference.size()==image.size(),"golden dimensions "+name.toStdString());
            int maximum=0,changed=0;
            for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
                const auto a=image.pixelColor(x,y),b=reference.pixelColor(x,y);
                const int difference=std::max({std::abs(a.red()-b.red()),std::abs(a.green()-b.green()),std::abs(a.blue()-b.blue())});
                maximum=std::max(maximum,difference);changed+=difference!=0;
            }
            const double fraction=double(changed)/(image.width()*image.height());
            std::cout<<"golden "<<name.toStdString()<<": max_channel_difference="<<maximum
                <<" differing_pixels="<<changed<<"/"<<image.width()*image.height()
                <<" fraction="<<std::setprecision(10)<<fraction
                <<" mode="<<(exact?"exact":"tolerant")
                <<" limits="<<(exact?0:limit.maximum)<<","<<(exact?0:limit.fraction)<<'\n';
            if(exact)check(image==reference,"exact golden "+name.toStdString());
            else check(maximum<=limit.maximum&&fraction<=limit.fraction,"tolerant golden "+name.toStdString());
        }
        if(name=="shapes") {check(image.pixelColor(140,120)==QColor(Qt::black),"evenodd hole pixel");}
        if(name=="instances") {const auto c=image.pixelColor(120,60);check(c.red()==0&&std::abs(c.green()-64)<=1&&std::abs(c.blue()-64)<=1,"nested group opacity overlap composited once");}
        const QString raster=QStandardPaths::findExecutable("rsvg-convert");
        if(!raster.isEmpty()) {
            QProcess p;p.start(raster,{"--width","320","--height","180","--background-color","black",directory+"/"+name+".svg"});
            check(p.waitForFinished(15000)&&p.exitCode()==0,"reference rasterizer process");const auto reference=QImage::fromData(p.readAllStandardOutput()).convertToFormat(QImage::Format_RGB888);
            check(reference.size()==image.size(),"reference dimensions");
            double total=0,edgeTotal=0,interiorTotal=0;int maximum=0,edgeMax=0,interiorMax=0,edges=0,interiors=0,above2=0;
            for(int y=0;y<180;++y)for(int x=0;x<320;++x) {
                const auto a=image.pixelColor(x,y),b=reference.pixelColor(x,y);bool edge=false;
                for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx) {
                    const auto n=reference.pixelColor(std::clamp(x+dx,0,319),std::clamp(y+dy,0,179));
                    if(std::max({std::abs(n.red()-b.red()),std::abs(n.green()-b.green()),std::abs(n.blue()-b.blue())})>8)edge=true;
                }
                const int sum=std::abs(a.red()-b.red())+std::abs(a.green()-b.green())+std::abs(a.blue()-b.blue());
                const int mx=std::max({std::abs(a.red()-b.red()),std::abs(a.green()-b.green()),std::abs(a.blue()-b.blue())});
                total+=sum;maximum=std::max(maximum,mx);above2+=mx>2;
                if(edge){++edges;edgeTotal+=sum;edgeMax=std::max(edgeMax,mx);}else{++interiors;interiorTotal+=sum;interiorMax=std::max(interiorMax,mx);}
            }
            std::cout<<"rsvg sanity "<<name.toStdString()<<": MAE="<<total/(320*180*3)<<" max="<<maximum<<" pixels_gt2="<<100.*above2/(320*180)<<"% edge_MAE="<<edgeTotal/std::max(1,edges*3)<<" edge_max="<<edgeMax<<" interior_MAE="<<interiorTotal/std::max(1,interiors*3)<<" interior_max="<<interiorMax<<" (informational)\n";
        } else std::cout<<"reference rasterizer unavailable (informational)\n";
    }
    std::cout<<"PASS committed SVG goldens ("<<(update?"updated":exact?"exact":"tolerant")<<")\n";
}

static void nativeReplay() {
    const auto art=good("<g id='native' transform='translate(10 20)' fill='rgb(1.2%,3%,2.4%)'><path d='M0 0 Q15 30 40 5 C60 0 80 50 90 10 L0 0 Z M20 10 L25 10 L25 15 Z'/><path d='M0 40 L40 40 L50 60 Z' fill='none' stroke='rgb(36%,72%,56%)' stroke-width='1.6' stroke-opacity='.45' stroke-linecap='round' stroke-linejoin='round'/><rect x='2' y='3' width='4' height='5'/><circle cx='20' cy='30' r='19'/></g>");
    Canvas actual,expected;
    actual.translate(-4.25,0);expected.translate(-4.25,0);
    check(art->replay(actual,"native"),"native retained-span replay");
    expected.save();expected.translate(10,20);expected.color(Col(.012f,.030f,.024f));
    expected.moveTo(0,0);expected.quadTo(15,30,40,5);expected.curveTo(60,0,80,50,90,10);expected.lineTo(0,0);expected.closePath();
    expected.moveTo(20,10);expected.lineTo(25,10);expected.lineTo(25,15);expected.closePath();expected.fill();
    expected.color(Col(.36f,.72f,.56f),.45);expected.moveTo(0,40);expected.lineTo(40,40);expected.lineTo(50,60);expected.closePath();expected.stroke(1.6);
    expected.fillRect(2,3,4,5,Col(.012f,.030f,.024f));expected.disc(20,30,19,Col(.012f,.030f,.024f));expected.restore();
    check(actual.vertices().size()==expected.vertices().size()&&std::memcmp(actual.vertices().data(),expected.vertices().data(),actual.vertices().size()*sizeof(Vertex))==0,"raw Q/C/close, native round strokes, rect/circle and decimal float paint retain every vertex");
    check(actual.commands().size()==expected.commands().size(),"native command count");
    for(std::size_t i=0;i<actual.commands().size();++i) {const auto a=actual.commands()[i],b=expected.commands()[i];check(a.kind==b.kind&&a.first==b.first&&a.count==b.count&&a.coverFirst==b.coverFirst&&a.coverCount==b.coverCount,"native command order");}
    auto paint=good("<defs><linearGradient id='grad' gradientUnits='userSpaceOnUse' x1='0' y1='132' x2='0' y2='273'><stop offset='0' stop-color='rgb(4%,17%,12.7%)'/><stop offset='1' stop-color='rgb(1.3%,5%,3.9%)'/></linearGradient></defs><rect id='gradient' x='0' y='132' width='100' height='141' fill='url(#grad)'/>");
    Canvas a,b;paint->replay(a,"gradient");b.linear(0,132,0,273,{{0,Col(.04f,.17f,.127f),1},{1,Col(.013f,.05f,.039f),1}});b.rect(0,132,100,141);b.fill();
    check(a.gradients()[0].data==b.gradients()[0].data,"native gradient equation and exact float stops");
    auto use=good("<defs><rect id='r' width='10' height='10'/></defs><use id='u' href='#r'/><g id='opacity' opacity='.5'><rect width='10' height='10'/></g>");
    check(!use->replay(a,"u")&&!use->replay(a,"opacity"),"replay rejects instances and composited opacity instead of silently changing semantics");
    std::cout<<"PASS exact native SVG replay\n";
}
static void diagnosticTool() {
    QProcess process;process.start(SVG_CHECK_TOOL,{"--import-svg",QString(SVG_FIXTURES)+"/shapes.svg"});
    check(process.waitForFinished(15000)&&process.exitCode()==0,"diagnostic tool valid exit");
    const auto json=QJsonDocument::fromJson(process.readAllStandardOutput()).object();
    check(json["subset"].toInt()==1&&json["elements"].toArray().size()==9,"diagnostic tool JSON element list");
    const auto elements=json["elements"].toArray();check(elements[1].toObject()["id"].toString()=="box"&&elements[1].toObject().contains("label"),"diagnostic tool IDs/labels");
    process.start(SVG_CHECK_TOOL,{"--import-svg",QString(SVG_FIXTURES)+"/unsupported-text.svg"});
    check(process.waitForFinished(15000)&&process.exitCode()==1,"diagnostic tool rejection exit");
    const auto expected=QString(SVG_FIXTURES)+"/unsupported-text.svg:2: element <text> id='caption': unsupported feature 'text (outline text before export)'\n";
    check(QString::fromUtf8(process.readAllStandardError())==expected,"diagnostic tool exact file/id/line error");
    process.start(SVG_CHECK_TOOL,QStringList{});check(process.waitForFinished(15000)&&process.exitCode()==2,"diagnostic tool missing argument exit");
    std::cout<<"PASS headless SVG diagnostic CLI\n";
}
int main(int argc,char** argv) {QCoreApplication app(argc,argv);parsing();rejections();nativeReplay();diagnosticTool();images();std::cout<<"PASS "<<checks<<" SVG checks\n";}
