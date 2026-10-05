#include "gpu.h"
#include "headless.h"
#include <QCoreApplication>
#include <iostream>
#include <cstdlib>
using namespace Journey;
static std::vector<unsigned char> capture(Gpu& gpu,int test) {
    gpu.begin(320,180);
    const std::string common="uniform float u_gain; uniform vec4 u_col;\n";
    auto& a=gpu.effect("a",(common+"void main(){o=u_col*u_gain;}").c_str());
    auto& b=test==1 || test==2 ? a : gpu.effect("b",(common+"void main(){o=u_col*(u_gain*1.0);}").c_str());
    const QRectF clip=test==3 ? QRectF(0,0,1920,540) : QRectF();
    gpu.pass(a,Blend::Replace,[&](Program& p) {p.set("u_gain",test==2 ? 0.5f : 1.f);p.set("u_col",.125f,.25f,.5f,1.f);},-1,clip);
    if(test==4) {
        Canvas c; c.color(Col(.25f,.125f,.5f),.5);c.rect(480,270,960,540);c.fill();gpu.draw(c);
    }
    gpu.pass(b,test==5 ? Blend::Add : Blend::Over,[](Program& p) {p.set("u_gain",1.f);p.set("u_col",.0625f,.125f,.0625f,.5f);});
    FinishParams finish;finish.bloom=finish.vignette=finish.grain=0;
    gpu.finish(finish,nullptr);
    std::vector<unsigned char> rgb;gpu.readRgb(rgb);return rgb;
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);HeadlessContext context;QString error;
    if(!context.create(error)) {std::cerr<<error.toStdString()<<'\n';return 1;}
    std::cout<<"GPU: "<<context.renderer().toStdString()<<'\n';
    qputenv("OSAKA_DISABLE_EFFECT_FUSION","1");Gpu reference;
    if(!reference.init(error))return 1;
    qunsetenv("OSAKA_DISABLE_EFFECT_FUSION");Gpu fused;
    if(!fused.init(error))return 1;
    for(int test=0;test<6;++test) {
        const auto before=capture(reference,test),after=capture(fused,test);
        if(before!=after) {std::cerr<<"effect fusion differs in case "<<test<<'\n';return 1;}
    }
    std::cout<<"PASS exact RGB: Over, same-program snapshots, differing shared inputs, clip boundaries, intervening canvas, Add\n";
}
