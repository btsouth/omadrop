#include "world.h"
namespace Journey {
void World::render(int w,int h,double t,const Audio& audio,const Score& score,const Schedule& schedule) {
    gpu_.begin(w,h);
    Ctx c{gpu_,t,audio,&score,seed_,&canvases_,&schedule};
    drawOsaka(c,{});
}
}
