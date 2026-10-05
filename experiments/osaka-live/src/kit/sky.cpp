#include "sky.h"
#include "../osaka_shaders.h"

namespace Journey::Kit {
void OsakaSkyV1::draw(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"sky");
    std::string body = std::string(Shaders::cloud) + Shaders::osakaSky;
    Program& p = c.gpu.effect("osakaSky", body.c_str());
    const double energy = 0.45 + 0.35 * c.a.bass + 0.25 * c.a.surge;
    c.gpu.pass(p, Blend::Replace, [&](Program& q) {
        q.set("u_cam", float(s.cam)); q.set("u_t", float(c.t + 10)); q.set("u_energy", float(energy));
    }, -1, c.staticGeometry && s.land==1 ? QRectF(0,0,1920,640) : QRectF());
}
}
