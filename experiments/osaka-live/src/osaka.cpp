// Osaka Jade: "Evening on the hill". A held stage that wakes up and comes
// alive: panes, a noodle cart, a lantern-bearer, shadow play behind paper,
// birds landing on the six wires, and a surge that bursts the flock and
// blooms one firework. Geometry and palette follow the approved mock.
#include "osaka_shaders.h"
#include "parts.h"
#include "rig.h"
#include "kit/actors.h"
#include "kit/groups.h"
#include "kit/neon.h"
#include "kit/firework.h"
#include "kit/light-wave.h"
#include "kit/events.h"
#include "kit/flock.h"
#include "kit/pulses.h"
#include "kit/strands.h"
#include "kit/poles.h"
#include "kit/network.h"
#include "kit/wisteria.h"
#include "kit/grass.h"
#include "kit/downhill.h"
#include "kit/city.h"
#include "kit/chime.h"
#include "kit/animals.h"
#include "kit/sky-lanterns.h"
#include "kit/festoon.h"
#include "kit/cloth.h"
#include "kit/onset.h"
#include "kit/lanterns.h"
#include "kit/rooms.h"
#include "kit/pane.h"
#include "kit/town.h"
#include "kit/layout.h"
#include "kit/ridges.h"
#include "kit/sky.h"
#include "kit/haze.h"
#include "kit/mountain.h"
#include "kit/disc.h"
#include "kit/primitives.h"

#include <cmath>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>

namespace Journey {
namespace {
using namespace Kit;
constexpr double QUAY = 2330.0;


// ---------- timeline ----------
// One firework shell. The main pair blooms on the surge; the finale shells
// burst on the bass hits that follow it, so the sky answers the drop.
using Kit::Shell;

using Life = Kit::OsakaEventState;








// Panes switch on at authored times, then breathe with their band and answer
// the firework with a flash that travels outward from it.





// ---------- backdrop ----------

}  // namespace



namespace {
void band(Ctx& c, double y0, double sigma, double lo, double hi, double shift, double seed, Col col, double gain,
          V2 noise = {480, 108}) {
    hazeBand(c, y0, sigma, lo, hi, shift, seed, col, gain, noise);
}


}  // namespace

// The one mountain: a broad cone in Osaka Jade that recedes to Fuji at sea.

namespace {
// The city below the hill: hazy blocks with lit windows that come and go,
// streets of lamps, a few cars, and warning lights on the towers. Its
// windows sparkle with the treble.




// A lit train crosses the valley below the ridge; sparks on bass hits.


// After the firework, sky lanterns rise from the valley.


// Star position for a shell: drag-limited outward travel plus gravity, so
// stars race out, slow, then sag (the willow sags most).




// ---------- town ----------






// ---------- people of the street ----------
// Walk schedules: distance travelled as a function of time, with eased
// starts and stops so feet plant and bodies settle.
// A paper chochin on a short pole: ribbed, capped, lit warm from within.




// A little reflected lantern light on the street-facing cloth. Keep the
// silhouette, but let a coat front / obi separate at small viewing sizes.




// ---------- the noodle cart, the couple, the cat on the rail ----------


// ---------- the six wires ----------

}  // namespace

std::array<std::array<V2, 4>, 6> osakaOutRuns(double cam);
std::array<std::array<V2, 4>, 6> osakaOutRuns(double cam) {
    return OsakaWireNetworkV1::outRuns(cam);
}

double outSag(int seg, int i) { return OsakaWireNetworkV1::sag(seg, i); }

namespace {


// Birds: each lands on its slot on an onset (timed fallback), the wire dips,
// it settles, hops on later onsets, and bursts away on the surge.

// Two passes: `far` draws the valley poles and the spans running down to them,
// behind the street and under the fog; the near pass draws everything else.
void polesWires(Ctx& c, const OsakaState& s, const Life& L, const std::vector<BirdPlan>& birds, bool far) {
    GpuProfile::Group profileGroup(c.gpu.profile,"polesWires");
    const double t = c.t, cam = s.cam;
    Canvas& cv = c.canvas();
    Canvas& l = c.canvas();
    Canvas& cone = c.canvas();
    l.preserveRaster=cone.preserveRaster=true;
    Spans spans = wireRuns(cam);
    const auto outs = osakaOutRuns(cam);
    const double land = clamp01(s.land);
    Canvas* staticPoles = c.retainedBuilder(cv, far ? "far-poles" : "near-poles", {cam, land});
    if(staticPoles)staticPoles->preserveRaster=true;
    OsakaPolesV1::draw(c, s, L, cv, l, cone, staticPoles, t, cam, land, far);
    const auto dips = OsakaFlockDipV1::make(c, L, birds, t);
    auto dipAt = [&](int wire, double u) { return dips.at(wire, u); };
    static const auto tailFade=[] {std::array<double,16> values{};for(int j=0;j<16;++j)values[j]=std::pow(1-j/16.0,1.6);return values;}();
    OsakaStrandsV1::draw(c, s, L, cv, l, spans, outs, dipAt, t, land, far);
    OsakaPulseStreamV1::draw(c, s, l, spans, outs, dipAt, tailFade, land, far);
    c.gpu.add(cone, 1.0f, 7);
    c.gpu.over(cv);
    c.gpu.over(l, 1.9f);
    c.gpu.add(l, 0.5f, 12);
}

// Festival lanterns strung from the near eave to the street pole. They come
// on one by one at 3 s; each onset then sends a wave of light along the
// string (alternating direction), bass hits lift them all, and every
// firework shell makes them flare as its light arrives.


// Moths circling the street lamp; they get busier with the treble.




}  // namespace

void drawBearerLantern(Canvas& body, Canvas& light, V2 hand, double swing, double size, double bright) {
    lantern(body, light, hand, swing, size, bright);
}


namespace {
// ---------- the near house ----------





}  // namespace

double bearerX(double t) { return OsakaBearerV1::x(t); }

// A shooting star crosses the upper sky on the first strong onset after
// 15.5 s (timed fallback at 16.8 s): one small thing to catch on a rewatch.


void drawOsakaBackdrop(Ctx& c, const OsakaState& s, bool disc, bool mountain, const BackdropHooks* hooks) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaBackdrop");
    const Life L = OsakaEventsV1::at(c);
    Kit::OsakaSkyV1::draw(c, s);
    if (hooks && hooks->afterSky) hooks->afterSky();
    if (s.chapter) OsakaShootingStarV1::draw(c, s);
    if (hooks && hooks->disc) hooks->disc();
    if (disc) {
        DiscLook d;
        d.pos = {1190 - s.cam * 0.015 + s.moonDx, 286 + s.moonDy};
        d.r = 108;
        // Opening: the moon brightens out of the haze over the first seconds.
        const double rise = 1;
        d.col = d.col2 = mix(CREAM, hex(0xe2703a), s.moonWarm) * float(1.22 * rise);
        d.halo = Col(0.55f, 0.95f, 0.74f) * float(rise);
        d.ring = CREAM;
        // The halo swells on each bass hit as well as with the bass body.
        d.energy = (0.35 + 0.6 * c.a.bass + 0.2 * c.a.surge + 0.5 * c.kick(6)) * rise;
        drawDisc(c, d, s.cam);
    }
    if (hooks && hooks->mountain) hooks->mountain();
    if (mountain) {
        MountainLook m;
        m.px = 1040 - s.cam * 0.04; m.peak = 396; m.base = 632; m.width = 330;
        m.top = Col(0.050f, 0.300f, 0.220f); m.bot = Col(0.30f, 0.84f, 0.60f);
        drawMountain(c, m);
    }
    if (hooks && hooks->beforeCoast) hooks->beforeCoast();
    if (hooks && hooks->coast) hooks->coast();
    else if (s.land > 0.01) {
        Kit::OsakaRidgesV1::draw(c, s);
        valleyCity(c, s);
        if (s.chapter) firework(c, s, L);
        if (hooks && hooks->afterValley) hooks->afterValley();
        OsakaNearRidgeV1::draw(c, s);
        OsakaTrainV1::draw(c, s);
    }
    skyLanterns(c, s, L);
}

void drawOsakaCoast(Ctx& c, const OsakaState& s, const BackdropHooks* hooks) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaCoast");
    if (s.land > 0.01) {
        const Life L = OsakaEventsV1::at(c);
        Kit::OsakaRidgesV1::draw(c, s);
        valleyCity(c, s);
        if (s.chapter) firework(c, s, L);
        if (hooks && hooks->afterValley) hooks->afterValley();
        OsakaNearRidgeV1::draw(c, s);
        OsakaTrainV1::draw(c, s);
    }
}

void drawOsakaDistantTown(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaDistantTown");
    const Life L = OsakaEventsV1::at(c);
    const auto plan = birdPlan(c);
    if (s.land > 0.01) downhillRoofs(c, s, L);
    if (s.land > 0.01) polesWires(c, s, L, plan, true);
}

void drawOsakaForeground(Ctx& c, const OsakaState& s, const OsakaHooks& hooks) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaForeground");
    const Life L = OsakaEventsV1::at(c);
    const auto plan = birdPlan(c);
    if (hooks.town) hooks.town();
    else {
        if (s.land > 0.01) downhillRoofs(c, s, L);
        if (s.land > 0.01) polesWires(c, s, L, plan, true);
    }
    if (hooks.afterTown) hooks.afterTown();
    OsakaRightTownV1::draw(c, s, L);
    OsakaStreetSurfaceV1::draw(c, s);
    OsakaCartGroupV1::draw(c, s, L);
    if (hooks.afterYatai) hooks.afterYatai();
    OsakaFestoonV1::draw(c, s, L, V2(POLES[0].x - 16 - s.cam * POLES[0].par, 640));
    polesWires(c, s, L, plan, false);
    OsakaMothsV1::draw(c, s, POLES[0].x - s.cam * POLES[0].par - 83);
    if (hooks.afterWires) hooks.afterWires();
    OsakaReflectionV1::draw(c, s);
    if (hooks.afterReflections) hooks.afterReflections();
    OsakaStreetActorsV1::draw(c, s, L);
    drawBirds(c, s, L, plan);
    OsakaNearGroupV1::draw(c, s, L);
    wisteria(c, s, L);
}





void osakaFog(Ctx& c, double amount, double top, double bottom, Col col) { OsakaFogV1::draw(c, amount, top, bottom, col); }

void osakaGlowThrough(Ctx& c, const OsakaState& s, double amount) { OsakaGlowThroughV1::draw(c, s, amount); }

void drawOsaka(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsaka");
    drawOsakaBackdrop(c, s, true, true);
    drawOsakaForeground(c, s, {});
    FinishParams f;
    f.time = float(c.t);
    c.gpu.finish(f,nullptr);

}
}
