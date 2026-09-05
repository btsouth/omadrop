#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"
#include "musical-field.glsl"

// Spectral Estuary. Intersecting full-screen filament currents and shadowed
// sheets. References the layered wave fields of Tides and Organic Light.
void main() {
    vec2 p=(uv-0.5)*vec2(resolution.x/resolution.y,1.0)*3.0;
    float low=musicalBand(0),mid=musicalBand(1),high=musicalBand(2);
    vec3 c=vec3(0.004,0.011,0.025);
    for(int i=0;i<4;++i) {
        float layer=float(i);
        vec2 q=rotate2d(-0.6+layer*0.64)*p+vec2(layer*0.8,layer*0.55);
        float spectrum=musicalDetail(clamp(0.5+q.x*0.17,0.0,1.0));
        q=fieldWarp(q,low*(1.0-layer*0.20),mid*(0.25+layer*0.25),spectrum);
        float center=q.y+0.50*sin(q.x*1.5+layer)+0.24*sin(q.x*3.2-layer);
        center+=motionScale*(0.12*spectrum*sin(q.x*4.0+layer)
            +0.025*impactMotion.z*sin(q.x*10.0+layer));
        center+=motionScale*(0.45*bassGesture()*(1.0-layer*0.25)*sin(q.x*1.4+layer)
            +melodicCurve(q.x+layer)*0.75*(0.2+layer*0.3));
        float phase=center*(9.0+layer*3.0);
        float sheet=0.5+0.5*sin(center*2.4+layer*0.8);
        float thread=fieldRidge(phase,14.0);
        float fine=fieldRidge(phase*3.0+q.x*4.0,28.0);
        float crest=fieldRidge(phase+0.5,2.0);
        float broad=pow(sheet,3.0);
        float envelope=0.3+0.7*smoothstep(0.0,0.9,sheet);
        vec3 dye=fieldPalette(layer*1.35+q.x*0.2+2.0);
        vec3 filament=dye*(thread*(0.55+0.7*spectrum)+crest*0.12);
        filament+=vec3(0.75,0.9,1.0)*fine*(0.04+0.38*high+0.35*impactMotion.z);
        filament+=vec3(1.0,0.28,0.045)*thread*impactMotion.x*(1.0-layer*0.23)*1.2;
        filament+=vec3(0.14,0.75,1.0)*crest*impactMotion.y*0.5*(0.25+layer*0.25);
        filament+=dye*broad*(0.05+0.18*low+0.14*mid);
        c*=1.0-broad*0.38;
        c+=filament*envelope*(0.6+layer*0.12);
    }
    color=vec4(musicalFinish(c*1.65),1.0);
}
