#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"

// Ember Atlas: a finite, fixed-view landscape. Audio changes its elevation
// and strata. The camera never flies and the texture never scrolls.
float low, middle, high;
float heightAt(vec2 p) {
    float ridge=sin(p.x*2.2+0.45*sin(p.y*1.6));
    float crossRidge=cos(p.y*2.8+p.x*0.7);
    return 0.17*ridge + 0.11*crossRidge
        +motionScale*(0.19*low*ridge + 0.14*impactMotion.x*exp(-p.x*p.x*0.8)
        +0.08*middle*sin(p.x*5.0+p.y*2.3)
        +0.035*musicalDetail(clamp(p.x*0.18+0.5,0.0,1.0))*cos(p.y*8.0));
}
void main() {
    low=musicalBand(0);middle=musicalBand(1);high=musicalBand(2);
    vec2 p=(uv-0.5)*vec2(resolution.x/resolution.y,1.0);
    vec3 ro=vec3(0.0,2.5,3.8);
    vec3 forward=normalize(vec3(0.0,-1.9,-3.8));
    vec3 right=vec3(1,0,0), up=cross(right,forward);
    vec3 rd=normalize(forward*1.65+p.x*right+p.y*up);
    vec3 c=mix(vec3(0.015,0.009,0.04),vec3(0.06,0.022,0.04),uv.y);
    float t=0.0;bool hit=false;
    for(int i=0;i<96;++i) {
        vec3 q=ro+rd*t;
        float d=q.y-heightAt(q.xz);
        if(d<0.003) {hit=true;break;}
        t+=max(0.012,d*0.42);
        if(t>12.0)break;
    }
    if(hit) {
        // Refine the intersection before shading. Coarse march endpoints
        // otherwise turn smooth contour lines into discontinuous stair steps.
        for(int refine=0;refine<5;++refine) {
            vec3 v=ro+rd*t;
            float d=v.y-heightAt(v.xz);
            vec3 next=v+rd*0.001;
            float derivative=(next.y-heightAt(next.xz)-d)/0.001;
            if(abs(derivative)>0.05) t-=clamp(d/derivative,-0.03,0.03);
        }
        vec3 q=ro+rd*t;
        q.y=heightAt(q.xz);
        float e=0.008;
        vec3 n=normalize(vec3(heightAt(q.xz-vec2(e,0))-heightAt(q.xz+vec2(e,0)),2.0*e,
            heightAt(q.xz-vec2(0,e))-heightAt(q.xz+vec2(0,e))));
        float light=max(0.0,dot(n,normalize(vec3(-0.5,1.0,0.8))));
        float level=q.y*22.0;
        float width=max(fwidth(level)*1.3,0.025);
        float contour=1.0-smoothstep(width,width*2.0,abs(fract(level)-0.5));
        float minorLevel=q.y*88.0;
        float minor=1.0-smoothstep(max(fwidth(minorLevel),0.025),max(fwidth(minorLevel)*2.0,0.05),abs(fract(minorLevel)-0.5));
        float heat=0.5+0.5*sin(q.y*7.0+q.x*0.4);
        vec3 mineral=mix(vec3(0.025,0.08,0.19),vec3(0.52,0.065,0.13),heat);
        vec3 lineColor=mix(vec3(0.08,0.6,0.8),vec3(1.0,0.38,0.12),heat);
        float zone=exp(-q.x*q.x*0.22);
        c=mineral*(0.22+light*0.9);
        c+=lineColor*contour*(0.14+0.38*middle+snare*0.50*zone);
        c+=vec3(0.9,0.67,0.32)*minor*(0.04+0.16*high+0.45*hat)*smoothstep(0.1,0.5,n.y);
        c+=vec3(1.0,0.17,0.055)*kick*zone*0.24;
        float reflectance=pow(max(0.0,dot(n,normalize(normalize(vec3(-0.5,1.0,0.8))-rd))),32.0);
        c+=lineColor*reflectance*(0.12+0.16*low);
        c=mix(c,vec3(0.025,0.017,0.047),1.0-exp(-0.018*t*t));
    }
    color=vec4(musicalFinish(c*1.8),1.0);
}
