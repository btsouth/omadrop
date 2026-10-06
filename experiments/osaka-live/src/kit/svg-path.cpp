#include "svg-art.h"
#include <QRegularExpression>
#include <cmath>

namespace Journey::Kit {
namespace {
constexpr double pi = 3.14159265358979323846;
class Numbers {
public:
    QString s; int at=0;
    explicit Numbers(QString text):s(std::move(text)) {}
    void space() { while(at<s.size() && (s[at].isSpace() || s[at]==',')) ++at; }
    bool number(double& out) {
        space(); static const QRegularExpression re(R"([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)");
        auto m=re.match(s,at,QRegularExpression::NormalMatch,QRegularExpression::AnchorAtOffsetMatchOption);
        if(!m.hasMatch())return false;
        bool ok=false;out=m.captured().toDouble(&ok);at=m.capturedEnd();
        return ok && std::isfinite(out) && std::abs(out)<=1e9;
    }
    bool flag(double& out) {space();if(at>=s.size() || (s[at]!='0' && s[at]!='1'))return false;out=s[at++].digitValue();return true;}
    bool end() {space();return at==s.size();}
};
void arc(QPainterPath& p,double rx,double ry,double degrees,bool large,bool sweep,QPointF end) {
    const auto start=p.currentPosition();rx=std::abs(rx);ry=std::abs(ry);
    if(start==end)return;
    if(rx==0 || ry==0) {p.lineTo(end);return;}
    const double phi=std::fmod(degrees,360.)*pi/180.,co=std::cos(phi),si=std::sin(phi);
    const auto half=(start-end)/2.;
    const double x=co*half.x()+si*half.y(),y=-si*half.x()+co*half.y();
    const double radius=x*x/(rx*rx)+y*y/(ry*ry);
    if(radius>1) {rx*=std::sqrt(radius);ry*=std::sqrt(radius);}
    const double denom=rx*rx*y*y+ry*ry*x*x;
    const double k=(large==sweep?-1.:1.)*std::sqrt(std::max(0.,(rx*rx*ry*ry-denom)/denom));
    const double cx=k*rx*y/ry,cy=-k*ry*x/rx;
    const auto mid=(start+end)/2.;
    const QPointF center(co*cx-si*cy+mid.x(),si*cx+co*cy+mid.y());
    const double ux=(x-cx)/rx,uy=(y-cy)/ry,vx=(-x-cx)/rx,vy=(-y-cy)/ry;
    const double a0=std::atan2(uy,ux);double delta=std::atan2(ux*vy-uy*vx,ux*vx+uy*vy);
    if(!sweep && delta>0)delta-=2*pi;
    if(sweep && delta<0)delta+=2*pi;
    const int n=std::max(1,int(std::ceil(std::abs(delta)/(pi/2))));
    auto point=[&](double a){return center+QPointF(co*rx*std::cos(a)-si*ry*std::sin(a),si*rx*std::cos(a)+co*ry*std::sin(a));};
    auto tangent=[&](double a){return QPointF(-co*rx*std::sin(a)-si*ry*std::cos(a),-si*rx*std::sin(a)+co*ry*std::cos(a));};
    for(int i=0;i<n;++i) {
        const double a=a0+delta*i/n,b=a0+delta*(i+1)/n,h=4./3.*std::tan((b-a)/4.);
        p.cubicTo(point(a)+tangent(a)*h,point(b)-tangent(b)*h,i+1==n?end:point(b));
    }
}
}
bool svgPath(const QString& data,QPainterPath& output,QString& error,std::vector<SvgPathCommand>* commands) {
    std::vector<SvgPathCommand> ops;
    auto op=[&](char code,std::initializer_list<double> values) { SvgPathCommand c{code};std::copy(values.begin(),values.end(),c.v.begin());ops.push_back(c); };
    auto point=[&](char code,QPointF a) {op(code,{a.x(),a.y()});};
    Numbers in(data);QPainterPath p;QPointF cubic,quad;QChar command,previous;
    auto fail=[&] {error=QString("invalid path at offset %1").arg(in.at);return false;};
    int operations=0;
    while(!in.end()) {
        if(++operations>100000)return fail();
        if(in.s[in.at].isLetter())command=in.s[in.at++];
        else if(command.isNull())return fail();
        const QChar code=command.toUpper();const bool relative=command.isLower();
        if(p.elementCount()==0 && code!='M')return fail();
        if(code=='Z') {op('Z',{});p.closeSubpath();previous=code;command=QChar();continue;}
        const int count=code=='H'||code=='V'?1:code=='M'||code=='L'||code=='T'?2:code=='Q'||code=='S'?4:code=='C'?6:code=='A'?7:0;
        if(!count)return fail();
        double v[7]{};
        for(int i=0;i<count;++i)if(!(code=='A' && (i==3||i==4)?in.flag(v[i]):in.number(v[i])))return fail();
        const auto origin=p.currentPosition();
        auto xy=[&](int i){return QPointF(v[i],v[i+1])+(relative?origin:QPointF());};
        if(code=='M') {point('M',xy(0));p.moveTo(xy(0));command=relative?'l':'L';}
        else if(code=='L') {point('L',xy(0));p.lineTo(xy(0));}
        else if(code=='H') {QPointF a(relative?origin.x()+v[0]:v[0],origin.y());point('L',a);p.lineTo(a);}
        else if(code=='V') {QPointF a(origin.x(),relative?origin.y()+v[0]:v[0]);point('L',a);p.lineTo(a);}
        else if(code=='C') {cubic=xy(2);auto a=xy(0),b=xy(4);op('C',{a.x(),a.y(),cubic.x(),cubic.y(),b.x(),b.y()});p.cubicTo(a,cubic,b);}
        else if(code=='S') {const auto reflected=previous=='C'||previous=='S'?2*origin-cubic:origin;cubic=xy(0);auto b=xy(2);op('C',{reflected.x(),reflected.y(),cubic.x(),cubic.y(),b.x(),b.y()});p.cubicTo(reflected,cubic,b);}
        else if(code=='Q') {quad=xy(0);auto b=xy(2);op('Q',{quad.x(),quad.y(),b.x(),b.y()});p.quadTo(quad,b);}
        else if(code=='T') {quad=previous=='Q'||previous=='T'?2*origin-quad:origin;auto b=xy(0);op('Q',{quad.x(),quad.y(),b.x(),b.y()});p.quadTo(quad,b);}
        else {
            const int first=p.elementCount();arc(p,v[0],v[1],v[2],v[3]!=0,v[4]!=0,xy(5));
            for(int i=first;i<p.elementCount();++i) {
                auto a=p.elementAt(i);
                if(a.isLineTo())op('L',{a.x,a.y});
                else if(a.type==QPainterPath::CurveToElement) {auto b=p.elementAt(++i),c=p.elementAt(++i);op('C',{a.x,a.y,b.x,b.y,c.x,c.y});}
            }
        }
        previous=code;
    }
    output=p;if(commands)*commands=std::move(ops);return true;
}
bool svgTransform(const QString& data,QTransform& output,QString& error) {
    Numbers in(data);QTransform result;
    while(!in.end()) {
        const int begin=in.at;while(in.at<in.s.size() && in.s[in.at].isLetter())++in.at;
        const auto name=in.s.mid(begin,in.at-begin);in.space();
        if(in.at==in.s.size() || in.s[in.at++]!='(') {error="invalid transform";return false;}
        std::vector<double> v;in.space();
        while(in.at<in.s.size() && in.s[in.at]!=')') {double x;if(v.size()==6 || !in.number(x)){error="invalid transform";return false;}v.push_back(x);in.space();}
        if(in.at==in.s.size()) {error="invalid transform";return false;}++in.at;
        QTransform t;
        if(name=="matrix" && v.size()==6)t=QTransform(v[0],v[1],v[2],v[3],v[4],v[5]);
        else if(name=="translate" && (v.size()==1||v.size()==2))t.translate(v[0],v.size()==2?v[1]:0);
        else if(name=="scale" && (v.size()==1||v.size()==2))t.scale(v[0],v.size()==2?v[1]:v[0]);
        else if(name=="rotate" && (v.size()==1||v.size()==3)) {if(v.size()==3)t.translate(v[1],v[2]);t.rotate(v[0]);if(v.size()==3)t.translate(-v[1],-v[2]);}
        else if((name=="skewX"||name=="skewY") && v.size()==1) {const double k=std::tan(v[0]*pi/180.);if(!std::isfinite(k)||std::abs(k)>1e9){error="invalid transform";return false;}t.shear(name=="skewX"?k:0,name=="skewY"?k:0);}
        else {error="unsupported transform '"+name+"'";return false;}
        result=t*result; // Qt maps row vectors; SVG postmultiplies column vectors.
    }
    output=result;return true;
}
}
