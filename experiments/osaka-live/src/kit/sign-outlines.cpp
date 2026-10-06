// Fixed outlines from art.svg; no font lookup. Noto Sans CJK JP Bold, SIL OFL 1.1.
#include "../world.h"
#include "world-art.h"
namespace Journey {
void drawSignGlyph(Canvas& c,int index,double x,double y,double size,Col col,double alpha) {
    if(index<0||index>=7||alpha<=0.002)return;
    const auto binding="sign-glyph-"+std::to_string(index);
    Kit::drawWorldArt(c,binding.c_str(),x,y,size,&col,alpha);
}
}
