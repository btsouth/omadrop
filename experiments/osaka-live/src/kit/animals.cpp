#include "animals.h"
#include "palette.h"
#include "primitives.h"
#include "pane.h"
#include "haze.h"
#include "../rig.h"
#include "../osaka_shaders.h"
#include <cmath>
#include <vector>

namespace Journey::Kit {
using Life = OsakaLegacyLife;
void OsakaMothsV1::draw(Ctx& c, const OsakaState& s, double px) {
    GpuProfile::Group profileGroup(c.gpu.profile,"moths");
    const double t = c.t, py = 612;
    if (px < -40 || px > 1960) return;
    Canvas& m = c.canvas();
    const double busy = 1 + 0.8 * c.lift(5);
    for (int k = 0; k < 4; ++k) {
        const double ph = t * (1.6 + 0.5 * k) * busy + k * 1.7 + 0.8 * noise1(t * 1.3, k);
        const double r = 16 + 9 * k + 6 * noise1(t * 0.9, k + 5);
        const V2 q(px + std::cos(ph) * r, py + 6 + std::sin(ph * 1.3) * r * 0.55);
        const double flap = 0.5 + 0.5 * std::sin(t * 40 + k * 3);
        m.ellipse(q.x, q.y, 2.0 + 1.2 * flap, 1.4);
        m.color(Col(0.9f, 0.95f, 0.8f), 0.85);
        m.fill();
    }
    c.gpu.over(m, 1.4f);
    c.gpu.add(m, 0.5f, 3);
}

void OsakaRailCatV1::draw(Ctx& c, const Life& L, Canvas& p, double t, double ox) {
    // Cat hops onto the railing at 13 s, walks, pauses, sits; startles at the surge.
    {
        const double railY = 863;
        double cx, dist = 0, sit = 0, look = 0, crouch = 0, y = railY;
        dist=150; cx=600+dist; sit=1;
        look=c.gesture(15.3,0.3,16.0,0.3)*0.6;
        {
            const double startle = L.look;
            CatPose cp;
            cp.pos = {cx + ox, y};
            cp.s = 21; cp.facing = 1; cp.distance = dist; cp.sit = sit * (1 - startle * 0.8);
            cp.look = look + startle * 1.2 + 0.6 * c.gesture(13.5, 0.3, 14.5, 0.35); cp.crouch = crouch + startle * 0.5;
            cp.tailBase = 0.3 * startle; cp.tailWave = 0.25 + 0.1 * std::sin(t * 0.7); cp.tailPhase = t * 3.1;
            drawCat(p, cp, INK);
        }
    }
}

void OsakaSillCatV1::draw(const Life& L, Canvas& sh, double t, double ox) {
        // A cat on the sill: sits, tail sways, ears turn.
        CatPose cp;
        cp.pos = {446 + ox, 508}; cp.s = 40; cp.facing = -1; cp.sit = 1;
        cp.tailWave = 0.35; cp.tailPhase = t * 2.2; cp.look = 0.3 * std::sin(t * 0.5) + L.look;
        drawCat(sh, cp, SHADOW);
}

void OsakaVerandaCatV1::draw(Ctx& c, const OsakaState& s, const Life& L, Canvas& f, double t, double ox) {
        // Veranda cat flicks its tail at 2 s, then sways it lazily.
        CatPose cp;
        const double greet = c.gesture(13.05, 0.25, 14.25, 0.4);
        cp.pos = {420 + ox, 968}; cp.s = 27; cp.facing = 1; cp.sit = 1 - 0.8 * greet;
        cp.crouch = 0.3 * greet;
        const double flick = s.chapter ? ring(t - 2.0, 2.4, 2.2) : 0;
        cp.tailWave = 0.18 + 0.9 * flick + 0.45 * greet; cp.tailPhase = t * 2.0 + flick * 4;
        cp.look = 0.25 * std::max(0.0, std::sin(t * 0.37)) + 1.1 * L.look + 0.6 * greet;
        drawCat(f, cp, INK);
}

}
