#pragma once
#include "events.h"
#include "figure.h"
namespace Journey::Kit {
struct OsakaWomanFanV1 {
    static constexpr const char* name = "osaka-womanfan-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& f, double t, double ox);
};
struct OsakaTeaV1 {
    static constexpr const char* name = "osaka-tea-v1";
    static void draw(Ctx& c, const OsakaEventState& L, Canvas& sh, double ox);
};
struct OsakaPatronsV1 {
    static constexpr const char* name = "osaka-patrons-v1";
    static void draw(Ctx& c, const OsakaEventState& L, Canvas& f, double t, double x0);
};
struct OsakaCookV1 {
    static constexpr const char* name = "osaka-cook-v1";
    static void draw(Ctx& c, const OsakaEventState& L, Canvas& p, double t, double yx);
};
struct OsakaCustomerV1 {
    static constexpr const char* name = "osaka-customer-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& p, double t, double yx);
};
}
