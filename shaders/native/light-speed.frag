#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();float r=max(length(p),0.006),a=atan(p.y,p.x);vec3 c=vec3(0.002,0.005,0.018);for(int i=0;i<4;i++){float f=float(i),lanes=160.0+f*73.0;float cell=floor((a/tau+0.5)*lanes);float h=hash21(vec2(cell,f));float angular=abs(fract((a/tau+0.5)*lanes)-0.5);angular+=0.09*midDrive()*sin(r*8.0+h*tau);float depth=fract(1.0/(r+0.16)*0.5-t*(0.12+f*0.03)+h);float streak=glow(angular,0.012+0.022*r+0.025*highDrive())*pow(max(0.0,1.0-depth),(16.0-9.0*lowDrive()));c+=mix(vec3(0.12,0.3,0.95),vec3(0.6,0.85,1.0),h)*streak*r*1.5;}c+=vec3(0.15,0.3,0.65)*0.017/(0.02+r*r);color=finishScene(c,p);}
