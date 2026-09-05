#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"

// Bass anchors, midrange bridges, treble satellites. All positions are fixed
// functions of measured sound; no orbit continues while a note is held.
vec2 node(int i) {
    float f = float(i);
    float a = f * 2.399963;
    float r = 0.13 + 0.055 * sqrt(f);
    vec2 home = vec2(cos(a)*r*1.85, sin(a)*r);
    float bassRole = i % 3 == 0 ? 1.0 : 0.0;
    return home * (1.0 + motionScale * bassRole *
        (0.12 * musicalBand(0) + 0.10 * impactMotion.x));
}
float segment(vec2 p, vec2 a, vec2 b) {
    vec2 d = b-a;
    return length(p-a-d*clamp(dot(p-a,d)/max(dot(d,d),0.0001),0.0,1.0));
}
void main() {
    vec2 p = (uv-0.5)*vec2(resolution.x/resolution.y,1.0);
    float mid = musicalBand(1), high = musicalBand(2);
    vec3 blue = mix(vec3(0.04,0.45,1.0),palettePrimary(0.2),0.2);
    vec3 gold = vec3(1.0,0.38,0.09);
    vec3 violet = vec3(0.46,0.14,0.95);
    vec3 c = vec3(0.001,0.002,0.007);
    for(int i=0;i<14;++i) {
        vec2 a=node(i), b=node((i+3)%14);
        float d=segment(p,a,b);
        float detail=musicalDetail(float(i)/13.0);
        float width=0.0012+0.0018*mid+0.002*impactMotion.y*motionScale;
        c += mix(blue,violet,float(i%3)/2.0)*exp(-d*180.0)*(0.04+0.12*mid);
        c += blue*line(d,width)*(0.12+0.35*mid+0.9*snare*detail);
    }
    for(int i=0;i<14;++i) {
        vec2 q=p-node(i);
        float role=float(i%3);
        float bassRole=role==0.0?1.0:0.0;
        float highRole=role==2.0?1.0:0.0;
        float radius=0.015+0.009*bassRole+0.013*musicalBand(0)*bassRole
            +motionScale*(0.017*impactMotion.x*bassRole+(0.008*high+0.005*impactMotion.z)*highRole);
        float r=length(q), edge=fwidth(r)*1.2;
        float mask=1.0-smoothstep(radius-edge,radius+edge,r);
        vec2 xy=q/max(radius,0.001);
        float z=sqrt(max(0.0,1.0-dot(xy,xy)));
        float lit=max(0.0,dot(normalize(vec3(xy,z)),normalize(vec3(-0.5,0.65,1.0))));
        vec3 dye=mix(blue,gold,bassRole);
        dye=mix(dye,violet,highRole);
        c += dye*exp(-r*38.0)*(0.06+0.18*kick*bassRole+0.65*high*highRole);
        c = mix(c,dye*(0.12+0.8*lit)+vec3(0.6,0.82,1.0)*pow(lit,24.0)*0.8,mask);
        c += gold*line(r-radius-0.012,0.0018)*kick*bassRole*0.9;
        float crossLight=exp(-abs(q.x)*900.0-abs(q.y)*55.0)
                       +exp(-abs(q.y)*900.0-abs(q.x)*55.0);
        c += vec3(0.55,0.85,1.0)*crossLight*highRole*(0.12*high+0.85*hat);
    }
    color=vec4(musicalFinish(c),1.0);
}
