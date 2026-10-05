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
}
