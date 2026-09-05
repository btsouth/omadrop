#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"
#include "musical-field.glsl"

// Luminous Mosaic. Full-frame recursive stained-light lattice, inspired by
// MilkDrop's kaleidoscopic layering. No camera travel or rotating clock phase.
void main() {
    vec2 p=(uv-0.5)*vec2(resolution.x/resolution.y,1.0)*2.8;
    float low=musicalBand(0),mid=musicalBand(1),high=musicalBand(2);
    float detail=musicalDetail(clamp(abs(p.x)*0.4+abs(p.y)*0.2,0.0,1.0));
    float radius=length(p);
    float angle=atan(p.y,p.x);
    // Continuous harmonic geometry, without hard sector folding. The musical
    // balance changes the symmetry itself: broad fourfold arches, sixfold
    // petals and finer eightfold tracery can coexist and exchange prominence.
    // All harmonics are periodic at the angle seam and vanish at the origin.
    float bloom=motionScale*low;
    float branching=motionScale*mid;
    float fineStructure=motionScale*high;
    float six=0.10+0.62*branching;
    float four=0.08+0.60*bloom;
    float eight=max(0.10,1.0-six-four);
    float weights=six+four+eight;
    float lobes=(eight*cos(8.0*angle)+six*cos(6.0*angle)
        +four*cos(4.0*angle))/weights;
    float petals=(eight*sin(8.0*angle)+six*sin(6.0*angle)
        +four*sin(4.0*angle))/weights;
    vec2 folded=radius*vec2(0.83+0.17*lobes,0.32*petals);
    // The inner and outer structures open differently. A hit changes arch
    // widths; it does not scale every layer away from the screen center.
    folded.x+=motionScale*(0.55*bassGesture()*sin(radius*2.4)
        +0.20*low*sin(radius*1.8));
    folded.y+=motionScale*(0.18*impactMotion.y*sin(radius*3.0)*petals
        +0.16*detail*sin(radius*4.0)*lobes);
    folded.y+=motionScale*melodicCurve(radius*1.5)*0.85;
    vec2 q=folded;
    vec3 c=vec3(0.004,0.006,0.022);
    float occlusion=1.0;
    for(int i=0;i<4;++i) {
        float layer=float(i);
        q=rotate2d(0.63+0.24*branching*sin(layer*1.7))*q;
        q=sqrt(q*q+vec2(0.004))-vec2(
            0.72+(0.22*bloom+0.32*motionScale*bassGesture())*cos(layer*1.3),
            0.42+0.20*branching*sin(layer*1.5+0.8));
        float rounded=sin(q.x*3.2)+cos(q.y*3.5);
        float pointed=1.35*(abs(sin(q.x*2.5))-abs(cos(q.y*3.0)));
        float geometry=mix(rounded,pointed,0.18+0.55*branching)
            +0.35*sin(q.x*2.0+q.y*3.0);
        geometry+=0.16*fineStructure*sin(q.x*7.0-q.y*4.0);
        geometry+=motionScale*(0.18*detail*sin(q.y*6.0+layer)
            +0.12*high*cos(q.x*8.0-layer));
        geometry+=motionScale*melodicCurve(q.x+layer)*0.65;
        float phase=geometry*5.0+layer*1.4;
        float edge=fieldRidge(phase,26.0);
        float inset=fieldRidge(phase*2.0+1.0,42.0);
        float halo=fieldRidge(phase,2.0);
        vec3 dye=fieldPalette(layer*1.25+geometry*0.35+1.9);
        float spatial=0.5+0.5*sin(p.x*2.0+p.y*3.0+layer*1.7);
        float accent=(i<2 ? impactMotion.x*(1.0-spatial) : (i<4 ? impactMotion.y*spatial : impactMotion.z));
        float pane=pow(0.5+0.5*sin(phase),2.0);
        c*=1.0-pane*0.16;
        c+=occlusion*dye*pane*(0.12+0.15*detail);
        c+=occlusion*dye*(edge*(0.6+0.6*detail+1.2*accent)+halo*0.055);
        c+=occlusion*vec3(0.5,0.8,1.0)*inset*(0.08+0.30*high+0.6*impactMotion.z)*0.45;
        c+=occlusion*dye*pow(halo,0.4)*(i<2 ? low*0.08 : mid*0.06);
        occlusion*=0.67;
        q=q*1.48+vec2(0.11,-0.07);
    }
    // Broad background interference makes the spaces between fine layers part
    // of the composition, rather than an empty backdrop around one emblem.
    float chamber=sin(p.x*2.0+sin(p.y*2.3)+motionScale*low*0.65);
    c+=vec3(0.035,0.018,0.08)*(0.5+0.5*chamber);
    color=vec4(musicalFinish(c*1.55),1.0);
}
