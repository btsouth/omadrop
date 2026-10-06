#include "gradient-sky.h"
namespace Journey::Kit {
namespace {
const char* gradientSkyFs = R"(
uniform vec4 u_stops[8];
uniform int u_count;
uniform vec3 u_paperTop, u_paperBottom;
uniform float u_printGrade, u_grain, u_warmth;
void main() {
    vec2 p = design();
    vec3 c = u_stops[0].rgb;
    for (int i = 1; i < u_count; ++i) {
        float a = u_stops[i-1].a, b = u_stops[i].a;
        c = mix(c, u_stops[i].rgb, clamp((p.y-a)/(b-a),0.0,1.0));
    }
    vec3 paper = mix(u_paperTop, u_paperBottom, smoothstep(190.0,640.0,p.y));
    c = mix(c,paper,u_printGrade);
    c += u_warmth * vec3(0.055,0.025,-0.018);
    c += (hash12(floor(p))-0.5)*u_grain;
    o = vec4(c,1.0);
}
)";
}
void GradientSkyV1::draw(Ctx& c, const GradientSkyParametersV1& p) {
    GpuProfile::Group group(c.gpu.profile,"gradient-sky-v1");
    float stops[32] = {};
    for (std::size_t i=0; i<p.stops.size(); ++i) {
        const auto& s=p.stops[i];
        stops[4*i]=s.color.r; stops[4*i+1]=s.color.g; stops[4*i+2]=s.color.b; stops[4*i+3]=float(s.y);
    }
    auto& program=c.gpu.effect(name,gradientSkyFs);
    // Preserve the prototype's stored sky before the disc. Fusing this ramp
    // with a full-frame disc reassociates arithmetic, while its clipped cached
    // path stays separate, producing widespread 1/255 differences on Intel.
    program.fusible=false;
    c.gpu.pass(program,Blend::Replace,[&](Program& q) {
        q.setArray("u_stops",stops,8,4); q.set("u_count",int(p.stops.size()));
        q.set("u_paperTop",p.paperTop); q.set("u_paperBottom",p.paperBottom);
        q.set("u_warmth",float(p.energyGrade*(c.score?clamp01(2.4*(c.score->bandBody[2][0]+c.score->bandBody[2][1])):0)));
        q.set("u_printGrade",float(p.printGrade)); q.set("u_grain",float(p.grain));
    });
    if(p.cloudBands){
        Canvas& cv=c.canvas();
        // Layered bokashi bands: tapered ink silhouettes with a fine paper rim.
        // Both depths drift continuously; the long bands cover their whole lane.
        for(int depth=0;depth<2;++depth)for(int i=0;i<3;++i){
            const double clock=c.t*(depth?.013:.006)+i*1.8;
            const double x=320+i*580+70*std::sin(clock),y=160+i*94+depth*37;
            const double w=depth?430:560,h=depth?20:12;
            const Col ink=mix(p.paperBottom,hex(0x725b69),depth?.56:.42);
            cv.color(ink,depth?.25:.20);cv.moveTo(x-w,y);
            cv.curveTo(x-w*.66,y-h,x-w*.20,y-h*.3,x,y-h*.6);
            cv.curveTo(x+w*.35,y-h*1.2,x+w*.7,y-h*.7,x+w,y);
            cv.curveTo(x+w*.45,y+h*.35,x-w*.50,y+h*.60,x-w,y);cv.closePath();cv.fill();
            cv.color(p.paperTop,depth?.18:.12);cv.moveTo(x-w*.80,y+h*.08);
            cv.curveTo(x-w*.25,y-h*.3,x+w*.30,y-h*.2,x+w*.77,y-h*.15);cv.stroke(depth?1.8:1.1);
        }
        c.gpu.over(cv);
    }
}
}
