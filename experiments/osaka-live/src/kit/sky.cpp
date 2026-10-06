#include "parameters.h"
#include "sky.h"
#include "../osaka_shaders.h"

namespace Journey::Kit {
void OsakaSkyV1::draw(Ctx& c, const OsakaState& s) {
    const auto& p = osakaParameters().sky;
    GpuProfile::Group profileGroup(c.gpu.profile,"sky");
    std::string body = std::string(Shaders::cloud) + Shaders::osakaSky;
    Program& program = c.gpu.effect("osakaSky", body.c_str());
    const double energy = p.energyBase + p.energyBass * c.a.bass + p.energySurge * c.a.surge;
    c.gpu.pass(program, Blend::Replace, [&](Program& q) {
        q.set("u_cam", float(s.cam)); q.set("u_t", float(c.t + p.timeOffset)); q.set("u_energy", float(energy));
    }, -1, c.staticGeometry && s.land==1 ? QRectF(0,0,1920,640) : QRectF());
}
}
