#include "parameters.h"
#include "composition.h"
#include "groups.h"
#include "sky.h"
#include "ridges.h"
#include "city.h"
#include "downhill.h"
#include "firework.h"
#include "sky-lanterns.h"
#include "festoon.h"
#include "animals.h"
#include "wisteria.h"
#include "flock.h"
#include "network.h"
#include "lanterns.h"
#include "actors.h"
#include "palette.h"
#include "haze.h"
#include "generic-window.h"
#include "label-pieces.h"
namespace Journey::Kit {
namespace {
void moon(Ctx& c, const OsakaState& s, const OsakaDiscPlacementV1& placement) {
    const auto& p = osakaParameters().disc;
    DiscLook d;
    d.pos = {placement.x - s.cam * placement.parallax + s.moonDx, placement.y + s.moonDy};
    d.r = placement.radius;
    const double rise = 1;
    d.col = d.col2 = mix(hex(p.creamHex), hex(p.warmHex), s.moonWarm) * float(p.colorGain * rise);
    d.halo = Col(float(p.haloR), float(p.haloG), float(p.haloB)) * float(rise);
    if (p.color2Hex >= 0) d.col2 = hex(p.color2Hex) * float(p.colorGain * rise);
    d.ring = hex(p.ringHex >= 0 ? p.ringHex : p.creamHex);
    d.energy = (p.energyBase + p.energyBass * c.a.bass + p.energySurge * c.a.surge + p.energyKick * c.kick(6)) * rise;
    if (p.energyLift != 0) d.energy += p.energyLift * c.lift(0) * rise;
    d.veil=p.veil; d.tex=p.texture; d.haloA=p.haloA; d.haloB=p.haloBRadius; d.haloC=p.haloC; d.haloD=p.haloD; d.haloFar=p.haloFar; d.restRings=p.restRings;
    drawDisc(c, d, s.cam);
}
void mountainLook(Ctx& c, const OsakaState& s, const OsakaMountainPlacementV1& placement) {
    const auto& p = osakaParameters().mountain;
    MountainLook m;
    m.px = placement.x - s.cam * placement.parallax; m.peak = placement.peak; m.base = placement.base; m.width = placement.width;
    m.top = Col(float(p.topR), float(p.topG), float(p.topB)); m.bot = Col(float(p.bottomR), float(p.bottomG), float(p.bottomB));
    m.foot=p.foot; m.snow=p.snow; m.snowScale=p.snowScale;
    m.snowCol=Col(float(p.snowR),float(p.snowG),float(p.snowB));
    drawMountain(c, m);
}
}
void OsakaCompositionV1::render(Ctx& c, const OsakaState& s, const OsakaWorldDescription& world,
                              OsakaPhase phase, bool disc, bool mountain,
                              const BackdropHooks* b, const OsakaHooks* h) {
    const char* group = phase == OsakaPhase::Backdrop ? "drawOsakaBackdrop" :
        phase == OsakaPhase::Coast ? "drawOsakaCoast" :
        phase == OsakaPhase::DistantTown ? "drawOsakaDistantTown" : "drawOsakaForeground";
    GpuProfile::Group profileGroup(c.gpu.profile, group);
    if (phase == OsakaPhase::Coast && !(s.land > 0.01)) return;
    const auto& stage = phase == OsakaPhase::Backdrop ? world.backdrop :
        phase == OsakaPhase::Coast ? world.coast :
        phase == OsakaPhase::DistantTown ? world.distantTown : world.foreground;
    const auto L = OsakaEventsV1::at(c);
    const auto plan = stage.events == OsakaEventRef::LifeAndFlock ? birdPlan(c) : std::vector<BirdPlan>{};
    const bool drawWindows = phase == OsakaPhase::Foreground && (!world.windows.empty() || !world.pieces.empty());
    if (drawWindows) {
        Canvas& art = c.canvas();
        world.art->draw(art);
        c.gpu.draw(art);
    }
    bool defaultCoastLand = false, defaultTown = false;
    auto enabled = [&](OsakaGate gate) {
        switch (gate) {
        case OsakaGate::Always: return true;
        case OsakaGate::Chapter: return s.chapter;
        case OsakaGate::DiscEnabled: return disc;
        case OsakaGate::MountainEnabled: return mountain;
        case OsakaGate::Land: return s.land > 0.01;
        case OsakaGate::DefaultCoastLand: return defaultCoastLand;
        case OsakaGate::DefaultCoastChapter: return defaultCoastLand && s.chapter;
        case OsakaGate::DefaultTownLand: return defaultTown && s.land > 0.01;
        }
        return false;
    };
    for (std::size_t i = 0; i < stage.count; ++i) {
        const auto& slot = stage.entries[i];
        if (!enabled(slot.gate)) continue;
        switch (slot.piece) {
        case OsakaOp::Sky: OsakaSkyV1::draw(c, s); break;
        case OsakaOp::WaterSurface: WaterSurfaceV1::draw(c, slot.params->water); break;
        case OsakaOp::SwellLines: SwellLinesV1::draw(c, slot.params->swell); break;
        case OsakaOp::BoatOnWater: BoatOnWaterV1::draw(c,slot.params->boat); break;
        case OsakaOp::GreatWave: GreatWaveV1::draw(c,slot.params->greatWave); break;
        case OsakaOp::FoamFlecks: FoamFlecksV1::draw(c, slot.params->foam); break;
        case OsakaOp::GradientSky: GradientSkyV1::draw(c, slot.params->gradientSky); break;
        case OsakaOp::AfterSky: if (b && b->afterSky) b->afterSky(); break;
        case OsakaOp::Star: OsakaShootingStarV1::draw(c, s); break;
        case OsakaOp::DiscHook: if (b && b->disc) b->disc(); break;
        case OsakaOp::Disc: moon(c, s, world.disc); break;
        case OsakaOp::MountainHook: if (b && b->mountain) b->mountain(); break;
        case OsakaOp::Mountain: mountainLook(c, s, world.mountain); break;
        case OsakaOp::BeforeCoast: if (b && b->beforeCoast) b->beforeCoast(); break;
        case OsakaOp::CoastHook:
            defaultCoastLand = !(b && b->coast) && s.land > 0.01;
            if (b && b->coast) b->coast();
            break;
        case OsakaOp::Ridges:
            if (slot.params && !slot.params->ridges.empty()) OsakaRidgesV1::draw(c, s, slot.params->ridges, slot.id);
            else OsakaRidgesV1::draw(c, s);
            break;
        case OsakaOp::Haze: {
            const auto& h = slot.params->haze;
            hazeBand(c, h.y, h.sigma, h.lo, h.hi, h.shift + c.t * h.drift, h.seed, h.color, h.gain);
            break;
        }
        case OsakaOp::City: valleyCity(c, s); break;
        case OsakaOp::Firework: firework(c, s, L); break;
        case OsakaOp::AfterValley: if (b && b->afterValley) b->afterValley(); break;
        case OsakaOp::NearRidge: OsakaNearRidgeV1::draw(c, s); break;
        case OsakaOp::Train: OsakaTrainV1::draw(c, s); break;
        case OsakaOp::SkyLanterns: skyLanterns(c, s, L); break;
        case OsakaOp::TownHook:
            defaultTown = !(h && h->town);
            if (h && h->town) h->town();
            break;
        case OsakaOp::Downhill: downhillRoofs(c, s, L); break;
        case OsakaOp::FarNetwork: OsakaNetworkGroupV1::draw(c, s, L, plan, true); break;
        case OsakaOp::AfterTown: if (h && h->afterTown) h->afterTown(); break;
        case OsakaOp::RightTown: OsakaRightTownV1::draw(c, s, L); break;
        case OsakaOp::StreetSurface: OsakaStreetSurfaceV1::draw(c, s); break;
        case OsakaOp::Cart: OsakaCartGroupV1::draw(c, s, L); break;
        case OsakaOp::AfterCart: if (h && h->afterYatai) h->afterYatai(); break;
        case OsakaOp::Festoon: OsakaFestoonV1::draw(c, s, L, V2(POLES[0].x - 16 - s.cam * POLES[0].par, 640)); break;
        case OsakaOp::NearNetwork: OsakaNetworkGroupV1::draw(c, s, L, plan, false); break;
        case OsakaOp::Moths: OsakaMothsV1::draw(c, s, POLES[0].x - s.cam * POLES[0].par - 83); break;
        case OsakaOp::AfterWires: if (h && h->afterWires) h->afterWires(); break;
        case OsakaOp::ReflectionCapture: OsakaReflectionV1::draw(c, s); break;
        case OsakaOp::AfterReflections: if (h && h->afterReflections) h->afterReflections(); break;
        case OsakaOp::StreetActors: OsakaStreetActorsV1::draw(c, s, L); break;
        case OsakaOp::Birds: drawBirds(c, s, L, plan); break;
        case OsakaOp::NearHouse: OsakaNearGroupV1::draw(c, s, L); break;
        case OsakaOp::Wisteria: wisteria(c, s, L); break;
        }
    }
    if (drawWindows) {
        for (const auto& window : world.windows) GenericWindowV1::draw(c, window);
        LabelPiecesV1::draw(c, L, world);
    }
}
}
namespace Journey {
void drawOsakaBackdrop(Ctx& c, const OsakaState& s, bool disc, bool mountain, const BackdropHooks* hooks) {
    Kit::OsakaCompositionV1::render(c,s,Kit::osakaWorld(),Kit::OsakaPhase::Backdrop,disc,mountain,hooks);
}
void drawOsakaCoast(Ctx& c, const OsakaState& s, const BackdropHooks* hooks) {
    Kit::OsakaCompositionV1::render(c,s,Kit::osakaWorld(),Kit::OsakaPhase::Coast,true,true,hooks);
}
void drawOsakaDistantTown(Ctx& c, const OsakaState& s) {
    Kit::OsakaCompositionV1::render(c,s,Kit::osakaWorld(),Kit::OsakaPhase::DistantTown);
}
void drawOsakaForeground(Ctx& c, const OsakaState& s, const OsakaHooks& hooks) {
    Kit::OsakaCompositionV1::render(c,s,Kit::osakaWorld(),Kit::OsakaPhase::Foreground,true,true,nullptr,&hooks);
}
void osakaFog(Ctx& c, double amount, double top, double bottom, Col col) { Kit::OsakaFogV1::draw(c,amount,top,bottom,col); }
void osakaGlowThrough(Ctx& c, const OsakaState& s, double amount) { Kit::OsakaGlowThroughV1::draw(c,s,amount); }
std::array<std::array<V2,4>,6> osakaOutRuns(double cam) { return Kit::OsakaWireNetworkV1::outRuns(cam); }
double outSag(int seg, int i) { return Kit::OsakaWireNetworkV1::sag(seg,i); }
double bearerX(double t) { return Kit::OsakaBearerV1::x(t); }
void drawBearerLantern(Canvas& body, Canvas& light, V2 hand, double swing, double size, double bright) {
    Kit::lantern(body,light,hand,swing,size,bright);
}
void drawOsaka(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsaka");
    drawOsakaBackdrop(c,s,true,true);
    drawOsakaForeground(c,s,{});
    auto f = Kit::osakaWorld().finish.defaults;
    f.time = float(c.t);
    c.gpu.finish(f,nullptr);
}
}
