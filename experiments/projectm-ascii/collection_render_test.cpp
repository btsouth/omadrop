#include "native_renderer.h"
#include "live_compositor.h"
#include "live_compositor_shaders.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 assert(SDL_Init(SDL_INIT_VIDEO)==0);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
 auto window=SDL_CreateWindow("Omadrop collection verification",0,0,960,540,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
 assert(window);auto context=SDL_GL_CreateContext(window);assert(context);
 glewExperimental=GL_TRUE;assert(glewInit()==GLEW_OK);while(glGetError()!=GL_NO_ERROR){}
 NativeRenderer renderer;LiveCompositor compositor;std::string error;
 if(!renderer.initialize(argv[1],error)||!compositor.initialize(LiveCompositorShaders::vertexSource,LiveCompositorShaders::fragmentSource,error)){std::cerr<<error<<'\n';return 1;}
 MusicFrame music;music.energyFast=0.45f;music.energySlow=0.4f;music.harmonic=0.5f;
 std::vector<float> pixels(960*540*4);
 for(std::size_t index=collectionFirstScene;index<nativeSceneCount;index++) {
  NativeSceneState scene;scene.currentScene=static_cast<NativeSceneKind>(index);
  scene.incomingScene=static_cast<NativeSceneKind>(collectionFirstScene+(index+1-collectionFirstScene)%(nativeSceneCount-collectionFirstScene));
  const auto begin=std::chrono::steady_clock::now();
  for(int frame=0;frame<24;frame++) {
   scene.transitioning=frame>=12;scene.transition=frame>=12?(frame-12)/11.0f:0.0f;
   scene.transitionStyle=nativeTransitionStyle(scene.currentScene,scene.incomingScene);
   if(!renderer.render(music,scene,960,540,{0.44f,0.7f,1.0f},0,1.0f,1.0f/60.0f,error)){std::cerr<<error<<'\n';return 1;}
   LiveCompositorFrame display;display.width=960;display.height=540;display.nativeRenderer=true;
   display.sourceTexture=renderer.texture(scene.currentScene);display.nextTexture=renderer.texture(scene.transitioning?scene.incomingScene:scene.currentScene);
   display.sceneMix=scene.transition;display.transitionMode=static_cast<int>(scene.transitionStyle);display.fieldExposure=1.65f;
   const auto& source=nativeSceneDefinition(scene.currentScene).transitionGeometry;
   const auto& incoming=nativeSceneDefinition(scene.incomingScene).transitionGeometry;
   display.sourceTransitionAnchor=source.focalPoint;display.incomingTransitionAnchor=incoming.focalPoint;
   display.sourceTransitionMotion=source.motionVector;display.incomingTransitionMotion=incoming.motionVector;
   display.sourceTransitionDepth=source.depthStrength;display.incomingTransitionDepth=incoming.depthStrength;
   assert(compositor.render(display,error));
   glReadPixels(0,0,960,540,GL_RGBA,GL_FLOAT,pixels.data());
   double total=0.0;float peak=0.0f;
   for(std::size_t i=0;i<pixels.size();i++)if(i%4!=3){assert(std::isfinite(pixels[i]));total+=pixels[i];peak=std::max(peak,pixels[i]);}
   assert(total/(960*540*3)>0.002 && peak>0.1f);
  }
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()/24.0;
  std::cout<<nativeSceneName(scene.currentScene)<<": render, readback and checks "<<ms<<" ms/frame\n";
 }
 compositor.shutdown();renderer.shutdown();SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
}
