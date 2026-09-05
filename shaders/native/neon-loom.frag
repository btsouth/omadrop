#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec3 c=vec3(0.003,0.006,0.025);for(int i=0;i<42;i++){float f=float(i)/42.0;float phase=f*tau;vec2 q=rotate2d(0.3*sin(t*0.07))*p;float y=0.6*sin(q.x*1.6+t*0.3+phase)+0.25*sin(q.x*3.1-t*0.17+phase*2.0);y+=0.36*lowDrive()*sin(phase+q.x*2.0);y+=0.18*midDrive()*sin(q.x*4.0+phase*2.0)+0.10*noteCurve(q.x+phase);float d=q.y-y;vec3 h=jewel(f*0.65+t*0.007);c+=h*(0.055*glow(d,0.025)+0.11*stroke(d,(0.0014+0.003*highDrive()*(0.5+0.5*sin(phase*3.0)))));}color=finishScene(c,p);}
