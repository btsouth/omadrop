#pragma once
#include "../parts.h"
#include "events.h"
namespace Journey::Kit {
enum class OsakaPhase { Backdrop, Coast, DistantTown, Foreground };
enum class OsakaOp {
    Sky, AfterSky, Star, DiscHook, Disc, MountainHook, Mountain, BeforeCoast,
    CoastHook, Ridges, City, Firework, AfterValley, NearRidge, Train, SkyLanterns,
    TownHook, Downhill, FarNetwork, AfterTown, RightTown, StreetSurface, Cart,
    AfterCart, Festoon, NearNetwork, Moths, AfterWires, ReflectionCapture,
    AfterReflections, StreetActors, Birds, NearHouse, Wisteria
};
enum class OsakaGate { Always, Chapter, DiscEnabled, MountainEnabled, Land,
                       DefaultCoastLand, DefaultCoastChapter, DefaultTownLand };
enum class OsakaEventRef { Life, LifeAndFlock };
struct OsakaRenderSlot {
    OsakaOp piece;
    OsakaGate gate;
    const char* profile;
    std::string id;
};
struct OsakaRenderStage {
    const OsakaRenderSlot* entries;
    std::size_t count;
    OsakaEventRef events;
    std::string id;
};
struct OsakaFinishV1 {
    static constexpr const char* name = "osaka-finish-v1";
    FinishParams defaults{};
};
struct OsakaDiscPlacementV1 {
    double x = 1190, y = 286, parallax = 0.015, radius = 108;
};
struct OsakaMountainPlacementV1 {
    double x = 1040, parallax = 0.04, peak = 396, base = 632, width = 330;
};
struct OsakaWorldDescription {
    static constexpr const char* name = "osaka-world-v1";
    OsakaRenderStage backdrop, coast, distantTown, foreground;
    OsakaFinishV1 finish;
    OsakaDiscPlacementV1 disc;
    OsakaMountainPlacementV1 mountain;
};
// World supplies ordered typed instances, never drawing callbacks.
const OsakaWorldDescription& osakaWorld();
struct OsakaCompositionV1 {
    static constexpr const char* name = "osaka-composition-v1";
    static void render(Ctx&, const OsakaState&, const OsakaWorldDescription&, OsakaPhase,
                       bool disc = true, bool mountain = true,
                       const BackdropHooks* = nullptr, const OsakaHooks* = nullptr);
};
}
