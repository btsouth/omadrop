#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

float mapWorld(vec3 p){float scale=1.0;for(int i=0;i<5;i++){p=abs(p)-vec3(0.8+0.020*lowDrive(),0.65+0.015*midDrive(),0.7+0.006*highDrive());p.xy=rotate2d(0.5+0.12*sin(clockTime()*0.11))*p.xy;p.xz=rotate2d(0.8)*p.xz;float k=1.5/clamp(dot(p,p),0.35,1.2);p*=k;scale*=k;}return length(p)/scale-0.018;}
void main(){vec2 p=canvas();float t=clockTime();vec3 ro=vec3(2.9*sin(t*0.08),0.7,2.9*cos(t*0.08));vec3 fw=normalize(-ro),rt=normalize(cross(fw,vec3(0,1,0))),up=cross(rt,fw);vec3 rd=normalize(fw*1.9+rt*p.x+up*p.y);float z=0.0,trap=0.0;vec3 c=vec3(0.006,0.013,0.03);
 for(int i=0;i<48;i++){vec3 q=ro+rd*z;float d=mapWorld(q);trap+=0.006*exp(-z*0.45)/(0.04+abs(d));if(d<0.003){float e=0.003;vec3 n=normalize(vec3(mapWorld(q+vec3(e,0,0))-mapWorld(q-vec3(e,0,0)),mapWorld(q+vec3(0,e,0))-mapWorld(q-vec3(0,e,0)),mapWorld(q+vec3(0,0,e))-mapWorld(q-vec3(0,0,e))));float l=max(dot(n,normalize(vec3(0.4,0.8,-0.6))),0.0);c+=metal(0.5+0.5*sin(length(q)*3.0+t*0.05))*(0.16+1.7*l)*exp(-z*0.16);break;}z+=max(d*0.65,0.008);if(z>8.0)break;}
 c+=vec3(0.03,0.17,0.3)*trap;color=finishScene(c,p);}
