#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"

float field(vec3 p) {
 p.xy=rotate2d(0.3*sin(clockTime()*0.09))*p.xy;
 p=abs(mod(p+1.0,2.0)-1.0);
 return min(length(p.xy)-(0.045+0.085*lowDrive()),min(length(p.yz)-(0.045+0.06*midDrive()),length(p.xz)-(0.045+0.05*highDrive())));
}
void main(){vec2 p=canvas();float t=clockTime();vec3 ro=vec3(0.45+0.2*sin(t*0.13),0.48+0.12*cos(t*0.1),t*0.2);vec3 rd=normalize(vec3(p,1.6));vec3 c=vec3(0.007,0.014,0.035);float z=0.0;
 for(int i=0;i<48;i++){vec3 q=ro+rd*z;float d=field(q);vec3 hue=mix(vec3(0.12,0.55,1.0),vec3(1.0,0.52,0.18),0.5+0.5*sin(q.z*0.5+t*0.12));c+=hue*0.008*exp(-z*0.15)/(0.018+abs(d));if(d<0.008){c+=hue*exp(-z*0.16)*0.8;break;}z+=max(abs(d)*0.8,0.02);if(z>16.0)break;}
 color=finishScene(c*0.75,p);}
