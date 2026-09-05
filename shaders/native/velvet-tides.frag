#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec3 c=vec3(0.008,0.02,0.04);for(int i=0;i<16;i++){float f=float(i);vec2 q=p+vec2(f*0.13,t*0.035);float n=fractal(vec3(q*2.1,t*0.09+f*0.15));float y=-0.8+f*0.10+0.28*sin(p.x*2.2+t*0.21+f*0.24)+0.3*(n-0.5);y+=0.22*lowDrive()*sin(p.x*1.8+f*0.24)+0.11*midDrive()*sin(p.x*5.0+f*0.6)+0.12*noteCurve(p.x+f*0.1);float d=p.y-y;vec3 h=mix(vec3(0.015,0.12,0.28),vec3(0.12,0.85,0.77),f/16.0);c=mix(c,h*(0.4+1.1*n),(1.0-smoothstep(-0.025,0.025,d))*0.4);c+=h*0.13*glow(d,0.013+0.012*highDrive());}color=finishScene(c*1.7,p);}
