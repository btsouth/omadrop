#include "gradient-sky.h"
namespace Journey::Kit {
namespace {
const char* gradientSkyFs = R"(
uniform vec4 u_stops[8];
uniform int u_count;
uniform vec3 u_paperTop, u_paperBottom;
uniform float u_printGrade, u_grain;
void main() {
    vec2 p = design();
    vec3 c = u_stops[0].rgb;
    for (int i = 1; i < u_count; ++i) {
        float a = u_stops[i-1].a, b = u_stops[i].a;
        c = mix(c, u_stops[i].rgb, clamp((p.y-a)/(b-a),0.0,1.0));
    }
    vec3 paper = mix(u_paperTop, u_paperBottom, smoothstep(190.0,640.0,p.y));
    c = mix(c,paper,u_printGrade);
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
        q.set("u_printGrade",float(p.printGrade)); q.set("u_grain",float(p.grain));
    });
}
}
