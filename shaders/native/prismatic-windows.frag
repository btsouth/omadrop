#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=rotate2d(t*0.025)*p*3.5;float a=atan(q.y,q.x);a=abs(mod(a,tau/6.0)-tau/12.0);q=length(q)*vec2(cos(a),sin(a));q+=0.25*vec2(sin(t*0.1),cos(t*0.12));vec2 id=floor(q);float d1=10.0,d2=10.0;vec2 chosen=vec2(0);for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){vec2 g=vec2(x,y);vec2 seed=0.5+0.36*sin(t*0.16+tau*vec2(hash21(id+g),hash21(id+g+19.0)));seed+=0.14*lowDrive()*sin(tau*vec2(hash21(id+g+7.0),hash21(id+g+3.0)))+0.1*midDrive()*cos(tau*vec2(hash21(id+g+9.0),hash21(id+g+4.0)));float d=length(g+seed-fract(q));if(d<d1){d2=d1;d1=d;chosen=id+g;}else d2=min(d2,d);}float edge=d2-d1;vec3 h=jewel(hash21(chosen)*0.8+t*0.006);vec3 c=h*(0.25+0.85*noise3(vec3(q*4.0,t*0.04)))*smoothstep(0.015,0.045,edge);c+=vec3(0.6,0.8,1)*0.10*glow(edge-0.055,0.012+0.03*highDrive());color=finishScene(c*1.6,p);}
