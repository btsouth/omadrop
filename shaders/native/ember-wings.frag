#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec3 c=vec3(0.015,0.003,0.007);for(int i=0;i<32;i++){float f=float(i)/32.0;vec2 q=p;q.x=abs(q.x);float x=0.13+f*1.1+0.12*sin(q.y*3.0+t*0.23+f*6.0)+0.07*sin(q.y*7.0-t*0.3+f*10.0);x+=0.23*lowDrive()*sin(q.y*1.7+f*2.0)+0.10*midDrive()*sin(q.y*5.0+f*3.0)+0.12*noteCurve(q.y);x*=0.8+0.4*q.y;float d=q.x-x;float fade=exp(-pow(q.y+0.1,2.0)*1.5);vec3 h=mix(vec3(0.75,0.035,0.06),vec3(1.0,0.7,0.25),f);c+=h*(0.06*glow(d,0.028)+0.12*stroke(d,0.0015+0.003*highDrive()))*fade;}color=finishScene(c,p);}
