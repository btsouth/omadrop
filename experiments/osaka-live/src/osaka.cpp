// Osaka Jade, compiled world description. Array order is render order.
// Shared canvases, pass recipes and absolute actor targets belong to the
// named immutable v1 library profiles. No importer or drawing code here.
#include "kit/composition.h"
namespace Journey::Kit {
namespace {
using Op = OsakaOp;
using Gate = OsakaGate;
using Slot = OsakaRenderSlot;
constexpr Slot backdrop[] = {
    {Op::Sky, Gate::Always, "osaka-sky-v1"},
    {Op::AfterSky, Gate::Always, "after-sky"},
    {Op::Star, Gate::Chapter, "osaka-shooting-star-v1"},
    {Op::DiscHook, Gate::Always, "disc-port"},
    {Op::Disc, Gate::DiscEnabled, "osaka-disc-v1"},
    {Op::MountainHook, Gate::Always, "mountain-port"},
    {Op::Mountain, Gate::MountainEnabled, "osaka-mountain-v1"},
    {Op::BeforeCoast, Gate::Always, "before-coast"},
    {Op::CoastHook, Gate::Always, "coast-port"},
    {Op::Ridges, Gate::DefaultCoastLand, "osaka-ridges-v1"},
    {Op::City, Gate::DefaultCoastLand, "osaka-valley-city-v1"},
    {Op::Firework, Gate::DefaultCoastChapter, "osaka-firework-v1"},
    {Op::AfterValley, Gate::DefaultCoastLand, "after-valley"},
    {Op::NearRidge, Gate::DefaultCoastLand, "osaka-near-ridge-v1"},
    {Op::Train, Gate::DefaultCoastLand, "osaka-train-v1"},
    {Op::SkyLanterns, Gate::Always, "osaka-sky-lanterns-v1"},
};
constexpr Slot coast[] = {
    {Op::Ridges, Gate::Always, "osaka-ridges-v1"},
    {Op::City, Gate::Always, "osaka-valley-city-v1"},
    {Op::Firework, Gate::Chapter, "osaka-firework-v1"},
    {Op::AfterValley, Gate::Always, "after-valley"},
    {Op::NearRidge, Gate::Always, "osaka-near-ridge-v1"},
    {Op::Train, Gate::Always, "osaka-train-v1"},
};
constexpr Slot distantTown[] = {
    {Op::Downhill, Gate::Land, "osaka-downhill-rows-v1"},
    {Op::FarNetwork, Gate::Land, "osaka-network-group-v1"},
};
constexpr Slot foreground[] = {
    {Op::TownHook, Gate::Always, "town-port"},
    {Op::Downhill, Gate::DefaultTownLand, "osaka-downhill-rows-v1"},
    {Op::FarNetwork, Gate::DefaultTownLand, "osaka-network-group-v1"},
    {Op::AfterTown, Gate::Always, "after-town"},
    {Op::RightTown, Gate::Always, "osaka-right-town-v1"},
    {Op::StreetSurface, Gate::Always, "osaka-street-surface-group-v1"},
    {Op::Cart, Gate::Always, "osaka-cart-group-v1"},
    {Op::AfterCart, Gate::Always, "after-cart"},
    {Op::Festoon, Gate::Always, "osaka-festoon-v1"},
    {Op::NearNetwork, Gate::Always, "osaka-network-group-v1"},
    {Op::Moths, Gate::Always, "osaka-moths-v1"},
    {Op::AfterWires, Gate::Always, "after-wires"},
    // Capture includes town/cart/lanterns/wires, before foreground figures.
    {Op::ReflectionCapture, Gate::Always, "osaka-reflection-v1"},
    {Op::AfterReflections, Gate::Always, "after-reflections"},
    {Op::StreetActors, Gate::Always, "osaka-street-actors-v1"},
    {Op::Birds, Gate::Always, "osaka-flock-v1"},
    {Op::NearHouse, Gate::Always, "osaka-near-group-v1"},
    {Op::Wisteria, Gate::Always, "osaka-wisteria-v1"},
};
template<std::size_t N>
constexpr OsakaRenderStage stage(const Slot (&entries)[N], OsakaEventRef refs) {
    return {entries, N, refs};
}
const OsakaWorldDescription world {
    stage(backdrop, OsakaEventRef::Life),
    stage(coast, OsakaEventRef::Life),
    stage(distantTown, OsakaEventRef::LifeAndFlock),
    stage(foreground, OsakaEventRef::LifeAndFlock),
    OsakaFinishV1{}, // original bloom/vignette/grain/knee defaults
    OsakaDiscPlacementV1{1190, 286, 0.015, 108},
    OsakaMountainPlacementV1{1040, 0.04, 396, 632, 330},
};
}
const OsakaWorldDescription& osakaWorld() { return world; }
}
