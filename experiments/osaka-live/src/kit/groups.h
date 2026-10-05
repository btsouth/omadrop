#pragma once
#include "events.h"
#include "flock.h"
namespace Journey::Kit {
struct OsakaNearRidgeV1 {
    static constexpr const char* name = "osaka-near-ridge-v1";
    static void draw(Ctx& c, const OsakaState& s);
};
struct OsakaTrainV1 {
    static constexpr const char* name = "osaka-train-v1";
    static void draw(Ctx& c, const OsakaState& s);
};
struct OsakaReflectionV1 {
    static constexpr const char* name = "osaka-reflection-v1";
    static void draw(Ctx& c, const OsakaState& s);
};
struct OsakaShootingStarV1 {
    static constexpr const char* name = "osaka-shooting-star-v1";
    static void draw(Ctx& c, const OsakaState& s);
};
struct OsakaFogV1 {
    static constexpr const char* name = "osaka-fog-v1";
    static void draw(Ctx& c, double amount, double top, double bottom, Col col);
};
struct OsakaGlowThroughV1 {
    static constexpr const char* name = "osaka-glow-through-v1";
    static void draw(Ctx& c, const OsakaState& s, double amount);
};
struct OsakaSteamV1 {
    static constexpr const char* name = "osaka-steam-v1";
    static void draw(Ctx& c, const OsakaEventState& L, double t, double yx);
};
}
