#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec3 c=vec3(0.006,0.018,0.025);for(int i=0;i<80;i++){float f=float(i),h=hash21(vec2(f,7));float u=t*(0.04+h*0.04)+f*2.4;vec2 center=vec2(sin(u*1.3+f)*1.5,cos(u*0.8+f*0.7)*0.75);center+=vec2(sin(f*1.3),cos(f*0.7))*0.24*lowDrive()+vec2(cos(f*2.1),sin(f))*0.15*midDrive();float d=length(p-center);float w=(0.003+h*h*0.018)*(1.0+1.8*highDrive());vec3 hue=mix(vec3(0.06,0.5,0.45),vec3(1.0,0.61,0.14),h);c+=hue*glow(d,w)*(0.6+h*0.8);c+=hue*glow(d,w*5.0)*0.08;}color=finishScene(c,p);}
