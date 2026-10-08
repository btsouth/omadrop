#include "gradient-sky.h"
#include "print-cloud.h"
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
        // Printed kasumi clouds: stacked tiers with scalloped tops, flat
        // undersides and rounded ends, cut as crisp blocks. Each tier carries
        // a muted underprint offset below it, a cream fill graded toward the
        // sky at its underside (bokashi), a thin key line and nested lobe
        // lines. Back tiers print first so the front tier's key line shapes
        // the stack. They drift slowly downwind, a little faster while the
        // music is loud, and wrap fully offscreen.
        auto skyAt=[&](double y) {
            Col col=p.stops.front().color;
            for(std::size_t i=1;i<p.stops.size();++i)
                col=mix(col,p.stops[i].color,clamp01((y-p.stops[i-1].y)/(p.stops[i].y-p.stops[i-1].y)));
            return mix(col,mix(p.paperTop,p.paperBottom,sstep(190,640,y)),p.printGrade);
        };
        struct Band {double x,y,w,h,step;int tiers;};
        static const Band bands[]={{360,128,760,30,.62,3},{1500,212,620,24,.55,2},{560,330,520,20,.6,2},{1640,402,380,16,.5,2}};
        for(std::size_t i=0;i<std::size(bands);++i) {
            const auto& b=bands[i];
            const double loud=c.score?c.score->bandIntegrals[1]+c.score->bandIntegrals[2]:0;
            const double reach=b.w*.75+120,span=1920+2*reach;
            double x=std::fmod(b.x+reach+(5+1.6*i)*c.t+10*loud,span);if(x<0)x+=span;x-=reach;
            const Col sky=skyAt(b.y),cream=hex(0xf4ead2);
            const Col light=mix(sky,cream,.94),under=mix(sky,cream,.38);
            const Col shadow=mix(sky,hex(0x7397a4),.30),key=mix(sky,hex(0x54546d),.55);
            for(int t=b.tiers-1;t>=0;--t) {
                const double w=b.w*std::pow(b.step,t),body=b.h*.62*(1-.12*t),lift=b.h*(1.35-.12*t);
                const double base=b.y+b.h/2-t*b.h*.58,cx=x+(t%2?-1:1)*b.w*.09*t;
                const auto tier=printCloudTier(cx,base,w,body,lift,2+int(w/(b.h*(4.4-.4*t))),i*7+t);
                cv.color(shadow);tracePrintCloud(cv,tier,2.5,3);cv.fill();
                cv.linear(0,tier.top,0,base,{{0,light,1},{.45,mix(light,under,.25),1},{1,under,1}});
                tracePrintCloud(cv,tier);cv.fill();
                cv.color(key,.55);tracePrintCloud(cv,tier);cv.stroke(1.3);
                printCloudEchoes(cv,tier,1.1,key,.32,b.h*.42);
            }
        }
        c.gpu.over(cv);
    }
}
}
