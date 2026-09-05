#include "audio_controls.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
static MusicFrame music;
static void feed(PilotAudioControls& c, double hz, float amp, int hops, bool inverted=false) {
    std::vector<float> pcm(1470);
    for(int h=0;h<hops;++h) {
        for(int i=0;i<735;++i) {
            const float x=amp*std::sin(6.283185307179586*hz*(h*735+i)/44100);
            pcm[2*i]=x; pcm[2*i+1]=inverted?-x:x;
        }
        c.processStereo(pcm.data(),735,music); c.update(music,1.f/60,true);
    }
}
int main() {
    music.harmonic=.8; music.spectralCentroid=.7;
    for(auto [hz,index]:{std::pair{60.,1}, {1000.,2}, {8000.,3}}) {
        PilotAudioControls c, opposite;
        feed(c,hz,.1,120); feed(opposite,hz,.1,120,true);
        std::cout<<hz<<" bands "<<c.values[1]<<","<<c.values[2]<<","<<c.values[3]<<"\n";
        assert(c.values[index]>.65);
        for(int b=1;b<=3;++b) if(b!=index) assert(c.values[index]>c.values[b]*2);
        for(int b=1;b<8;++b) assert(std::abs(c.values[b]-opposite.values[b])<1e-6);
        feed(c,hz,0,60); for(int b=1;b<8;++b) assert(c.values[b]<.01);
    }
    PilotAudioControls c;
    feed(c,60,.0001,120); for(int b=1;b<8;++b) assert(c.values[b]==0);
    c.resetAudio(); feed(c,60,.1,3); assert(c.values[1]>.5); // control response within 50 ms of PCM
    feed(c,60,.1,120); const float loud=c.values[1];
    feed(c,60,.025,12); assert(c.values[1]<loud*.75); // preserve a short musical decrease
    for(int i=0;i<60;++i)c.update(music,1.f/60,false);
    for(float v:c.values)assert(v<.001); // stale capture and comparison fade
    c.resetAudio(); assert(c.rms==0 && c.reference==.04f);
    std::vector<float> bad(1470,std::numeric_limits<float>::quiet_NaN()); bad[1]=std::numeric_limits<float>::infinity();
    c.processStereo(bad.data(),735,music);c.update(music,1.f/60,true);
    for(float v:c.values)assert(std::isfinite(v)&&v>=0&&v<=1);
    std::cout<<"Stereo roles, noise gate, control attack, dynamics, silence, stale capture, reset, finite bounds passed\n";
}
