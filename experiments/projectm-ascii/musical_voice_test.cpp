#include "audio_features.h"
#include "music_frame.h"
#include "musical_motion.h"
#include "paired_music_state.h"
#include <cassert>
#include <iostream>
#include <limits>

AudioFeatures tone(float hz,float amplitude) {
    AudioFeatureBus bus;
    std::vector<float> pcm(AudioFeatureBus::hopSize);
    AudioFeatures features;
    for(int frame=0;frame<240;++frame) {
        for(int i=0;i<AudioFeatureBus::hopSize;++i)
            pcm[i]=amplitude*std::sin(6.28318530718f*hz*
                (frame*AudioFeatureBus::hopSize+i)/AudioFeatureBus::sampleRate);
        features=bus.processMono(pcm.data(),pcm.size());
    }
    return features;
}
int main() {
    auto soft=tone(60,.08f),loud=tone(60,.4f),mid=tone(660,.4f),other=tone(1100,.4f),quiet=tone(60,0);
    // A sustained louder bass remains visibly stronger after adaptation.
    assert(loud.bassBody>soft.bassBody*2.0f);
    assert(loud.bassBody>mid.bassBody*10.0f);
    assert(quiet.bassBody==0);
    float difference=0,melody=0;
    for(int i=0;i<32;++i) {
        assert(std::isfinite(mid.harmonicShape[i]));
        assert(mid.harmonicShape[i]>=0 && mid.harmonicShape[i]<=1);
        assert(quiet.harmonicShape[i]==0);
        difference+=std::abs(mid.harmonicShape[i]-other.harmonicShape[i]);
        melody+=mid.harmonicShape[i];
    }
    assert(difference>1.0f && melody>0.5f);
    MusicFrameBuilder builder;
    auto frame=builder.update(mid,{},1.0f/60);
    assert(frame.harmonicShape==mid.harmonicShape);
    MusicalMotion motion;
    for(int i=0;i<120;++i)motion.update(frame,1.0f/60);
    auto held=motion.update(frame,1.0f/60);
    assert(std::abs(held.harmonicShape[19]-frame.harmonicShape[19])<.001f);
    PairedMusicState packet;packet.serial=1;packet.frame=frame;packet.frame.bassBody=loud.bassBody;
    auto decoded=decodePairedMusicState(encodePairedMusicState(packet));
    assert(decoded && decoded->frame.harmonicShape==frame.harmonicShape);
    assert(decoded->frame.bassBody==loud.bassBody);
    packet.frame.harmonicShape[3]=std::numeric_limits<float>::quiet_NaN();
    assert(!decodePairedMusicState(encodePairedMusicState(packet)));
    std::cout<<"Sustained bass level, tonal distinction, silence, smoothing and paired transport passed\n";
}
