#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"
#include "musical-field.glsl"

// Molten Current. Original analytic field, inspired by the multiscale relief
// of Geiss's Reaction Diffusion presets. No preset equations are copied.
// Broad liquid plates, engraved tributaries, submerged light and surface glints.
float liquidField(vec2 q,float mid,float spectral) {
    return sin(q.x*2.1+sin(q.y*2.2))+cos(q.y*1.8-q.x*0.65)
        +0.42*sin(q.x*4.2+q.y*2.6+0.65*sin(q.y*3.5))
        +motionScale*(0.12*mid*sin(q.x*5.0-q.y*3.0)
        +0.06*spectral*cos(q.y*8.0+q.x*2.0));
}
void main() {
    vec2 p=(uv-0.5)*vec2(resolution.x/resolution.y,1.0)*3.4;
    float low=musicalBand(0), mid=musicalBand(1), high=musicalBand(2);
    float spectral=musicalDetail(uv.x*0.65+uv.y*0.35);
    // Fixed local centers of the resting liquid basins. Each surround stays
    // in place while its enclosed rings move together, like a speaker cone.
    const vec2 centers[7]=vec2[7](
        vec2(0.054,-1.504),vec2(2.258,-1.368),vec2(-1.754,-0.648),
        vec2(0.449,-0.264),vec2(2.557,-0.208),vec2(3.046,1.192),vec2(-1.115,1.696));
    float excursion=coneExcursion();
    vec2 cone=p;
    float core=0.0;
    for(int i=0;i<7;++i) {
        cone-=coneOffset(p,centers[i],0.36,1.05,excursion);
        // Protect the bass interiors. Sustained voices own the connecting
        // folds, so their movement does not consume cone excursion headroom.
        float interior=1.0-smoothstep(0.38,0.92,length(p-centers[i]));
        // Smooth union: max() left a normal crease where two supports met.
        core=1.0-(1.0-core)*(1.0-interior);
    }
    // Fixed resting warp, without fieldWarp's implicit global kick/snare bend.
    vec2 q=cone;
    q+=0.38*vec2(sin(cone.y*1.7+cone.x*0.4),cos(cone.x*1.5-cone.y*0.5));
    q+=0.19*vec2(sin(q.y*3.4+q.x),cos(q.x*3.0-q.y));
    float lower=sustainedRegister(10), middle=sustainedRegister(15);
    float upper=sustainedRegister(20), air=sustainedRegister(25);
    float mantle=1.0-core;
    // Long, unequal folds carry separate sustained spectral registers. A held
    // spectrum holds this pose; changing notes reshape the existing surface.
    vec2 fold=vec2(
        lower*sin(q.y*1.15+0.6)-upper*cos(q.y*1.75+q.x*0.3),
        middle*sin(q.x*1.25-0.8)+upper*cos(q.x*1.9+q.y*0.25));
    // Retain the finer spectral fingerprint inside the broad registers.
    // Equal total activity with different notes must not become the same pose.
    fold+=vec2(melodicCurve(q.y+1.4),melodicCurve(q.x-0.8))*1.6;
    q+=motionScale*mantle*fold*0.26;
    // Shorter-scale flex for percussion, spatially separate from the bass
    // cores. A bounded mapping makes ordinary mixed-song hits readable while
    // preserving stronger-hit headroom. Never use the raw attack as a light.
    float snap=1.0-exp(-6.0*impactMotion.y);
    float tick=1.0-exp(-6.0*impactMotion.z);
    q+=motionScale*mantle*snap*0.065
        *vec2(sin(q.y*3.0+q.x),cos(q.x*2.0-q.y));
    // The interstitial landscape can reconnect as the arrangement changes.
    // This changes the broad surface, not a second drawing or a timer-driven
    // transition. Core interiors retain their reference field and excursion.
    float landscape=lower*0.58*cos(q.x*0.85+q.y*1.65)
        +middle*0.62*sin(q.x*1.35-q.y*0.75)
        +upper*0.48*sin(q.x*1.1+q.y+1.3);
    float f=liquidField(q,mid*0.25,spectral*0.25)
        +motionScale*mantle*landscape;
    float basePhase=f*7.0+q.x*0.7+motionScale*melodicCurve(q.x)*0.14;
    float phase=basePhase;
    float plate=0.5+0.5*sin(phase);
    // Percussion flexes the cross-section of the folds. Fine relief catches
    // light from the same deformed normal, instead of flashing an overlay.
    float shoulder=pow(0.5+0.5*cos(phase-0.7),3.0);
    float strike=motionScale*snap*mantle;
    float grain=motionScale*(0.65*tick+0.12*high);
    float relief=sin(phase)+0.17*sin(phase*2.0+q.y)
        +0.33*strike*sin(phase*2.0-0.6)*shoulder
        +0.075*grain*sin(phase*5.0+q.y*2.0)*shoulder;
    // Derivatives of the complete audio-deformed field give coherent lighting.
    vec2 gradient=vec2(dFdx(relief),dFdy(relief))*resolution.y/112.0;
    vec3 n=normalize(vec3(-gradient,1.0));
    vec3 light=normalize(vec3(-0.45,0.65,1.0));
    float diffuse=max(dot(n,light),0.0);
    float reflected=max(dot(reflect(-light,n),vec3(0,0,1)),0.0);
    float spec=pow(reflected,32.0);
    float softSpec=pow(reflected,7.0);
    float seam=fieldRidge(phase+2.8,24.0);
    float vein=fieldRidge(phase*4.0+q.y*3.0+grain,34.0);
    // Indigo recesses, petrol shoulders, violet interference, pale silver
    // crests. Thin-film color belongs to the surface orientation and spectrum.
    float facing=clamp(n.z,0.0,1.0);
    float film=0.5+0.5*cos(3.4*facing+f*0.6+q.y*0.35
        +motionScale*mantle*(upper*1.1-middle*0.65));
    float mineral=0.5+0.5*sin(f*1.35+q.y*0.8+0.6);
    vec3 metal=mix(vec3(0.012,0.026,0.07),vec3(0.20,0.025,0.14),mineral);
    metal=mix(metal,vec3(0.045,0.30,0.34),smoothstep(0.15,0.92,plate)*0.76);
    float cavity=smoothstep(-0.95,0.65,sin(phase));
    vec3 c=metal*(0.24+1.5*diffuse)*(0.48+0.52*cavity);
    vec3 pearl=mix(vec3(0.10,0.50,0.60),vec3(0.63,0.34,0.60),film*0.65+mineral*0.25);
    c+=pearl*softSpec*0.34;
    c+=vec3(0.62,0.83,0.88)*spec*0.90;
    // Continuous light underneath the lips adds depth without exposure pulses.
    c+=vec3(0.025,0.46,0.56)*seam*(0.36+0.32*mid);
    c+=pearl*vein*(0.045+0.18*grain)*shoulder;
    c+=vec3(0.015,0.075,0.11)*pow(plate,3.0)*(0.25+low);
    c+=vec3(0.09,0.014,0.045)*mineral*(1.0-cavity)*0.55;
    c+=vec3(0.055,0.09,0.15)*mantle*air*softSpec*motionScale;
    c*=0.78+0.22*pow(max(0.0,1.0-dot(uv-0.5,uv-0.5)),0.5);
    color=vec4(musicalFinish(c*1.75),1.0);
}
