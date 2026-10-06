#pragma once
#include "composition.h"
#include <stdexcept>
namespace Journey::Kit {
inline void drawWorldArt(Canvas& canvas,const char* binding,double x=0,double y=0,double size=1,const Col* tint=nullptr,double alpha=1) {
    const auto& w=osakaWorld();
    const auto it=w.artwork.find(binding);
    if(it==w.artwork.end())throw std::runtime_error(std::string("missing world art binding: ")+binding);
    canvas.save();canvas.translate(x,y);canvas.scale(size,size);
    if(!w.art->replay(canvas,it->second,tint,alpha))throw std::runtime_error("world art cannot replay");
    canvas.restore();
}
}
