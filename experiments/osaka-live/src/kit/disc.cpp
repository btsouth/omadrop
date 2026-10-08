#include "parameters.h"
#include "disc.h"
#include "../osaka_shaders.h"

namespace Journey {
namespace {
// The existing disc without veil or texture. Keep the authored halo, vertical
// paint blend and ring arithmetic, while avoiding thirteen unused noise octaves.
const char* plainDiscFs=R"(
uniform vec3 u_disc;
uniform float u_energy;
uniform vec4 u_rings[6];
uniform vec3 u_halo, u_col, u_col2, u_ring;
uniform vec4 u_haloK;
uniform float u_haloFar;
void main() {
    vec2 p = design();
    float d = length(p - u_disc.xy), r = u_disc.z, e = u_energy;
    float dd = max(d - r, 0.0);
    vec3 add = u_halo * (exp(-dd / (u_haloK.x + u_haloK.y * e)) * (u_haloK.z + u_haloK.w * e) + exp(-dd / 300.0) * u_haloFar);
    for (int k = 0; k < 6; ++k) add += u_ring * exp(-pow((d - u_rings[k].x) / 1.4, 2.0)) * u_rings[k].y;
    float m = ss(r + 1.2, r - 1.2, d);
    vec3 dc = mix(u_col, u_col2, ss(-u_disc.z, u_disc.z, p.y - u_disc.y));
    o = vec4(add * (1.0 - m) + m * dc, m);
}
)";
}
void Kit::OsakaDiscV1::draw(Ctx& c, const DiscLook& d, double camForClouds) {
    const auto& p = osakaParameters().disc;
    GpuProfile::Group profileGroup(c.gpu.profile,"drawDisc");
    const bool plain=d.tex==0 && d.veil==0;
    std::string body=plain ? plainDiscFs : std::string(Shaders::cloud)+Shaders::disc;
    Program& program=c.gpu.effect(plain ? "plain-disc" : "disc",body.c_str());
    // Two resting rings breathe; strong bass sends rings travelling outward.
    float rings[24] = {};
    const double e = d.energy;
    rings[0] = float(d.r + p.ring0Offset + p.ring0Energy * e); rings[1] = float(p.ring0Alpha * (p.ringAlphaBase + e) * d.restRings);
    rings[4] = float(d.r + p.ring1Offset + p.ring1Energy * e); rings[5] = float(p.ring1Alpha * (p.ringAlphaBase + e) * d.restRings);
    int k = 2;
    if (c.score) {
        for (auto it = c.score->bassHits.rbegin(); it != c.score->bassHits.rend() && k < 6; ++it) {
            const double age = c.t - it->t;
            if (age < 0) continue;
            if (age > p.hitSeconds) break;
            if (it->strength < p.hitThreshold) continue;
            const double u = age / p.hitSeconds;
            rings[k * 4] = float(d.r + p.hitOffset + p.hitTravel * easeOut(u));
            rings[k * 4 + 1] = float(p.hitAlpha * it->strength * (1 - u) * (1 - u));
            ++k;
        }
    }
    c.gpu.pass(program, Blend::Over, [&](Program& q) {
        q.set("u_cam", float(camForClouds)); q.set("u_t", float(c.t + p.timeOffset));
        q.set("u_disc", float(d.pos.x), float(d.pos.y), float(d.r));
        q.set("u_energy", float(e)); q.set("u_veil", float(d.veil)); q.set("u_texAmt", float(d.tex));
        q.setArray("u_rings", rings, 6, 4);
        q.set("u_halo", d.halo); q.set("u_col", d.col); q.set("u_col2", d.col2); q.set("u_ring", d.ring);
        q.set("u_haloK", float(d.haloA), float(d.haloB), float(d.haloC), float(d.haloD)); q.set("u_haloFar", float(d.haloFar));
    }, -1, c.staticGeometry ? QRectF(0,0,1920,640) : QRectF());
}
}
