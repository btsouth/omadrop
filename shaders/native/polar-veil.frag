#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec3 c=mix(vec3(0.018,0.032,0.10),vec3(0.001,0.004,0.018),smoothstep(-0.7,0.8,p.y));for(int i=0;i<18;i++){float f=float(i)/18.0;float y=-0.18+0.24*sin(p.x*1.7+t*0.16+f*1.3)+0.09*sin(p.x*4.0-t*0.13+f*5.0);y+=0.19*lowDrive()*sin(p.x*1.4+f)+0.10*midDrive()*sin(p.x*4.0+f*2.0)+0.15*noteCurve(p.x);float d=p.y-y-f*0.22;float threads=0.4+0.6*noise3(vec3(p.x*75.0+f*7.0,f*3.0,t*0.14));vec3 h=mix(vec3(0.06,0.95,0.48),vec3(0.35,0.18,0.8),f);c+=h*0.09*exp(-abs(d)*7.0)*threads;}
 vec2 z=p*110.0;float h=hash21(floor(z));c+=vec3(0.45,0.58,0.8)*glow(length(fract(z)-0.5),0.022+0.028*highDrive())*smoothstep(0.984,1.0,h)*smoothstep(-0.2,0.5,p.y);float mountain=-0.68+0.07*sin(p.x*3.0)+0.025*sin(p.x*15.0);c=mix(c,vec3(0.008,0.016,0.035),1.0-smoothstep(mountain-0.015,mountain,p.y));color=finishScene(c*1.8,p);}
