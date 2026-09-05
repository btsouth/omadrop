// Shared materials for the evolving collection. Geometry lives in each scene.
#include "musical-response.glsl"
uniform float flowTime;
uniform float qualityScale;
// Continuous analyzer envelopes, independent of the background evolution clock.
// No camera zoom or whole-frame exposure modulation: scenes assign these to forms.
float lowDrive() { return motionScale*(0.72*(1.0-exp(-4.0*bassBody))
    +0.28*(1.0-exp(-5.0*impactMotion.x))); }
float midDrive() { return motionScale*(0.65*musicalBand(1)
    +0.35*(1.0-exp(-4.0*impactMotion.y))); }
float highDrive() { return motionScale*(0.55*musicalBand(2)
    +0.45*(1.0-exp(-4.0*impactMotion.z))); }
float noteCurve(float x) { return motionScale*melodicCurve(x); }
float clockTime() { return flowTime*4.0; }
vec2 canvas() { return (uv-0.5)*vec2(resolution.x/resolution.y,1.0)*2.0; }
float hash21(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float hash31(vec3 p) { return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453); }
float noise3(vec3 p) {
 vec3 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
 return mix(mix(mix(hash31(i),hash31(i+vec3(1,0,0)),f.x),mix(hash31(i+vec3(0,1,0)),hash31(i+vec3(1,1,0)),f.x),f.y),mix(mix(hash31(i+vec3(0,0,1)),hash31(i+vec3(1,0,1)),f.x),mix(hash31(i+vec3(0,1,1)),hash31(i+vec3(1,1,1)),f.x),f.y),f.z);
}
float fractal(vec3 p) { float v=0.0,a=0.5;for(int i=0;i<4;i++){v+=a*noise3(p);p=p*2.03+vec3(13.1,7.7,5.2);a*=0.5;}return v; }
vec3 jewel(float x) { return 0.52+0.48*cos(tau*(x+vec3(0.04,0.30,0.57))); }
vec3 metal(float x) { return mix(vec3(0.04,0.16,0.28),vec3(1.0,0.59,0.22),clamp(x,0.0,1.0)); }
float glow(float d,float w) {return w*w/(d*d+w*w);}
float stroke(float d,float w) { return 1.0-smoothstep(w,w+max(fwidth(d),0.0008),abs(d)); }
vec4 finishScene(vec3 c,vec2 p) { c*=1.0-0.19*smoothstep(0.6,2.0,length(p));return vec4(pow(1.0-exp(-max(c,vec3(0))),vec3(0.85)),1); }
float segment(vec2 p,vec2 a,vec2 b) {vec2 q=p-a,v=b-a;return length(q-v*clamp(dot(q,v)/max(dot(v,v),1e-6),0.0,1.0));}
vec2 projectPoint(vec3 p) {return p.xy/(3.5+p.z);}
