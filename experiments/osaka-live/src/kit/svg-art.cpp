#include "svg-art.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QPainterPathStroker>
#include <QColor>
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <stdexcept>
#include <cmath>
#include <optional>

namespace Journey::Kit {
namespace {
struct Node {
    QString tag,id,label; qint64 line=0; std::size_t index=0;
    QMap<QString,QString> a;
    std::vector<std::shared_ptr<Node>> children;
};
struct Failure {QString text;};
const QSet<QString> presentation={"fill","fill-rule","fill-opacity","opacity","stroke","stroke-width","stroke-opacity","stroke-linecap","stroke-linejoin","stroke-miterlimit","color"};
struct Style {
    QString fill="black",stroke="none",color="black";
    double fillAlpha=1,strokeAlpha=1,width=1,miter=4;
    Qt::FillRule rule=Qt::WindingFill;
    Qt::PenCapStyle cap=Qt::FlatCap;
    Qt::PenJoinStyle join=Qt::SvgMiterJoin;
};
void replay(Canvas& canvas,const QPainterPath& path) {
    // Sign-outline style replay, after SVG normalization. QPainterPath stores
    // quadratics as cubics. It preserves explicit close by returning to start.
    for(int i=0;i<path.elementCount();++i) {
        const auto e=path.elementAt(i);
        if(e.isMoveTo())canvas.moveTo(e.x,e.y);
        else if(e.isLineTo())canvas.lineTo(e.x,e.y);
        else if(e.type==QPainterPath::CurveToElement) {
            const auto b=path.elementAt(++i),c=path.elementAt(++i);
            canvas.curveTo(e.x,e.y,b.x,b.y,c.x,c.y);
        }
    }
}
}
class SvgCompiler {
public:
    QString filename;double scale;
    QMap<QString,std::shared_ptr<Node>> ids;
    std::vector<std::shared_ptr<Node>> order;
    QSet<const Node*> active;
    std::size_t compilations=0,vertices=0;
    std::shared_ptr<SvgArt> art=std::make_shared<SvgArt>();
    SvgCompiler(QString name,double pixelScale):filename(std::move(name)),scale(pixelScale) {}
    [[noreturn]] void fail(const Node& n,const QString& feature,bool unsupported=true) const {
        throw Failure{QString("%1:%2: element <%3> id='%4': %5%6%7").arg(filename).arg(n.line).arg(n.tag,n.id,unsupported?"unsupported feature '":"",feature,unsupported?"'":"")};
    }
    double number(const Node& n,const QString& text,const QString& feature,bool percent=false,double percentScale=1) const {
        auto s=text.trimmed();bool pct=s.endsWith('%');if(pct)s.chop(1);
        if(s.endsWith("px"))s.chop(2);
        bool ok=false;double v=s.toDouble(&ok);
        if(!ok||!std::isfinite(v)||std::abs(v)>1e9||(pct&&!percent))fail(n,"invalid "+feature,false);
        return pct?v*percentScale/100.:v;
    }
    double value(const Node& n,const QString& key,double fallback=0) const {return n.a.contains(key)?number(n,n.a[key],key):fallback;}
    double alpha(const Node& n,const QString& key,double fallback=1) const {return n.a.contains(key)?std::clamp(number(n,n.a[key],key,true),0.,1.):fallback;}
    QTransform transform(const Node& n,const QString& key="transform") const {
        QTransform t;QString e;if(!svgTransform(n.a.value(key),t,e))fail(n,e,false);return t;
    }
    QColor color(const Node& n,QString text,const QString& current) const {
        if(text=="currentColor")text=current;
        static const QRegularExpression rgb(R"(^rgb\(\s*([^,]+),\s*([^,]+),\s*([^\)]+)\)$)");
        auto m=rgb.match(text);
        if(m.hasMatch()) {
            QColor c;c.setRgbF(std::clamp(number(n,m.captured(1),"color",true,255)/255.,0.,1.),std::clamp(number(n,m.captured(2),"color",true,255)/255.,0.,1.),std::clamp(number(n,m.captured(3),"color",true,255)/255.,0.,1.));return c;
        }
        QColor c(text);if(!c.isValid())fail(n,"paint '"+text+"'");return c;
    }
    std::shared_ptr<Node> read(QXmlStreamReader& xml,int depth=0) {
        auto n=std::make_shared<Node>();n->tag=xml.name().toString();n->line=xml.lineNumber();n->id=xml.attributes().value("id").toString();
        if(depth>64||order.size()>=10000)fail(*n,"document complexity limit",false);
        const auto uri=xml.namespaceUri().toString();
        if(!uri.isEmpty()&&uri!="http://www.w3.org/2000/svg")fail(*n,"element namespace '"+uri+"'");
        for(const auto& a:xml.attributes()) {
            auto key=a.name().toString();
            if(key=="label" && a.namespaceUri().toString()=="http://www.inkscape.org/namespaces/inkscape")n->label=a.value().toString();
            else if(!a.namespaceUri().isEmpty() && !(key=="href"&&a.namespaceUri().toString()=="http://www.w3.org/1999/xlink"))fail(*n,"attribute '"+a.qualifiedName().toString()+"'");
            else n->a[key]=a.value().toString();
        }
        n->id=n->a.value("id");if(n->label.isEmpty())n->label=n->a.value("data-name");
        if(!n->id.isEmpty()) {
            if(ids.contains(n->id))throw Failure{QString("%1:%2: element <%3> id='%4' label='%5': duplicate id '%4'")
                .arg(filename).arg(n->line).arg(n->tag,n->id,n->label)};
            ids[n->id]=n;
        }
        n->index=order.size();order.push_back(n);
        if(n->a.contains("style")) {
            for(const auto& declaration:n->a["style"].split(';',Qt::SkipEmptyParts)) {
                const auto colon=declaration.indexOf(':');
                if(colon<1)fail(*n,"invalid inline style",false);
                const auto key=declaration.left(colon).trimmed(),v=declaration.mid(colon+1).trimmed();
                if(!presentation.contains(key) && key!="stop-color" && key!="stop-opacity")fail(*n,"style '"+key+"'");
                n->a[key]=v;
            }
        }
        QSet<QString> allowed=presentation;allowed.unite({"id","data-name","style","transform"});
        if(n->tag=="svg")allowed.unite({"viewBox","width","height","version"});
        else if(n->tag=="path")allowed.insert("d");
        else if(n->tag=="rect")allowed.unite({"x","y","width","height","rx","ry"});
        else if(n->tag=="circle")allowed.unite({"cx","cy","r"});
        else if(n->tag=="ellipse")allowed.unite({"cx","cy","rx","ry"});
        else if(n->tag=="line")allowed.unite({"x1","y1","x2","y2"});
        else if(n->tag=="polygon"||n->tag=="polyline")allowed.insert("points");
        else if(n->tag=="use")allowed.unite({"href","x","y","width","height"});
        else if(n->tag=="symbol")allowed.unite({"viewBox","preserveAspectRatio"});
        else if(n->tag=="linearGradient"||n->tag=="radialGradient") {
            allowed.unite({"href","gradientUnits","gradientTransform","spreadMethod"});
            if(n->tag=="linearGradient")allowed.unite({"x1","y1","x2","y2"});
            else allowed.unite({"cx","cy","r","fx","fy","fr"});
        }
        else if(n->tag=="stop")allowed.unite({"offset","stop-color","stop-opacity"});
        else if(n->tag!="g"&&n->tag!="defs")fail(*n,n->tag=="text"?"text (outline text before export)":"element '"+n->tag+"'");
        for(auto i=n->a.begin();i!=n->a.end();++i)if(!allowed.contains(i.key()))fail(*n,"attribute '"+i.key()+"'");
        if(n->a.contains("href")&&!n->a["href"].startsWith('#'))fail(*n,"external reference '"+n->a["href"]+"'");
        while(!xml.atEnd()) {
            xml.readNext();
            if(xml.isStartElement())n->children.push_back(read(xml,depth+1));
            else if(xml.isEndElement())break;
            else if(xml.isDTD()||xml.isEntityReference())fail(*n,"DTD/entities");
            else if(xml.isProcessingInstruction())fail(*n,"processing instruction '"+xml.processingInstructionTarget().toString()+"'");
            else if(xml.isCharacters()&&!xml.isWhitespace())fail(*n,"character data");
        }
        return n;
    }
    Style style(const Node& n,Style s) const {
        auto string=[&](const QString& key,QString& v){if(n.a.contains(key)&&n.a[key]!="inherit")v=n.a[key];};
        string("fill",s.fill);string("stroke",s.stroke);string("color",s.color);
        if(n.a.contains("fill-opacity")&&n.a["fill-opacity"]!="inherit")s.fillAlpha=alpha(n,"fill-opacity");
        if(n.a.contains("stroke-opacity")&&n.a["stroke-opacity"]!="inherit")s.strokeAlpha=alpha(n,"stroke-opacity");
        if(n.a.contains("stroke-width")&&n.a["stroke-width"]!="inherit")s.width=value(n,"stroke-width");
        if(n.a.contains("stroke-miterlimit")&&n.a["stroke-miterlimit"]!="inherit")s.miter=value(n,"stroke-miterlimit");
        if(s.width<0||s.miter<1)fail(n,"invalid stroke width/miter limit",false);
        const auto rule=n.a.value("fill-rule","inherit");
        if(rule=="evenodd")s.rule=Qt::OddEvenFill;else if(rule=="nonzero")s.rule=Qt::WindingFill;else if(rule!="inherit")fail(n,"fill-rule '"+rule+"'");
        const auto cap=n.a.value("stroke-linecap","inherit"),join=n.a.value("stroke-linejoin","inherit");
        if(cap=="round")s.cap=Qt::RoundCap;else if(cap=="butt")s.cap=Qt::FlatCap;else if(cap=="square")s.cap=Qt::SquareCap;else if(cap!="inherit")fail(n,"stroke-linecap '"+cap+"'");
        if(join=="round")s.join=Qt::RoundJoin;else if(join=="miter")s.join=Qt::SvgMiterJoin;else if(join=="bevel")s.join=Qt::BevelJoin;else if(join!="inherit")fail(n,"stroke-linejoin '"+join+"'");
        return s;
    }
    QPainterPath geometry(const Node& n) const {
        QPainterPath p;QString e;
        if(n.tag=="path") {if(!svgPath(n.a.value("d"),p,e))fail(n,e,false);}
        else if(n.tag=="rect") {
            const double x=value(n,"x"),y=value(n,"y"),w=value(n,"width"),h=value(n,"height");
            double rx=value(n,"rx",value(n,"ry")),ry=value(n,"ry",rx);
            if(w<0||h<0||rx<0||ry<0)fail(n,"invalid rectangle dimensions",false);
            if(w>0&&h>0)p.addRoundedRect(QRectF(x,y,w,h),std::min(rx,w/2),std::min(ry,h/2),Qt::AbsoluteSize);
        } else if(n.tag=="circle"||n.tag=="ellipse") {
            const double rx=value(n,n.tag=="circle"?"r":"rx"),ry=n.tag=="circle"?rx:value(n,"ry");
            if(rx<0||ry<0)fail(n,"invalid ellipse dimensions",false);
            if(rx>0&&ry>0)p.addEllipse(QPointF(value(n,"cx"),value(n,"cy")),rx,ry);
        } else if(n.tag=="line") {p.moveTo(value(n,"x1"),value(n,"y1"));p.lineTo(value(n,"x2"),value(n,"y2"));}
        else if(n.tag=="polygon"||n.tag=="polyline") {
            // Reuse the path scanner for the SVG comma/space/sign/exponent grammar.
            if(!svgPath("M"+n.a.value("points")+(n.tag=="polygon"?" Z":""),p,e))fail(n,e,false);
        }
        return p;
    }
    std::shared_ptr<Node> reference(const Node& n) const {
        const auto id=n.a.value("href").mid(1);if(!ids.contains(id))fail(n,"missing reference '#"+id+"'",false);return ids[id];
    }
    Node gradient(const Node& n,QSet<const Node*> chain={}) const {
        if(chain.contains(&n))fail(n,"cyclic gradient reference",false);chain.insert(&n);
        Node merged=n;
        if(n.a.contains("href")) {
            auto ref=reference(n);if(ref->tag!="linearGradient"&&ref->tag!="radialGradient")fail(n,"reference is not a gradient",false);
            Node base=gradient(*ref,chain);
            for(auto i=base.a.begin();i!=base.a.end();++i)if(!merged.a.contains(i.key()))merged.a[i.key()]=i.value();
            if(merged.children.empty())merged.children=base.children;
        }
        return merged;
    }
    void paint(Canvas& c,const Node& node,const QString& text,double opacity,const Style& s,const QTransform& world,const QRectF& box) const {
        if(!text.startsWith("url(")) {const auto col=color(node,text,s.color);c.color(Col(col.redF(),col.greenF(),col.blueF()),opacity*col.alphaF());return;}
        static const QRegularExpression re(R"(^url\(\s*#([^\s\)]+)\s*\)$)");
        const auto match=re.match(text);if(!match.hasMatch())fail(node,"external or invalid paint reference '"+text+"'");
        const auto key=match.captured(1);if(!ids.contains(key))fail(node,"missing gradient '#"+key+"'",false);
        const auto& raw=*ids[key];if(raw.tag!="linearGradient"&&raw.tag!="radialGradient")fail(node,"paint reference is not a gradient",false);
        const auto g=gradient(raw);
        const auto units=g.a.value("gradientUnits","objectBoundingBox");
        if(units!="objectBoundingBox"&&units!="userSpaceOnUse")fail(raw,"gradientUnits '"+units+"'");
        if(g.a.value("spreadMethod","pad")!="pad")fail(raw,"spreadMethod '"+g.a["spreadMethod"]+"'");
        QTransform coords;
        const bool object=units=="objectBoundingBox";
        if(object) {if(box.width()==0||box.height()==0) {c.color(Col(0,0,0),0);return;}coords.translate(box.x(),box.y());coords.scale(box.width(),box.height());}
        const auto total=transform(g,"gradientTransform")*coords*world;
        bool ok=false;const auto inv=total.inverted(&ok);if(!ok)fail(raw,"singular gradient transform",false);
        GradientRow row;row.data[4]=inv.m11();row.data[5]=inv.m12();row.data[6]=inv.m21();row.data[7]=inv.m22();row.data[8]=inv.dx();row.data[9]=inv.dy();
        auto coordinate=[&](QString key,double fallback,double percentScale) {return g.a.contains(key)?number(raw,g.a[key],key,true,object?1:percentScale):fallback;};
        if(g.tag=="linearGradient") {
            row.data[0]=4;row.data[10]=coordinate("x1",0,1920);row.data[11]=coordinate("y1",0,1080);
            row.data[12]=coordinate("x2",object?1:1920,1920);row.data[13]=coordinate("y2",0,1080);
        } else {
            row.data[0]=5;row.data[10]=coordinate("cx",object?.5:960,1920);row.data[11]=coordinate("cy",object?.5:540,1080);
            const double diagonal=std::hypot(1920.,1080.)/std::sqrt(2.);
            row.data[14]=coordinate("r",object?.5:diagonal*.5,diagonal);
            row.data[2]=coordinate("fx",row.data[10],1920);row.data[3]=coordinate("fy",row.data[11],1080);
            if(row.data[14]<0)fail(raw,"negative radial radius",false);
            row.data[15]=coordinate("fr",0,diagonal);
            if(row.data[15]<0)fail(raw,"negative radial focal radius",false);
            row.data[15]=std::min(row.data[15],row.data[14]);
            // SVG clamps a focal point outside the circle just inside its edge.
            const double dx=row.data[2]-row.data[10],dy=row.data[3]-row.data[11],d=std::hypot(dx,dy);
            if(d>row.data[14]-row.data[15]) {const double k=(row.data[14]-row.data[15])*(1-1e-6)/d;row.data[2]=row.data[10]+dx*k;row.data[3]=row.data[11]+dy*k;}
        }
        int i=0;double last=0;bool translucent=false;
        for(const auto& stop:g.children) {
            if(stop->tag!="stop")fail(*stop,"gradient child '"+stop->tag+"'");
            if(i==1024)fail(raw,"more than 1024 gradient stops");
            const auto col=color(*stop,stop->a.value("stop-color","black"),s.color);
            const float a=alpha(*stop,"stop-opacity")*opacity*col.alphaF();
            last=std::max(last,std::clamp(number(*stop,stop->a.value("offset","0"),"stop offset",true),0.,1.));
            if(i<8) {row.data[16+i]=last;row.data[24+i*4]=col.redF();row.data[25+i*4]=col.greenF();row.data[26+i*4]=col.blueF();row.data[27+i*4]=a;}
            else row.extraStops.push_back({float(last),Col(col.redF(),col.greenF(),col.blueF()),a});
            translucent|=a<.999f;++i;
        }
        if(!i) {c.color(Col(0,0,0),0);return;}
        const bool degenerate=g.tag=="linearGradient" ? row.data[10]==row.data[12] && row.data[11]==row.data[13] : row.data[14]==0;
        if(degenerate) {
            const auto last=g.children.back();const auto col=color(*last,last->a.value("stop-color","black"),s.color);
            c.color(Col(col.redF(),col.greenF(),col.blueF()),alpha(*last,"stop-opacity")*opacity*col.alphaF());return;
        }
        row.data[1]=i;c.gradient(row,translucent);
    }

    Col preciseColor(const Node& n,QString text,const QString& current) const {
        if(text=="currentColor")text=current;
        static const QRegularExpression rgb(R"(^rgb\(\s*([^,]+),\s*([^,]+),\s*([^\)]+)\)$)");
        const auto m=rgb.match(text);
        if(!m.hasMatch()) {const auto c=color(n,text,current);return Col(c.redF(),c.greenF(),c.blueF());}
        return Col(std::clamp(number(n,m.captured(1),"color",true,255)/255.,0.,1.),
                   std::clamp(number(n,m.captured(2),"color",true,255)/255.,0.,1.),
                   std::clamp(number(n,m.captured(3),"color",true,255)/255.,0.,1.));
    }
    using Replay=std::function<void(Canvas&,const Col*,double)>;
    Replay sourceReplay(const Node& n,const QString& text,double opacity,const Style& s,const QTransform& world,const QRectF& box) const {
        if(!text.startsWith("url(")) {
            const auto c=preciseColor(n,text,s.color);const auto a=opacity*color(n,text,s.color).alphaF();
            return [c,a](Canvas& target,const Col* tint,double alpha) {target.color(tint?*tint:c,a*alpha);};
        }
        Canvas sample(scale);paint(sample,n,text,opacity,s,world,box);
        const auto key=text.mid(text.indexOf('#')+1);const auto id=key.left(key.indexOf(')')).trimmed();
        const auto g=gradient(*ids[id]);
        if(sample.gradients().empty()) {
            if(g.children.empty())return [](Canvas& c,const Col*,double){c.color(Col(0,0,0),0);};
            const auto& last=*g.children.back();const auto col=preciseColor(last,last.a.value("stop-color","black"),s.color);
            const auto a=alpha(last,"stop-opacity")*opacity*color(last,last.a.value("stop-color","black"),s.color).alphaF();
            return [col,a](Canvas& c,const Col* tint,double alpha){c.color(tint?*tint:col,a*alpha);};
        }
        auto row=sample.gradients().back();bool translucent=false;
        // Use the native linear equation, and preserve float channels before
        // QColor's 16-bit conversion. Both are generic SVG paint operations.
        const bool nativeLinear=g.tag=="linearGradient"&&g.children.size()<=8;
        if(nativeLinear)row.data[0]=1;
        for(std::size_t i=0;i<g.children.size();++i) {
            const auto& stop=*g.children[i];auto c=preciseColor(stop,stop.a.value("stop-color","black"),s.color);
            const float a=alpha(stop,"stop-opacity")*opacity*color(stop,stop.a.value("stop-color","black"),s.color).alphaF();
            translucent|=a<.999f;
            if(i<8){row.data[24+i*4]=c.r*(nativeLinear?a:1);row.data[25+i*4]=c.g*(nativeLinear?a:1);row.data[26+i*4]=c.b*(nativeLinear?a:1);row.data[27+i*4]=a;}
            else {row.extraStops[i-8].c=c;row.extraStops[i-8].a=a;}
        }
        return [row,translucent](Canvas& c,const Col*,double alpha) {
            auto r=row;for(int i=0;i<std::min(8,int(r.data[1]));++i)for(int j=r.data[0]==1?0:3;j<4;++j)r.data[24+i*4+j]*=alpha;
            for(auto& s:r.extraStops)s.a*=alpha;
            c.gradientUserSpace(std::move(r),translucent||alpha<.999);
        };
    }
    Replay nativeReplay(const Node& n,const Style& s,const QTransform& world,const QPainterPath& path,std::optional<QPainterPath> clip) const {
        // Viewport clipping uses draw(); replay into an existing span is explicit.
        if(clip)return {};
        std::vector<SvgPathCommand> ops;QString error;QPainterPath normalized;
        if(n.tag=="path")svgPath(n.a.value("d"),normalized,error,&ops);
        else if(n.tag=="polygon"||n.tag=="polyline")svgPath("M"+n.a.value("points")+(n.tag=="polygon"?" Z":""),normalized,error,&ops);
        const auto tag=n.tag;
        std::array<double,6> shape{};
        if(tag=="rect")shape={value(n,"x"),value(n,"y"),value(n,"width"),value(n,"height"),value(n,"rx",value(n,"ry")),value(n,"ry",value(n,"rx"))};
        if(tag=="circle"||tag=="ellipse")shape={value(n,"cx"),value(n,"cy"),value(n,tag=="circle"?"r":"rx"),value(n,tag=="circle"?"r":"ry"),0,0};
        if(tag=="line")shape={value(n,"x1"),value(n,"y1"),value(n,"x2"),value(n,"y2"),0,0};
        auto geometry=[ops,tag,shape,path](Canvas& c) {
            if(tag=="rect"&&shape[4]==0&&shape[5]==0) {c.rect(shape[0],shape[1],shape[2],shape[3]);return;}
            if(tag=="circle"||tag=="ellipse") {c.ellipse(shape[0],shape[1],shape[2],shape[3]);return;}
            if(tag=="line") {c.moveTo(shape[0],shape[1]);c.lineTo(shape[2],shape[3]);return;}
            if(ops.empty()) {replay(c,path);return;}
            for(const auto& op:ops) {const auto& v=op.v;switch(op.code) {
                case 'M':c.moveTo(v[0],v[1]);break;case 'L':c.lineTo(v[0],v[1]);break;
                case 'Q':c.quadTo(v[0],v[1],v[2],v[3]);break;
                case 'C':c.curveTo(v[0],v[1],v[2],v[3],v[4],v[5]);break;case 'Z':c.closePath();break;
            }}
        };
        Replay fill,stroke;
        if(s.fill!="none")fill=sourceReplay(n,s.fill,s.fillAlpha,s,{},path.boundingRect());
        if(s.stroke!="none"&&s.width>0)stroke=sourceReplay(n,s.stroke,s.strokeAlpha,s,{},path.boundingRect());
        const double dot=world.m11()*world.m21()+world.m12()*world.m22();
        const double x2=world.m11()*world.m11()+world.m12()*world.m12(),y2=world.m21()*world.m21()+world.m22()*world.m22();
        const bool similarity=std::abs(dot)<1e-12&&std::abs(x2-y2)<1e-12;
        const bool round=s.cap==Qt::RoundCap&&s.join==Qt::RoundJoin&&similarity;
        QPainterPath outline;
        if(stroke&&!round) {QPainterPathStroker stroker;stroker.setWidth(s.width);stroker.setCapStyle(s.cap);stroker.setJoinStyle(s.join);stroker.setMiterLimit(s.miter/2.);stroker.setCurveThreshold(.01);outline=stroker.createStroke(path);}
        return [geometry,fill,stroke,world,s,round,outline,path,clip](Canvas& c,const Col* tint,double alpha) {
            c.save();c.transform(world.m11(),world.m12(),world.m21(),world.m22(),world.dx(),world.dy());
            if(fill) {fill(c,tint,alpha);geometry(c);c.fill(s.rule==Qt::OddEvenFill);}
            if(stroke) {stroke(c,tint,alpha);if(round){geometry(c);c.stroke(s.width);}else{replay(c,outline);c.fill(outline.fillRule()==Qt::OddEvenFill);}}
            c.restore();
        };
    }
    std::shared_ptr<const Canvas> compile(const std::shared_ptr<Node>& n,Style inherited={},QTransform parent={},bool record=true,bool definition=false,const Node* use=nullptr,std::optional<QPainterPath> clip=std::nullopt) {
        if(active.contains(n.get()))fail(*n,"cyclic use reference",false);
        if(++compilations>20000||active.size()>64)fail(*n,"compiled complexity limit",false);
        if(n->tag=="svg"&&!active.empty())fail(*n,"nested svg");
        active.insert(n.get());
        const auto s=style(*n,inherited);auto world=transform(*n)*parent;
        for(const double v:{world.m11(),world.m12(),world.m21(),world.m22(),world.dx(),world.dy()})
            if(!std::isfinite(v)||std::abs(v)>1e7)fail(*n,"transform exceeds coordinate limit",false);
        auto canvas=std::make_shared<Canvas>(scale);
        // Validate explicit paint even on empty/hidden elements and groups.
        for(const QString key:{"fill","stroke"})if(n->a.contains(key) && n->a[key]!="none" && n->a[key]!="inherit") {
            Canvas check(scale);paint(check,*n,n->a[key],1,s,world,QRectF(0,0,1,1));
        }
        auto path=geometry(*n);path.setFillRule(s.rule);
        std::vector<Replay> replayChildren;
        if(!path.isEmpty())replayChildren.push_back(nativeReplay(*n,s,world,path,clip));
        if(n->tag=="use") {
            QTransform placement;placement.translate(value(*n,"x"),value(*n,"y"));
            auto ref=reference(*n);if(ref->tag=="svg"||ref->tag=="defs"||ref->tag=="stop"||ref->tag.endsWith("Gradient"))fail(*n,"use target '"+ref->tag+"'");
            auto instance=compile(ref,s,placement*world,false,true,n.get(),clip);
            canvas->appendOwned(instance);
            replayChildren.push_back({});
        } else if(n->tag=="symbol" && n->a.contains("viewBox")) {
            const auto list=n->a["viewBox"].split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
            if(list.size()!=4)fail(*n,"invalid symbol viewBox",false);
            double v[4];for(int i=0;i<4;++i)v[i]=number(*n,list[i],"viewBox");
            if(v[2]<=0||v[3]<=0)fail(*n,"invalid symbol viewBox",false);
            if(n->a.value("preserveAspectRatio","xMidYMid meet")!="xMidYMid meet" && n->a["preserveAspectRatio"]!="none")fail(*n,"preserveAspectRatio '"+n->a["preserveAspectRatio"]+"'");
            if(use) {
                const double w=value(*use,"width",v[2]),h=value(*use,"height",v[3]);if(w<0||h<0)fail(*use,"invalid use dimensions",false);
                QPainterPath viewportClip;viewportClip.addRect(QRectF(0,0,w,h));viewportClip=world.map(viewportClip);
                clip=clip?clip->intersected(viewportClip):viewportClip;
                double sx=w/v[2],sy=h/v[3];QTransform viewport;
                if(n->a.value("preserveAspectRatio")!="none") {sx=sy=std::min(sx,sy);viewport.translate((w-v[2]*sx)/2,(h-v[3]*sy)/2);}
                viewport.scale(sx,sy);viewport.translate(-v[0],-v[1]);world=viewport*world;
            }
        }
        if(n->tag=="symbol" && use && !n->a.contains("viewBox")) {
            const double w=value(*use,"width",1920),h=value(*use,"height",1080);
            if(w<0||h<0)fail(*use,"invalid use dimensions",false);
            QPainterPath viewportClip;viewportClip.addRect(QRectF(0,0,w,h));viewportClip=world.map(viewportClip);
            clip=clip?clip->intersected(viewportClip):viewportClip;
        }
        if(!path.isEmpty()) {
            const auto raw=world.map(path);
            const auto transformed=clip?raw.intersected(*clip):raw;
            const auto extent=transformed.boundingRect();
            for(const double v:{extent.left(),extent.top(),extent.right(),extent.bottom(),s.width*std::max(std::abs(world.m11())+std::abs(world.m21()),std::abs(world.m12())+std::abs(world.m22()))})
                if(!std::isfinite(v)||std::abs(v)>1e7)fail(*n,"geometry exceeds coordinate limit",false);
            if(s.fill!="none") {paint(*canvas,*n,s.fill,s.fillAlpha,s,world,path.boundingRect());replay(*canvas,transformed);canvas->fill(transformed.fillRule()==Qt::OddEvenFill);}
            if(s.stroke!="none"&&s.width>0) {
                QPainterPathStroker stroker;stroker.setWidth(s.width);stroker.setCapStyle(s.cap);stroker.setJoinStyle(s.join);stroker.setMiterLimit(s.miter/2.);stroker.setCurveThreshold(.01);
                const auto outline=world.map(stroker.createStroke(path));const auto bounded=clip?outline.intersected(*clip):outline;
                paint(*canvas,*n,s.stroke,s.strokeAlpha,s,world,path.boundingRect());replay(*canvas,bounded);canvas->fill(bounded.fillRule()==Qt::OddEvenFill);
            }
        }
        const bool container=n->tag=="svg"||n->tag=="g"||n->tag=="defs"||n->tag=="symbol";
        if(!container && !n->tag.endsWith("Gradient") && !n->children.empty())fail(*n,"child elements on '"+n->tag+"'");
        for(const auto& child:n->children) {
            const bool hidden=child->tag=="defs"||child->tag=="symbol"||child->tag.endsWith("Gradient")||child->tag=="stop";
            auto compiled=compile(child,s,world,record,hidden,nullptr,clip);
            if(container && !hidden && (n->tag!="defs"||definition)) {
                canvas->appendOwned(compiled);
                if(record)replayChildren.push_back(art->elements_[child->index].replay);
            }
        }
        vertices+=canvas->vertices().size();if(vertices>2000000)fail(*n,"compiled vertex limit",false);
        canvas->freeze();std::shared_ptr<const Canvas> result=canvas;
        const double opacity=alpha(*n,"opacity");
        if(opacity!=1) {auto group=std::make_shared<Canvas>(scale);group->appendOwned(canvas,opacity);group->freeze();result=group;}
        if(record) {
            auto& entry=art->elements_[n->index];entry.geometry=path;entry.transform=world;entry.canvas=result;
            if(opacity==1&&std::all_of(replayChildren.begin(),replayChildren.end(),[](const auto& draw){return bool(draw);}))
                entry.replay=[children=std::move(replayChildren)](Canvas& c,const Col* tint,double alpha) {for(const auto& draw:children)draw(c,tint,alpha);};
        }
        active.remove(n.get());return result;
    }
    SvgImport run(const QByteArray& data) {
        try {
            if(!std::isfinite(scale)||scale<=0||scale>16)throw Failure{filename+": invalid pixel scale"};
            if(data.size()>16*1024*1024)throw Failure{filename+": SVG exceeds 16 MiB"};
            QXmlStreamReader xml(data);std::shared_ptr<Node> root;
            while(!xml.atEnd()) {
                xml.readNext();
                if(xml.isDTD()||xml.isEntityReference())throw Failure{filename+": DTD/entities are unsupported"};
                if(xml.isProcessingInstruction())throw Failure{QString("%1:%2: unsupported processing instruction '%3'").arg(filename).arg(xml.lineNumber()).arg(xml.processingInstructionTarget().toString())};
                if(xml.isStartElement()) {if(root)throw Failure{filename+": multiple root elements"};root=read(xml);}
            }
            if(xml.hasError())throw Failure{QString("%1:%2: XML error: %3").arg(filename).arg(xml.lineNumber()).arg(xml.errorString())};
            if(!root||root->tag!="svg")throw Failure{filename+": expected <svg> root"};
            const auto view=root->a.value("viewBox").split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
            if(view.size()!=4||number(*root,view[0],"viewBox")!=0||number(*root,view[1],"viewBox")!=0||number(*root,view[2],"viewBox")!=1920||number(*root,view[3],"viewBox")!=1080)fail(*root,"viewBox must be '0 0 1920 1080'",false);
            for(const auto& n:order)art->elements_.push_back({n->id,n->label,n->tag,n->line,{}, {}, {}, {}});
            art->root_=compile(root);
            // Definitions must also be validated when they have no consumers.
            for(const auto& n:order)if(n->tag.endsWith("Gradient")) {if(n->id.isEmpty())fail(*n,"gradient requires an id",false);Canvas check(scale);paint(check,*n,"url(#"+n->id+")",1,{}, {},QRectF(0,0,1,1));}
            return {art,{}};
        } catch(const Failure& f) {return {{},f.text};}
    }
};
bool SvgArt::draw(Canvas& target,const QString& id) const {
    for(const auto& e:elements_)if(e.id==id&&!id.isEmpty()) {target.appendOwned(e.canvas);return true;}return false;
}
bool SvgArt::replay(Canvas& target,const QString& id,const Col* tint,double alpha) const {
    for(const auto& e:elements_)if(e.id==id&&!id.isEmpty()&&e.replay) {e.replay(target,tint,alpha);return true;}return false;
}
SvgImport compileSvg(const QByteArray& data,const QString& filename,double pixelScale) {return SvgCompiler(filename,pixelScale).run(data);}
SvgImport importSvg(const QString& filename,double pixelScale) {
    QFile file(filename);if(!file.open(QIODevice::ReadOnly))return {{},filename+": cannot open SVG: "+file.errorString()};
    if(file.size()>16*1024*1024)return {{},filename+": SVG exceeds 16 MiB"};
    return compileSvg(file.readAll(),filename,pixelScale);
}
}
