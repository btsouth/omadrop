#include "disc.h"
#include "../osaka_shaders.h"

namespace Journey {
void Kit::OsakaDiscV1::draw(Ctx& c, const DiscLook& d, double camForClouds) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawDisc");
    std::string body = std::string(Shaders::cloud) + Shaders::disc;
    Program& p = c.gpu.effect("disc", body.c_str());
    // Two resting rings breathe; strong bass sends rings travelling outward.
    float rings[24] = {};
    const double e = d.energy;
    rings[0] = float(d.r + 34 + 10 * e); rings[1] = float(0.10 * (0.5 + e) * d.restRings);
    rings[4] = float(d.r + 80 + 14 * e); rings[5] = float(0.06 * (0.5 + e) * d.restRings);
    int k = 2;
    if (c.score) {
        for (auto it = c.score->bassHits.rbegin(); it != c.score->bassHits.rend() && k < 6; ++it) {
            const double age = c.t - it->t;
            if (age < 0) continue;
            if (age > 2.6) break;
            if (it->strength < 0.35) continue;
            const double u = age / 2.6;
            rings[k * 4] = float(d.r + 24 + 300 * easeOut(u));
            rings[k * 4 + 1] = float(0.22 * it->strength * (1 - u) * (1 - u));
            ++k;
        }
    }
    c.gpu.pass(p, Blend::Over, [&](Program& q) {
        q.set("u_cam", float(camForClouds)); q.set("u_t", float(c.t + 10));
        q.set("u_disc", float(d.pos.x), float(d.pos.y), float(d.r));
        q.set("u_energy", float(e)); q.set("u_veil", float(d.veil)); q.set("u_texAmt", float(d.tex));
        q.setArray("u_rings", rings, 6, 4);
        q.set("u_halo", d.halo); q.set("u_col", d.col); q.set("u_col2", d.col2); q.set("u_ring", d.ring);
        q.set("u_haloK", float(d.haloA), float(d.haloB), float(d.haloC), float(d.haloD)); q.set("u_haloFar", float(d.haloFar));
    }, -1, c.staticGeometry ? QRectF(0,0,1920,640) : QRectF());
}
}
