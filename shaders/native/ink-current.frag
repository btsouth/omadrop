#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"

// Chromatic Pleats: broad shadowed fans, nested seams, iridescent fine fibers.
// Fixed spatial phases; only measured audio changes geometry or material.
void main() {
    vec2 p=(uv-0.5)*vec2(resolution.x/resolution.y,1.0);
    float low=musicalBand(0), middle=musicalBand(1), high=musicalBand(2);
    vec3 c=vec3(0.008,0.012,0.025);
    for(int i=0;i<11;++i) {
        float f=float(i), depth=f/10.0;
        float a=-0.42+0.085*f;
        vec2 q=rotate2d(a)*(p-vec2((depth-0.5)*0.06,(depth-0.5)*0.37));
        float x=q.x;
        float role=exp(-12.0*(depth-0.5)*(depth-0.5));
        float detail=musicalDetail(clamp(x*0.6+0.5,0.0,1.0));
        float center=0.17*sin(x*3.0+f*0.38)+0.07*cos(x*6.0-f*0.2);
        center+=motionScale*(0.085*low*(1.0-depth)*sin(x*3.0+f*0.38)
            +0.17*impactMotion.x*(1.0-depth)*exp(-2.0*(x+0.3)*(x+0.3))
            +0.065*middle*role*sin(x*8.0+f)
            +0.07*impactMotion.y*role*sin(x*16.0+f)
            +0.03*detail*sin(x*12.0+f*0.4)
            +0.014*impactMotion.z*depth*sin(x*58.0+f));
        float width=0.035+0.025*(0.5+0.5*sin(x*2.5+f));
        float v=(q.y-center)/width;
        float sd=abs(q.y-center)-width;
        float aa=max(fwidth(sd),0.0008);
        float mask=(1.0-smoothstep(-aa,aa,sd))*(1.0-smoothstep(0.74,1.05,abs(x)));
        float shadow=exp(-abs(q.y-center+width+0.02)*48.0)*(1.0-smoothstep(0.8,1.05,abs(x)));
        c*=1.0-0.45*shadow;
        float arc=sqrt(max(0.0,1.0-v*v));
        float pleat=0.05*sin(v*17.0+x*3.0);
        vec3 n=normalize(vec3(-0.15*cos(x*3.0+f),v+pleat,0.35+arc));
        float light=max(0.0,dot(n,normalize(vec3(-0.3,0.6,1.0))));
        float hue=depth*2.0+x*0.65;
        vec3 dye=0.48+0.46*cos(vec3(0.5,2.2,4.2)+hue*3.8);
        dye=mix(dye,vec3(0.08,0.35,0.65),0.16);
        float fiberPhase=v*98.0+x*35.0;
        float fiber=(0.5+0.5*sin(fiberPhase))*(1.0-smoothstep(0.8,2.5,fwidth(fiberPhase)));
        float rim=pow(clamp(abs(v),0.0,1.0),16.0);
        float seam=pow(0.5+0.5*cos(x*16.0+f),12.0);
        vec3 sheet=dye*(0.12+0.9*light)*(0.9+0.1*fiber);
        sheet+=vec3(0.8,0.9,1.0)*pow(light,30.0)*0.6;
        sheet+=vec3(1.0,0.4,0.16)*kick*(1.0-depth)*0.45*arc;
        sheet+=vec3(0.75,0.9,1.0)*snare*seam*role*0.75;
        sheet+=vec3(0.25,0.85,1.0)*(0.25*high+0.8*hat)*depth*(0.2*fiber+rim);
        sheet+=dye*middle*0.10*role;
        c=mix(c,sheet,mask);
    }
    color=vec4(musicalFinish(c*1.55),1.0);
}
