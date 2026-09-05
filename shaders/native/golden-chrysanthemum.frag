#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();float r=length(p),a=atan(p.y,p.x);vec3 c=vec3(0.02,0.012,0.008);for(int i=0;i<24;i++){float f=float(i);float petal=0.18+f*0.038+0.085*sin(a*9.0+f*0.38+t*0.2)+0.035*sin(a*23.0-f*0.4);petal+=0.06*midDrive()*sin(a*7.0+f*0.3)+0.05*noteCurve(a+f*0.1);float d=r-petal*(1.0+0.22*lowDrive());c+=mix(vec3(0.45,0.16,0.045),vec3(1.0,0.82,0.36),f/24.0)*(0.06*glow(d,0.026)+0.16*stroke(d,0.0015+0.003*highDrive()));}c+=vec3(0.35,0.08,0.015)*exp(-r*r*5.0);color=finishScene(c,p);}
