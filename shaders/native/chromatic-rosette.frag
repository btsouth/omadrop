#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=p;float r=length(q),a=atan(q.y,q.x);a=abs(mod(a+t*0.035,tau/8.0)-tau/16.0);q=vec2(cos(a),sin(a))*r;vec3 c=vec3(0.015,0.006,0.035);for(int i=0;i<7;i++){q=abs(q)-vec2(0.56+0.03*sin(t*0.13)+0.07*midDrive(),0.34);q=rotate2d(0.72+t*0.025)*q*1.42;float d=abs(length(q)-0.38-0.19*lowDrive());float f=float(i);c+=jewel(f*0.11+t*0.007)*(0.08*glow(d,0.05)+0.22*stroke(d,0.006+0.010*highDrive()))*exp(-f*0.09);}color=finishScene(c,p);}
