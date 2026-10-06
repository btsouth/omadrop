#include "haze.h"
#include "../osaka_shaders.h"

namespace Journey {
void Kit::OsakaHazeV1::draw(Ctx& c, double y0, double sigma, double lo, double hi, double shift, double seed, Col col, double gain,
              V2 noise) {
    Program& p = c.gpu.effect("band", Shaders::band);
    c.gpu.pass(p, Blend::Add, [&](Program& q) {
        q.set("u_y0", float(y0)); q.set("u_sigma", float(sigma)); q.set("u_lo", float(lo)); q.set("u_hi", float(hi));
        q.set("u_shift", float(shift)); q.set("u_seed", float(seed)); q.set("u_gain", float(gain));
        q.set("u_col", col); q.set("u_noise", float(noise.x), float(noise.y));
    }, -1, c.staticGeometry ? QRectF(0,y0-7*sigma,1920,14*sigma) : QRectF());
}
}
