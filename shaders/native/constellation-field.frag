#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"

// Opal Bloom. Fixed camera and a solid folded torus. Low frequencies change
// the body, middle frequencies open folds, high frequencies reveal ridges.
float body, folds, fine;
float sculpture(vec3 p) {
    p.yz = rotate2d(-0.46) * p.yz;
    p.xy = rotate2d(0.35) * p.xy;
    float a=atan(p.y,p.x), r=length(p.xy);
    // Opposing lobes bend in different directions instead of scaling the ring.
    float lowRegion=0.5+0.5*sin(a-0.4);
    float middleRegion=0.5+0.5*cos(a+0.7);
    float localSpectrum=musicalDetail(0.08+0.82*(0.5+0.5*cos(a)));
    float radius=0.72 + 0.075*cos(a*7.0)
        +body*0.11*cos(a*2.0+0.4);
    p.z-=motionScale*0.42*impactMotion.x*lowRegion*cos(a*3.0);
    float wav=sin(a*7.0 + p.z*2.0);
    vec2 q=vec2(r-radius,p.z);
    q=rotate2d(a*3.5 + folds*0.70*sin(a*3.0)*middleRegion
        +motionScale*0.32*localSpectrum*cos(a*4.0))*q;
    float thickness=0.28+0.038*wav+0.025*folds*cos(a*3.0)
        +motionScale*0.045*(localSpectrum-0.45*musicalBand(1));
    // Smooth lobes produce a continuous silhouette with deep occluded folds.
    float ridges=0.009*sin(a*35.0+atan(q.y,q.x)*3.0);
    return (length(q*vec2(0.74,1.65))-thickness+ridges)*0.42;
}
vec3 normalAt(vec3 p) {
    vec2 e=vec2(0.002,-0.002);
    return normalize(e.xyy*sculpture(p+e.xyy)+e.yyx*sculpture(p+e.yyx)
        +e.yxy*sculpture(p+e.yxy)+e.xxx*sculpture(p+e.xxx));
}
void main() {
    body=motionScale*musicalBand(0);
    folds=motionScale*(musicalBand(1)+0.85*impactMotion.y);
    fine=musicalBand(2);
    vec2 p=(uv-0.5)*vec2(resolution.x/resolution.y,1.0);
    vec3 ro=vec3(0.0,0.15,3.7), rd=normalize(vec3(p*2.55,-3.0));
    vec3 c=mix(vec3(0.012,0.020,0.045),vec3(0.055,0.016,0.055),uv.y);
    c+=vec3(0.035,0.06,0.13)*exp(-dot(p,p)*5.0);
    float travel=0.0; bool hit=false;
    for(int i=0;i<120;++i) {
        float d=sculpture(ro+rd*travel);
        if(d<0.0006) {hit=true;break;}
        travel+=max(d,0.0004);
        if(travel>6.0) break;
    }
    if(hit) {
        vec3 pos=ro+rd*travel, n=normalAt(pos);
        float facing=max(0.0,dot(n,-rd));
        vec3 l=normalize(vec3(-0.6,0.9,1.4)), l2=normalize(vec3(0.9,-0.4,0.6));
        float diffuse=max(0.0,dot(n,l)), rim=pow(1.0-facing,2.6);
        float angle=atan(pos.y,pos.x);
        float interference=0.5+0.5*sin(facing*7.5+angle*2.0+pos.z*3.0);
        vec3 pearl=mix(vec3(0.06,0.5,0.62),vec3(0.8,0.3,0.56),interference);
        pearl=mix(pearl,vec3(0.94,0.77,0.46),pow(0.5+0.5*sin(angle*3.0+facing*6.0),5.0)*0.75);
        float ao=clamp(sculpture(pos+n*0.14)/0.075,0.25,1.0);
        float seam=pow(0.5+0.5*cos(angle*25.0+pos.z*23.0),18.0);
        float microPhase=angle*125.0+pos.z*95.0;
        float micro=(0.5+0.5*sin(microPhase))*(1.0-smoothstep(0.8,2.5,fwidth(microPhase)));
        float spec=pow(max(0.0,dot(n,normalize(l-rd))),72.0);
        float strip=pow(max(0.0,dot(n,normalize(l2-rd))),14.0);
        c=pearl*(0.18+0.8*diffuse)*ao;
        c+=vec3(0.8,0.92,1.0)*(spec*1.6+strip*0.30+rim*0.20);
        c+=vec3(1.0,0.28,0.12)*kick*(0.15+0.3*rim)*smoothstep(-0.2,-0.9,pos.y);
        c+=vec3(0.7,0.85,1.0)*snare*seam*0.60;
        c+=vec3(0.25,0.82,1.0)*(0.12*fine+0.40*hat)*micro*(0.3+rim);
        c+=pearl*fine*0.13;
    }
    c*=1.0-0.3*smoothstep(0.35,1.2,length(p));
    color=vec4(musicalFinish(c*1.35),1.0);
}
