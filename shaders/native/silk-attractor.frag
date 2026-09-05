#version 330 core
in vec2 uv;
out vec4 color;
#include "collection-common.glsl"
#include "collection-lorenz-path.glsl"
// A Lorenz trajectory, integrated as geometry rather than played as a preset.
void main(){
 vec2 p=canvas();float t=clockTime();vec3 c=vec3(0.006,0.009,0.024);
 vec2 previous=vec2(0);
 mat2 turn=rotate2d(0.38*sin(t*0.08));
 for(int i=0;i<240;i++){
  vec3 v=lorenzPath[i];
  vec3 point=vec3(v.x*0.068,(v.z-25.0)*0.065,v.y*0.045);
  point.x+=0.28*lowDrive()*sin(point.y*2.0);point.y+=0.18*midDrive()*sin(point.x*3.0)+0.1*noteCurve(point.x);point.xz=turn*point.xz;vec2 pt=projectPoint(point)*2.2;
  if(i>0){float d=segment(p,previous,pt);float f=float(i)/240.0;
   vec3 h=mix(vec3(0.12,0.5,0.9),vec3(1.0,0.56,0.2),f);
   float shimmer=0.7+0.3*sin(f*17.0-t*0.3);
   c+=h*(0.025*glow(d,0.023)+0.19*stroke(d,0.0015+0.003*highDrive()))*shimmer;
  }previous=pt;
 }color=finishScene(c,p);
}
