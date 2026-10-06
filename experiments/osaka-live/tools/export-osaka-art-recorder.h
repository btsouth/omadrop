
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
namespace Journey {
// Records original design-space commands, never tessellated points.
struct Stop { float offset; Col c; float a; };
struct Canvas {
    std::string path, paint, group; int serial=0;
    static std::string num(double v) { std::ostringstream out; out<<std::setprecision(17)<<v; return out.str(); }
    static std::string rgb(Col c) { return "rgb("+num(double(c.r)*100)+"%,"+num(double(c.g)*100)+"%,"+num(double(c.b)*100)+"%)"; }
    void end() { if(!group.empty()) std::cout<<"</g>\n"; group.clear(); }
    void begin(std::string id) { end(); group=id; serial=0; std::cout<<"<g id=\""<<id<<"\" data-name=\""<<id<<"\">\n"; }
    void color(Col c,double a=1) { paint="fill=\""+rgb(c)+"\" fill-opacity=\""+num(a)+"\""; }
    void linear(double x,double y,double xx,double yy,std::initializer_list<Stop> stops) {
        auto id=group+"-gradient-"+std::to_string(++serial);
        std::cout<<"<defs><linearGradient id=\""<<id<<"\" gradientUnits=\"userSpaceOnUse\" x1=\""<<num(x)<<"\" y1=\""<<num(y)<<"\" x2=\""<<num(xx)<<"\" y2=\""<<num(yy)<<"\">\n";
        for(auto s:stops) std::cout<<"<stop offset=\""<<num(s.offset)<<"\" stop-color=\""<<rgb(s.c)<<"\" stop-opacity=\""<<num(s.a)<<"\"/>\n";
        std::cout<<"</linearGradient></defs>\n"; paint="fill=\"url(#"+id+")\"";
    }
    void op(char cmd,std::initializer_list<double> v={}) { path+=cmd; for(auto x:v) path+=" "+num(x); path+=" "; }
    void moveTo(double x,double y) { op('M',{x,y}); }
    void lineTo(double x,double y) { op('L',{x,y}); }
    void curveTo(double a,double b,double c,double d,double e,double f) { op('C',{a,b,c,d,e,f}); }
    void quadTo(double a,double b,double c,double d) { op('Q',{a,b,c,d}); }
    void closePath() { op('Z'); }
    void fill() { std::cout<<"<path id=\""<<group<<"-"<<++serial<<"\" d=\""<<path<<"\" "<<paint<<" fill-rule=\"nonzero\"/>\n"; path.clear(); }
    void stroke(double width) { auto p=paint; auto at=p.find("fill"); p.replace(at,4,"stroke"); at=p.find("fill-opacity"); if(at!=std::string::npos)p.replace(at,12,"stroke-opacity"); std::cout<<"<path id=\""<<group<<"-"<<++serial<<"\" d=\""<<path<<"\" fill=\"none\" "<<p<<" stroke-width=\""<<num(width)<<"\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n"; path.clear(); }
    void rect(double x,double y,double w,double h) { moveTo(x,y);lineTo(x+w,y);lineTo(x+w,y+h);lineTo(x,y+h);closePath(); }
    void fillRect(double x,double y,double w,double h,Col c,double a=1) { color(c,a); std::cout<<"<rect id=\""<<group<<"-"<<++serial<<"\" x=\""<<num(x)<<"\" y=\""<<num(y)<<"\" width=\""<<num(w)<<"\" height=\""<<num(h)<<"\" "<<paint<<"/>\n"; }
    void line(double x,double y,double xx,double yy,double w,Col c,double a=1) { color(c,a);moveTo(x,y);lineTo(xx,yy);stroke(w); }
    void disc(double x,double y,double r,Col c,double a=1) { color(c,a);std::cout<<"<circle id=\""<<group<<"-"<<++serial<<"\" cx=\""<<num(x)<<"\" cy=\""<<num(y)<<"\" r=\""<<num(r)<<"\" "<<paint<<"/>\n"; }
};
struct OsakaState { double cam=0; };
struct Ctx { template<class F> void retain(Canvas& cv,const std::string& id,F f,std::initializer_list<double>) { cv.begin(id);f(cv); } };
}
