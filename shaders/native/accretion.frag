#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=p;q.y*=3.2;q.y+=0.18*lowDrive()*sin(q.x*2.0)*exp(-q.x*q.x*0.3);float r=length(q),a=atan(q.y,q.x);float disk=smoothstep(0.36,0.53,r)*exp(-r*1.2);float dust=fractal(vec3(vec2(a*3.0,r*10.0-t*0.35+0.9*midDrive()+noteCurve(a)),t*0.05));vec3 c=metal(dust*1.5)*disk*(0.5+2.5*dust);float lens=abs(length(p)-0.39);c+=vec3(1.0,0.68,0.32)*glow(lens,0.014+0.018*highDrive())*0.75;float black=1.0-smoothstep(0.34,0.385,length(p));c*=1.0-black;vec2 z=p*140.0;float h=hash21(floor(z));c+=vec3(0.35,0.5,0.8)*glow(length(fract(z)-0.5),0.017)*smoothstep(0.988,1.0,h)*(1.0-black);color=finishScene(c,p);}
