#include "network.h"
#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
std::vector<BirdPlan> birdPlan(const Ctx& c);

struct OsakaFlockV1 {
    static constexpr const char* name = "osaka-flock-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, const std::vector<BirdPlan>& plan);
};

inline void drawBirds(Ctx& c, const OsakaState& s, const OsakaEventState& L, const std::vector<BirdPlan>& plan) { OsakaFlockV1::draw(c, s, L, plan); }

struct OsakaFlockDipV1 {
    static constexpr const char* name = "osaka-flock-dip-v1";
    struct Dip {int wire;double u,weight,surge;};
    std::vector<Dip> dips;
    static OsakaFlockDipV1 make(const Ctx& c, const OsakaEventState& L, const std::vector<BirdPlan>& birds, double t);
    double at(int wire,double u) const {
        double dy=0;
        for(const auto& b:dips)if(b.wire==wire) {
            dy+=b.weight*std::exp(-std::pow((u-b.u)/0.05,2));
            if(b.surge!=0)dy-=b.surge*std::exp(-std::pow((u-b.u)/0.07,2));
        }
        return dy;
    }

};

}
