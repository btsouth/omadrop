#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"

// Seven crystal voices. Bass opens their bases, midrange shapes the crowns,
// snare illuminates facet seams, and treble changes the small upper shards.
void main() {
    vec2 p=(uv-0.5)*vec2(resolution.x/resolution.y,1.0);
    float low=musicalBand(0), mid=musicalBand(1), high=musicalBand(2);
    vec3 teal=mix(vec3(0.025,0.7,0.75),palettePrimary(0.4),0.15);
    vec3 violet=vec3(0.45,0.12,0.9), gold=vec3(1.0,0.38,0.12);
    vec3 c=vec3(0.002,0.004,0.008);
    c+=teal*exp(-abs(p.y+0.31)*35.0)*exp(-p.x*p.x*3.0)*(0.025+0.08*low);
    for(int i=0;i<7;++i) {
        float f=float(i), x=(f-3.0)*0.205;
        float detail=musicalDetail(f/6.0);
        float base=-0.30+0.025*sin(f*2.1);
        float h=0.27+0.17*(0.5+0.5*sin(f*1.8+0.7))
            +motionScale*(0.10*mid+0.10*detail+0.055*impactMotion.y);
        float w=0.054+motionScale*(0.025*low+0.028*impactMotion.x);
        vec2 q=p-vec2(x,base);
        float y=q.y/h;
        float profile=max(0.0,min(y/0.22,(1.0-y)/0.38));
        float sd=abs(q.x)-w*profile;
        float aa=max(fwidth(sd),0.0007);
        float mask=(1.0-smoothstep(-aa,aa,sd))*step(0.0,y)*step(y,1.0);
        float facet=clamp(q.x/max(w*profile,0.001),-1.0,1.0);
        vec3 dye=mix(teal,violet,f/6.0);
        float faceLight=facet<0.0?0.75:0.25;
        faceLight+=0.16*smoothstep(0.4,0.95,y);
        vec3 crystal=dye*faceLight;
        float seam=line(q.x+0.18*w*profile,0.0012)*step(0.05,y)*step(y,0.95);
        crystal+=mix(dye,vec3(0.7,0.92,1.0),0.55)*seam*(0.3+1.5*snare);
        crystal+=gold*exp(-y*9.0)*(0.12+0.9*kick);
        crystal+=vec3(0.25,0.8,1.0)*detail*0.22;
        c=mix(c,crystal,mask);
        c+=dye*exp(-abs(sd)*180.0)*step(0.0,y)*step(y,1.0)*(0.12+0.25*mid);
        vec2 shard=q-vec2(0.012*sin(f),h+0.035+motionScale*0.018*impactMotion.z);
        float diamond=abs(shard.x)+abs(shard.y)*0.65;
        float size=0.007+0.006*high+motionScale*0.006*impactMotion.z;
        c+=mix(dye,vec3(0.8,0.94,1.0),0.6)*line(diamond,size)*(0.15+1.4*hat+0.3*high);
    }
    color=vec4(musicalFinish(c*1.4),1.0);
}
