#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=p*2.0;float v=0.0;for(int i=0;i<9;i++){float f=float(i);vec2 center=vec2(sin(f*13.1+t*0.09),cos(f*7.3-t*0.07))*1.7;center+=vec2(sin(f),cos(f*1.7))*0.25*midDrive();float r=length(q-center);v+=(1.0+0.8*lowDrive())*sin(r*(12.0+f)-t*(0.5+f*0.05)-0.8*lowDrive())*exp(-r*0.5)/9.0;}float dx=dFdx(v)*resolution.x,dy=dFdy(v)*resolution.y;vec3 n=normalize(vec3(dx*0.1,dy*0.1,1));float light=pow(max(dot(n,normalize(vec3(0.4,0.7,1))),0.0),12.0);vec3 c=mix(vec3(0.015,0.09,0.14),vec3(0.3,0.7,0.64),v*0.7+0.35);c+=vec3(1.0,0.72,0.36)*light*(0.65+0.35*highDrive());color=finishScene(c*1.4,p);}
