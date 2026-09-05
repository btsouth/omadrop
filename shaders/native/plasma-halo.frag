#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=rotate2d(0.25)*p;q.y*=1.45;float r=length(q),a=atan(q.y,q.x);vec3 c=vec3(0.004,0.004,0.02);for(int i=0;i<18;i++){float f=float(i);float radius=0.65+0.08*sin(a*5.0+t*0.4+f*0.34)+0.035*sin(a*13.0-t*0.3+f);radius+=0.23*lowDrive();radius+=0.07*midDrive()*sin(a*7.0+f*0.2)+0.05*noteCurve(a);float d=r-radius-f*0.008;c+=jewel(0.55+f*0.018)*(0.08*glow(d,0.022)+0.15*stroke(d,0.002+0.004*highDrive()));}c+=vec3(0.06,0.12,0.35)*exp(-abs(r-0.8)*5.0);color=finishScene(c,p);}
