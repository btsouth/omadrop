#include "audio_controls.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <projectM-4/projectM.h>
#include <cassert>
#include <iostream>
#include <limits>
extern "C" void projectm_set_omadrop_audio(void*, const float*, unsigned);
int main() {
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    auto* w=SDL_CreateWindow("pilot bridge test",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    assert(w); auto c=SDL_GL_CreateContext(w); assert(c);
    glewExperimental=GL_TRUE; assert(glewInit()==GLEW_OK);
    auto a=projectm_create(), b=projectm_create(); assert(a&&b);
    const char* preset=R"(MILKDROP_PRESET_VERSION=201
PSVERSION=2
[preset00]
fRating=3
fDecay=1
fGammaAdj=1
fWaveAlpha=0
per_frame_1=q1=om_mix*om_low; q2=om_mid; q3=om_high;
comp_1=`shader_body { ret = float3(q1,q2,q3); }
)";
    for(auto p:{a,b}) {projectm_set_window_size(p,64,64); projectm_set_preset_locked(p,true); projectm_load_preset_data(p,preset,false);}
    auto pixel=[&](auto p) {
        glBindFramebuffer(GL_FRAMEBUFFER,0); glViewport(0,0,64,64);
        projectm_opengl_render_frame(p);
        glBindFramebuffer(GL_READ_FRAMEBUFFER,0); glReadBuffer(GL_BACK);
        std::array<unsigned char,4> px{}; glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px.data());return px;
    };
    std::array<float,8> values{1,1,0,0,0,0,0,0};
    projectm_set_omadrop_audio(a,values.data(),8);
    auto red=pixel(a); std::cerr << "pixel " << int(red[0]) << "," << int(red[1]) << "," << int(red[2]) << "\n"; assert(red[0]>245 && red[1]<5 && red[2]<5);
    auto black=pixel(b); assert(black[0]<5&&black[1]<5&&black[2]<5);
    values={1,std::numeric_limits<float>::quiet_NaN(),2,-2,0,0,0,0};
    projectm_set_omadrop_audio(a,values.data(),8);
    auto green=pixel(a); assert(green[0]<5&&green[1]>245&&green[2]<5);
    std::string rolePreset=preset;
    const auto rolePos=rolePreset.find("q1=om_mix*om_low; q2=om_mid; q3=om_high;");
    rolePreset.replace(rolePos, std::string("q1=om_mix*om_low; q2=om_mid; q3=om_high;").size(),
        "q1=om_attack; q2=om_tone; q3=om_air;");
    projectm_load_preset_data(a,rolePreset.c_str(),false);
    values={1,0,0,0,.25f,.5f,.75f,0};
    projectm_set_omadrop_audio(a,values.data(),8);
    auto roles=pixel(a);
    assert(std::abs(int(roles[0])-64)<3 && std::abs(int(roles[1])-128)<3 && std::abs(int(roles[2])-191)<3);
    projectm_destroy(a);projectm_destroy(b);SDL_GL_DeleteContext(c);SDL_DestroyWindow(w);SDL_Quit();
    std::cout<<"Bridge: real evaluator/shader input, independent instances, bounds and NaN passed\n";
}
