#include "flock.h"
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
namespace {
double easedDistance(double t, double t0, double t1, double d0, double d1) {
    if (t <= t0) return d0;
    if (t >= t1) return d1;
    const double u = (t - t0) / (t1 - t0);
    // Accelerate over the first 12%, cruise, decelerate over the last 15%.
    const double a = 0.12, b = 0.15;
    const double v = 1.0 / (1 - a / 2 - b / 2);
    double s;
    if (u < a) s = v * u * u / (2 * a);
    else if (u < 1 - b) s = v * (a / 2 + (u - a));
    else { const double r = 1 - u; s = 1 - v * r * r / (2 * b); }
    return lerp(d0, d1, s);
}
const double SLOTS[15][2] = {{1, 0.33}, {1, 0.37}, {3, 0.42}, {0, 0.47}, {2, 0.50}, {2, 0.535}, {4, 0.57}, {1, 0.61},
                             {1, 0.64}, {3, 0.67}, {5, 0.71}, {0, 0.74}, {2, 0.79}, {4, 0.83}, {3, 0.26}};



V2 flockCentre(double t, double t0) {
    // After the burst the flock wheels up, regroups and holds near the moon's
    // side of the sky in screen space, drifting as the camera glides.
    const double u = t - t0;
    const V2 a(1260, 230);
    return a + V2(120*std::sin(u*0.35)+40*u*0.2+easedDistance(u,7,12,0,1500),
                  -30*std::sin(u*0.5)+25*std::sin(u*0.21));
}
const double V_FORM[15][3] = {{0, 0, 1.0}, {-46, 22, 0.95}, {-52, -30, 0.9}, {-100, 44, 0.9}, {-112, -58, 0.85},
                              {-160, 70, 0.8}, {-176, -84, 0.8}, {-70, -4, 0.7}, {-230, 96, 0.75}, {-250, -110, 0.7},
                              {-140, 10, 0.7}, {-300, 30, 0.65}, {-210, -40, 0.7}, {-280, -70, 0.66}, {-330, 60, 0.6}};


}

std::vector<BirdPlan> birdPlan(const Ctx& c) {
    std::vector<BirdPlan> plan;
    for (int k=0;k<15;++k) {
        const double land=c.schedule->birdLand[k];
        const int order = int(std::fmod(k * 7 + c.seed * 3, 15.0));
        BirdPlan b;
        b.land = land; b.wire = int(SLOTS[order][0]); b.u = SLOTS[order][1];
        b.face = c.jit(90 + k) > -0.4 ? 1 : -1;
        b.fromA = -Pi / 2 + 0.9 * c.jit(110 + k) + (c.jit(130 + k) > 0 ? 0.7 : -0.7);
        b.fromD = 520 + 200 * hash2(k, c.seed);
        plan.push_back(b);
    }
    return plan;
}

void OsakaFlockV1::draw(Ctx& c, const OsakaState& s, const Life& L, const std::vector<BirdPlan>& plan) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawBirds");
    const double t = c.t;
    Canvas& cv = c.canvas();
    const Spans spans = wireRuns(s.cam);
    for (std::size_t k = 0; k < plan.size(); ++k) {
        const BirdPlan& b = plan[k];
        const WireSpan& w = spans[1][std::size_t(b.wire)];
        const V2 slot = wireAt(w, b.u) + V2(0, 1.4 + 3.0 * ring(t - b.land, 2.2, 3.5));
        const double fly = 1.15;
        const double burst=c.schedule->fireworks>=b.land?c.schedule->fireworks+0.04*k+0.12*hash2(k,3):1e9;
        if (t < b.land - fly) continue;
        if (t < b.land) {
            // Approach on a curve, flare and land.
            const double u = (t - (b.land - fly)) / fly;
            const V2 direction(std::cos(b.fromA),std::sin(b.fromA));
            const double tx=direction.x>0?(2000-slot.x)/direction.x:(-80-slot.x)/direction.x;
            const double ty=direction.y>0?(1160-slot.y)/direction.y:(-80-slot.y)/direction.y;
            const double distance=std::max(b.fromD,std::min(tx,ty)+80);
            const V2 from=slot+direction*distance;
            const V2 ctrl = lerp(from, slot, 0.5) + V2(0, -120);
            const double e = easeOut(u);
            const V2 q = from * ((1 - e) * (1 - e)) + ctrl * (2 * (1 - e) * e) + slot * (e * e);
            const double flare = sstep(0.75, 1.0, u);
            const double flap = lerp(0.5 + 0.5 * std::sin(t * 24 + k), 1.0, flare);
            drawBirdFly(cv, q.x, q.y - 8 * flare, 13 - 2 * flare, INK, flap, 0.2 * (slot.x > from.x ? 1 : -1) * (1 - flare), 1 - 0.2 * flare);
        } else if (t < burst) {
            // Perched: settle, hop on onsets, turn now and then, preen.
            const double settle = 1 - clamp01((t - b.land) / 0.25);
            double hop = 0, dip = 0;
            double face = b.face;
            if (c.score) {
                const Event* e = Score::last(c.score->onsets, t);
                if (e && e->t > b.land + 0.3 && hash2(k, std::floor(e->t * 50)) < 0.35) {
                    const double age = t - e->t;
                    if (age < 0.28) hop = 5 * std::sin(Pi * age / 0.28);
                }
            }
            const double turn = std::floor((t + hash2(k, 7) * 4) / 3.1);
            if (hash2(k, turn) < 0.3) face = -face;
            dip = std::max(0.0, std::sin(t * 0.8 + k * 1.9)) > 0.97 ? 0.7 : 0.0;
            dip += 0.4 * L.look;
            if (settle > 0) drawBirdFly(cv, slot.x, slot.y - 10 * settle, 11, INK, 1.0, 0, settle * 0.8, settle);
            drawBirdPerched(cv, slot.x, slot.y - hop, 10.5, INK, face, dip);
        } else {
            // Burst: scatter upward, then regroup into the flock heading right.
            const double age = t - burst;
            if(age>12) continue;
            const double ang = -1.9 + 0.55 * c.jit(150 + k);
            const double dist = (150 + 320 * hash2(k, 11)) * easeOut(std::min(1.0, age / 1.4));
            const V2 scatter = slot + V2(std::cos(ang) * dist * 1.4 + 120 * std::min(1.0, age), std::sin(ang) * dist);
            const double join = sstep(1.6, 4.0, age);
            const V2 formation = flockCentre(t, burst) + V2(V_FORM[k][0], V_FORM[k][1]);
            const V2 q = lerp(scatter, formation, join);
            const double flapRate = lerp(22, 9, join);
            const double size = lerp(14, 13 * V_FORM[k][2], join);
            drawBirdFly(cv, q.x, q.y, size, INK, 0.5 + 0.5 * std::sin(t * flapRate + k * 2.1), -0.15 + 0.1 * std::sin(k * 1.0));
        }
    }
    c.gpu.over(cv);
}

OsakaFlockDipV1 OsakaFlockDipV1::make(const Ctx& c, const Life& L, const std::vector<BirdPlan>& birds, double t) {
    // The weights are constant for this frame; sample only each spatial profile.
    std::vector<Dip> dips;
    for(const auto& b:birds) {
        const double age=t-b.land;
        if(age<0)continue;
        const double gone=c.schedule->fireworks>=b.land?clamp01((t-c.schedule->fireworks)/0.3):0;
        const double weight=1.4*(1-gone)+3.0*ring(age,2.2,3.5)*(1-gone);
        const double surge=L.surge.t>=0 && t>L.surge.t?2.5*ring(t-L.surge.t,2.6,2.5):0;
        dips.push_back({b.wire,b.u,weight,surge});
    }
    return {std::move(dips)};
}

}
