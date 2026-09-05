#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

void main(){vec2 p=canvas();float t=clockTime();vec2 q=p*5.0;vec2 id=floor(q);float nearest=10.0,second=10.0;vec2 win=vec2(0);for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){vec2 g=vec2(x,y),cell=id+g;vec2 o=0.5+0.38*sin(t*0.15+tau*vec2(hash21(cell),hash21(cell+12.0)));o+=0.13*lowDrive()*sin(tau*vec2(hash21(cell+7.0),hash21(cell+3.0)))+0.1*midDrive()*cos(tau*vec2(hash21(cell+9.0),hash21(cell+4.0)));float d=length(g+o-fract(q));if(d<nearest){second=nearest;nearest=d;win=cell;}else second=min(second,d);}float e=second-nearest;vec3 h=jewel(hash21(win)*0.4+0.42+t*0.006);vec3 c=h*(0.2+0.6*pow(max(0.0,1.0-nearest),2.0));c+=mix(h,vec3(0.8,0.9,1),0.25)*0.35*glow(e-0.035,0.02+0.025*highDrive());c*=smoothstep(0.0,0.025,e)*0.8+0.2;color=finishScene(c*1.5,p);}
