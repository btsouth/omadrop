#pragma once
#include "../parts.h"
#include "events.h"
#include "parameters.h"
#include "piece-label.h"
#include "ridges.h"
#include "gradient-sky.h"
#include "water-surface.h"
#include "swell-lines.h"
#include "foam-flecks.h"
#include "great-wave.h"
#include "boat-on-water.h"
#include "print-life.h"
#include "svg-art.h"
#include "window-label.h"
#include <map>
namespace Journey::Kit {
enum class OsakaPhase { Backdrop, Coast, DistantTown, Foreground };
enum class OsakaOp {
    Sky, AfterSky, Star, DiscHook, Disc, MountainHook, Mountain, BeforeCoast,
    CoastHook, Ridges, City, Firework, AfterValley, NearRidge, Train, SkyLanterns,
    TownHook, Downhill, FarNetwork, AfterTown, RightTown, StreetSurface, Cart,
    AfterCart, Festoon, NearNetwork, Moths, AfterWires, ReflectionCapture,
    AfterReflections, StreetActors, Birds, NearHouse, Wisteria, Haze, GradientSky, WaterSurface, SwellLines, FoamFlecks, GreatWave, BoatOnWater, SmokePlume, BirdFlock, PrintMoments
};
enum class OsakaGate { Always, Chapter, DiscEnabled, MountainEnabled, Land,
                       DefaultCoastLand, DefaultCoastChapter, DefaultTownLand };
enum class OsakaEventRef { Life, LifeAndFlock };
// A soft band of haze, from the slot's params in scene.json.
struct OsakaHazeSlotV1 {
    double y = 0, sigma = 1, lo = 0, hi = 0, shift = 0, drift = 0, seed = 0, gain = 0;
    Col color;
};
// Settings a slot carries in scene.json. Osaka's slots carry none.
struct OsakaSlotParamsV1 {
    OsakaHazeSlotV1 haze;
    GradientSkyParametersV1 gradientSky;
    WaterSurfaceParametersV1 water;
    SwellLinesParametersV1 swell;
    FoamFlecksParametersV1 foam;
    GreatWaveParametersV1 greatWave;
    BoatOnWaterParametersV1 boat;
    PrintLifeParametersV1 life;
    std::vector<OsakaRidgeSpecV1> ridges;
};
struct OsakaRenderSlot {
    OsakaOp piece;
    OsakaGate gate;
    const char* profile;
    std::string id;
    std::shared_ptr<const OsakaSlotParamsV1> params = nullptr;
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
    OsakaParametersV1 parameters;
    std::shared_ptr<const SvgArt> art;
    std::map<std::string,QString> artwork;
    std::vector<OsakaWindowNodeV1> windows;
    std::vector<OsakaPieceNodeV1> pieces;
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
