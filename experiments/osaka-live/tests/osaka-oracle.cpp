// Osaka Jade, compiled world description. Array order is render order.
// Shared canvases, pass recipes and absolute actor targets belong to the
// named immutable v1 library profiles. No importer or drawing code here.
#include "../src/kit/composition.h"
namespace Journey::Kit {
namespace {
using Op = OsakaOp;
using Gate = OsakaGate;
using Slot = OsakaRenderSlot;
const Slot backdrop[] = {
    {Op::Sky, Gate::Always, "osaka-sky-v1", "backdrop-Sky"},
    {Op::AfterSky, Gate::Always, "after-sky", "backdrop-AfterSky"},
    {Op::Star, Gate::Chapter, "osaka-shooting-star-v1", "backdrop-Star"},
    {Op::DiscHook, Gate::Always, "disc-port", "backdrop-DiscHook"},
    {Op::Disc, Gate::DiscEnabled, "osaka-disc-v1", "backdrop-Disc"},
    {Op::MountainHook, Gate::Always, "mountain-port", "backdrop-MountainHook"},
    {Op::Mountain, Gate::MountainEnabled, "osaka-mountain-v1", "backdrop-Mountain"},
    {Op::BeforeCoast, Gate::Always, "before-coast", "backdrop-BeforeCoast"},
    {Op::CoastHook, Gate::Always, "coast-port", "backdrop-CoastHook"},
    {Op::Ridges, Gate::DefaultCoastLand, "osaka-ridges-v1", "backdrop-Ridges"},
    {Op::City, Gate::DefaultCoastLand, "osaka-valley-city-v1", "backdrop-City"},
    {Op::Firework, Gate::DefaultCoastChapter, "osaka-firework-v1", "backdrop-Firework"},
    {Op::AfterValley, Gate::DefaultCoastLand, "after-valley", "backdrop-AfterValley"},
    {Op::NearRidge, Gate::DefaultCoastLand, "osaka-near-ridge-v1", "backdrop-NearRidge"},
    {Op::Train, Gate::DefaultCoastLand, "osaka-train-v1", "backdrop-Train"},
    {Op::SkyLanterns, Gate::Always, "osaka-sky-lanterns-v1", "backdrop-SkyLanterns"},
};
const Slot coast[] = {
    {Op::Ridges, Gate::Always, "osaka-ridges-v1", "coast-Ridges"},
    {Op::City, Gate::Always, "osaka-valley-city-v1", "coast-City"},
    {Op::Firework, Gate::Chapter, "osaka-firework-v1", "coast-Firework"},
    {Op::AfterValley, Gate::Always, "after-valley", "coast-AfterValley"},
    {Op::NearRidge, Gate::Always, "osaka-near-ridge-v1", "coast-NearRidge"},
    {Op::Train, Gate::Always, "osaka-train-v1", "coast-Train"},
};
const Slot distantTown[] = {
    {Op::Downhill, Gate::Land, "osaka-downhill-rows-v1", "distantTown-Downhill"},
    {Op::FarNetwork, Gate::Land, "osaka-network-group-v1", "distantTown-FarNetwork"},
};
const Slot foreground[] = {
    {Op::TownHook, Gate::Always, "town-port", "foreground-TownHook"},
    {Op::Downhill, Gate::DefaultTownLand, "osaka-downhill-rows-v1", "foreground-Downhill"},
    {Op::FarNetwork, Gate::DefaultTownLand, "osaka-network-group-v1", "foreground-FarNetwork"},
    {Op::AfterTown, Gate::Always, "after-town", "foreground-AfterTown"},
    {Op::RightTown, Gate::Always, "osaka-right-town-v1", "foreground-RightTown"},
    {Op::StreetSurface, Gate::Always, "osaka-street-surface-group-v1", "foreground-StreetSurface"},
    {Op::Cart, Gate::Always, "osaka-cart-group-v1", "foreground-Cart"},
    {Op::AfterCart, Gate::Always, "after-cart", "foreground-AfterCart"},
    {Op::Festoon, Gate::Always, "osaka-festoon-v1", "foreground-Festoon"},
    {Op::NearNetwork, Gate::Always, "osaka-network-group-v1", "foreground-NearNetwork"},
    {Op::Moths, Gate::Always, "osaka-moths-v1", "foreground-Moths"},
    {Op::AfterWires, Gate::Always, "after-wires", "foreground-AfterWires"},
    // Capture includes town/cart/lanterns/wires, before foreground figures.
    {Op::ReflectionCapture, Gate::Always, "osaka-reflection-v1", "foreground-ReflectionCapture"},
    {Op::AfterReflections, Gate::Always, "after-reflections", "foreground-AfterReflections"},
    {Op::StreetActors, Gate::Always, "osaka-street-actors-v1", "foreground-StreetActors"},
    {Op::Birds, Gate::Always, "osaka-flock-v1", "foreground-Birds"},
    {Op::NearHouse, Gate::Always, "osaka-near-group-v1", "foreground-NearHouse"},
    {Op::Wisteria, Gate::Always, "osaka-wisteria-v1", "foreground-Wisteria"},
};
template<std::size_t N>
OsakaRenderStage stage(const Slot (&entries)[N], const char* id, OsakaEventRef refs) {
    return {entries, N, refs, id};
}
const OsakaWorldDescription world {
    stage(backdrop, "backdrop", OsakaEventRef::Life),
    stage(coast, "coast", OsakaEventRef::Life),
    stage(distantTown, "distantTown", OsakaEventRef::LifeAndFlock),
    stage(foreground, "foreground", OsakaEventRef::LifeAndFlock),
    OsakaFinishV1{}, // original bloom/vignette/grain/knee defaults
    OsakaDiscPlacementV1{1190, 286, 0.015, 108},
    OsakaMountainPlacementV1{1040, 0.04, 396, 632, 330},
};
}
const OsakaWorldDescription& osakaOracle() { return world; }
}
