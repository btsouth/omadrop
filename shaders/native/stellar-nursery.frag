#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=rotate2d(t*0.015)*p;float r=length(q);float a=atan(q.y,q.x);vec3 v=vec3(q*2.3,t*0.065);v.xy+=q*exp(-r*r)*0.6*lowDrive()+vec2(noteCurve(q.y),noteCurve(q.x))*0.3;v.z+=0.22*midDrive();float n=fractal(v+vec3(fractal(v+4.0)*2.0));float cloud=pow(max(0.0,n-0.25),2.0)*6.0;vec3 c=mix(vec3(0.06,0.12,0.65),vec3(0.9,0.14,0.35),n)*cloud;float arm=sin(a*3.0-r*5.0+t*0.12);c+=vec3(1.0,0.55,0.18)*pow(max(arm,0.0),8.0)*exp(-r*1.5)*0.4;for(int i=0;i<3;i++){vec2 z=q*(65.0+float(i)*37.0);vec2 id=floor(z),f=fract(z)-0.5;float h=hash21(id+float(i));float d=length(f);c+=vec3(0.6,0.8,1.0)*glow(d,0.015+0.02*highDrive())*smoothstep(0.975,1.0,h)*(0.5+float(i)*0.3);}c+=vec3(1.0,0.75,0.45)*0.06/(0.05+r*r);color=finishScene(c,p);}
