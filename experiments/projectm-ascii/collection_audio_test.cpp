#include "native_renderer.h"
#include "audio_features.h"
#include "musical_structure.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
int main(int argc,char** argv) {
 if(argc!=3)return 2;
 assert(SDL_Init(SDL_INIT_VIDEO)==0);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
 auto window=SDL_CreateWindow("Collection audio regression",0,0,320,180,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(window);auto context=SDL_GL_CreateContext(window);assert(context);
 glewExperimental=GL_TRUE;assert(glewInit()==GLEW_OK);while(glGetError()!=GL_NO_ERROR){}
 NativeRenderer renderer;std::string error;if(!renderer.initialize(argv[1],error)){std::cerr<<error<<'\n';return 1;}
 AudioFeatureBus bus;MusicalStructureTracker structure;MusicFrameBuilder builder;
 std::ifstream input(argv[2],std::ios::binary);assert(input);std::vector<float> pcm(AudioFeatureBus::hopSize*2);MusicFrame real;
 // Use the strongest measured low-frequency body in this actual recording.
 float strongest=-1;
 for(int i=0;i<240 && input.read(reinterpret_cast<char*>(pcm.data()),pcm.size()*sizeof(float));i++) {
  auto features=bus.processStereo(pcm.data(),AudioFeatureBus::hopSize);
  auto frame=builder.update(features,structure.update(features),1.0f/60.0f,0.0f,-1.0f);
  if(frame.bassBody>strongest){strongest=frame.bassBody;real=frame;}
 }
 assert(strongest>0.001f);
 auto render=[&](NativeSceneKind kind,MusicFrame music) {
  renderer.reset();NativeSceneState scene;scene.currentScene=kind;
  // Equalize clock inputs and freeze its phase. Differences must come from the
  // musical uniforms, not energy-driven animation speed or different times.
  music.energyFast=0;music.energySlow=0;
  for(int i=0;i<45;i++) {renderer.synchronizeFlowTime(5.0f);assert(renderer.render(music,scene,320,180,{0.44f,0.7f,1.0f},0,1.0f,1.0f/60.0f,error));}
  std::vector<float> pixels(320*180*4);glBindTexture(GL_TEXTURE_2D,renderer.texture(kind));glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_FLOAT,pixels.data());return pixels;
 };
 MusicFrame low;low.bassBody=0.3f;low.kick=0.7f;
 MusicFrame mid;mid.bandLevel[2]=0.6f;mid.bandLevel[3]=0.5f;mid.snare=0.5f;
 MusicFrame high;high.bandLevel[4]=0.5f;high.bandLevel[5]=0.4f;high.hat=0.5f;
 bool passed=true;
 for(std::size_t i=collectionFirstScene;i<nativeSceneCount;i++) {
  auto kind=static_cast<NativeSceneKind>(i);auto baseline=render(kind,{});std::cout<<nativeSceneName(kind);
  for(const auto& music:{low,mid,high,real}) {auto pixels=render(kind,music);double difference=0;
   for(std::size_t j=0;j<pixels.size();j++)if(j%4!=3){assert(std::isfinite(pixels[j]));difference+=std::abs(pixels[j]-baseline[j]);}
   difference/=320*180*3;std::cout<<' '<<difference;passed&=difference>0.000001;
  }std::cout<<'\n';
 }
 renderer.shutdown();SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();
 std::cout<<"columns: low middle high actual-music; frozen animation phase\n";return passed?0:1;
}
