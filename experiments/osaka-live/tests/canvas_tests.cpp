#include "canvas.h"
#include <cstdint>
#include <iostream>
#include <cstring>

std::uint64_t digest(bool optimized) {
    Journey::Canvas::useKnownConvex=optimized;
    std::uint64_t hash=14695981039346656037ull;
    auto bytes=[&](const void* p,std::size_t n) {
        const auto* b=static_cast<const unsigned char*>(p);
        while(n--) { hash^=*b++; hash*=1099511628211ull; }
    };
    // Original 4c2d413 canvas establishes this digest. Exercise the art's
    // ellipses, transforms, tiny discs and compound/concave paths. The
    // optimization must preserve vertices and draw commands exactly.
    for(int scale=1;scale<=3;++scale) for(int k=0;k<100;++k) {
        Journey::Canvas c(scale/2.0);
        c.translate(30.1,24.7); c.rotate(k*0.13); c.scale(k%2?-1.2:1.2,0.87);
        c.color({0.3f,0.8f,0.5f},0.6);
        c.ellipse(k*17.3,k*3.1,0.1+k*0.7,0.15+k*0.3,k*0.1); c.fill();
        c.rect(k*3.7,k*11.1,6.4,7.2); c.fill();
        c.moveTo(0,0); c.lineTo(20,0); c.lineTo(10,4); c.lineTo(20,20); c.lineTo(0,20); c.closePath(); c.fill();
        for(const auto& v:c.vertices()) bytes(&v,sizeof v);
        for(const auto& cmd:c.commands()) {
            const int fields[]={int(cmd.kind),cmd.first,cmd.count,cmd.coverFirst,cmd.coverCount};
            bytes(fields,sizeof fields);
        }
    }
    return hash;
}

int main() {
    const auto reference=digest(false), optimized=digest(true);
    std::cout<<std::hex<<reference<<" "<<optimized<<'\n';
    return reference==0xf5cd4a8a18f3da98ull && optimized==reference ? 0 : 1;
}
