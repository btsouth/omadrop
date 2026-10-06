#pragma once
#include "../world.h"
namespace Journey::Kit {
struct OsakaSkyV1 {
    static constexpr const char* name = "osaka-sky-v1";
    static void draw(Ctx& c, const OsakaState& s);
};
}
