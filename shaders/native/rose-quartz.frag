#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=rotate2d(0.3)*p;vec3 c=vec3(0.025,0.008,0.04);for(int i=0;i<20;i++){float f=float(i);float curve=sin(q.x*1.3+t*0.13+f*0.15)*0.55+sin(q.x*2.7-t*0.07+f*0.2)*0.18+(f-10.0)*0.055;curve+=0.25*lowDrive()*sin(q.x*1.5+f*0.2)+0.12*midDrive()*sin(q.x*4.2+f*0.3)+0.12*noteCurve(q.x);float d=q.y-curve;float light=0.5+0.5*sin(q.x*3.0+f*0.3+t*0.09);vec3 h=mix(vec3(0.12,0.04,0.25),vec3(1.0,0.35,0.5),f/20.0);c=mix(c,h*(0.3+light),(1.0-smoothstep(-0.01,0.05,d))*0.24);c+=mix(h,vec3(0.8,0.65,0.5),0.3)*glow(d,0.007+0.012*highDrive())*0.24;}color=finishScene(c*1.8,p);}
