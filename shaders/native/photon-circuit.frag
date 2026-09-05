#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=rotate2d(0.65)*p*5.0;q.x+=t*0.06;vec2 id=floor(q),f=fract(q)-0.5;float h=hash21(id);if(h>0.5)f.x=-f.x;float d=abs(abs(f.x+f.y)-0.5-0.14*lowDrive()*sin(h*tau))*0.707;vec3 c=vec3(0.005,0.015,0.03);c+=vec3(0.02,0.18,0.22)*glow(d,0.12);float travel=0.5+0.5*sin((id.x+id.y)*0.6+t*0.5+midDrive()*2.0);c+=mix(vec3(0.05,0.55,0.65),vec3(1.0,0.54,0.16),h)*(stroke(d,0.014)*0.7+glow(d,0.03)*0.4)*travel;float dotD=length(f-vec2(0.25,-0.25));c+=vec3(0.45,0.9,1.0)*glow(dotD,0.028+0.065*highDrive())*smoothstep(0.75,1.0,h);color=finishScene(c,p);}
