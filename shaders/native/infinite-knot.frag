#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"
void main(){vec2 p=canvas();float t=clockTime();vec3 c=vec3(0.008,0.008,0.024);vec2 prev=vec2(0);float nearest=9.0;vec3 surface=vec3(0);mat2 turn=rotate2d(0.7+t*0.06),turn2=rotate2d(t*0.04);
 for(int i=0;i<160;i++){
  float u=float(i)*tau/159.0;vec3 v=vec3((1.0+0.34*cos(3.0*u+t*0.12))*cos(2.0*u),(1.0+0.34*cos(3.0*u+t*0.12))*sin(2.0*u),0.6*sin(3.0*u+t*0.12));
  v.xy+=0.18*lowDrive()*vec2(cos(3.0*u),sin(3.0*u));v.z+=0.22*midDrive()*sin(5.0*u)+0.1*noteCurve(u);v.yz=turn*v.yz;v.xz=turn2*v.xz;vec2 pt=projectPoint(v)*2.8;
  float d=segment(p,prev,pt),w=(0.033+0.014*highDrive())/(1.0+v.z*0.18);float signedD=d-w;
  if(i>0){vec3 h=jewel(u/tau*0.45+t*0.007);c+=h*glow(d,0.055)*0.011;
   if(signedD<nearest){nearest=signedD;float crossSection=sqrt(max(0.0,1.0-pow(d/w,2.0)));surface=h*(0.13+0.9*crossSection)+vec3(0.8,0.86,1.0)*pow(crossSection,14.0)*0.5;}
  }prev=pt;
 }c=mix(c,surface,1.0-smoothstep(-0.002,0.002,nearest));color=finishScene(c,p);}
