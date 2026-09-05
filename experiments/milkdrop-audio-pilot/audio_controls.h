#pragma once
#include "music_frame.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

// Stereo, sample-driven expressive controls. These are frequency/timbre roles,
// not isolated instruments. PCM supplied to projectM is never modified.
// ABI: mix, low, mid, high, attack, tone, air, energy.
class PilotAudioControls {
    struct Biquad {
        double b0{}, b1{}, b2{}, a1{}, a2{}, z1{}, z2{};
        static Biquad make(double hz, bool highpass) {
            const double w=6.283185307179586*hz/44100.0;
            const double c=std::cos(w), alpha=std::sin(w)/1.4142135623730951;
            const double a0=1+alpha;
            Biquad f;
            f.b0=(highpass ? (1+c)/2 : (1-c)/2)/a0;
            f.b1=(highpass ? -(1+c) : 1-c)/a0;
            f.b2=f.b0; f.a1=-2*c/a0; f.a2=(1-alpha)/a0;
            return f;
        }
        double sample(double x) {
            const double y=b0*x+z1;
            z1=b1*x-a1*y+z2; z2=b2*x-a2*y;
            return y;
        }
    };
    struct Channel {
        Biquad low=Biquad::make(180,false);
        Biquad midHigh=Biquad::make(180,true), midLow=Biquad::make(3500,false);
        Biquad high=Biquad::make(3500,true);
        std::array<double,3> sample(double x) {
            return {low.sample(x),midLow.sample(midHigh.sample(x)),high.sample(x)};
        }
    };
    std::array<Channel,2> filters_{};
    std::array<float,8> target_{};
    std::array<float,3> envelope_{};
    float slowEnergy_=0, age_=0;
    bool primed_=false;
    static float finite(float x) { return std::isfinite(x) ? x : 0.0f; }
    static float smooth(float previous, float target, float rate, float dt) {
        return previous+(target-previous)*(-std::expm1(-rate*dt));
    }
public:
    std::array<float,8> values{};
    float rms=0, reference=.04f;
    void resetAudio() { *this=PilotAudioControls{}; }

    void processStereo(const float* pcm, std::size_t frames, const MusicFrame& music) {
        if (!pcm || frames==0) return;
        std::array<double,3> power{};
        double full=0;
        for (std::size_t i=0;i<frames;++i) {
            for (unsigned ch=0;ch<2;++ch) {
                const double x=std::clamp(finite(pcm[2*i+ch]),-4.0f,4.0f);
                full+=x*x;
                const auto band=filters_[ch].sample(x);
                for (unsigned b=0;b<3;++b) power[b]+=band[b]*band[b];
            }
        }
        const float dt=static_cast<float>(frames)/44100;
        age_=0;
        rms=static_cast<float>(std::sqrt(full/(2*frames)));
        // Absolute gate prevents quiet noise and filter tails becoming loud gestures.
        const float gate=std::clamp((rms-.00025f)/.0015f,0.0f,1.0f);
        if (!primed_ && rms>.002f) { reference=std::max(.008f,rms);primed_=true; }
        if (gate>0) reference=smooth(reference,std::max(.008f,rms),.25f,dt);
        const float denominator=std::max(.008f,reference);
        constexpr std::array<float,3> weight{1.15f,1.0f,1.5f};
        for (unsigned b=0;b<3;++b) {
            const float amplitude=static_cast<float>(std::sqrt(power[b]/(2*frames)));
            const float shaped=gate*(-std::expm1(-1.35f*weight[b]*amplitude/denominator));
            // One shared loudness reference retains the balance between frequency regions.
            envelope_[b]=smooth(envelope_[b],shaped,shaped>envelope_[b]?55.0f:12.0f,dt);
            target_[b+1]=envelope_[b];
        }
        const float energy=gate*(-std::expm1(-rms/denominator));
        const float attack=std::clamp((energy-slowEnergy_)*2.5f,0.0f,1.0f);
        slowEnergy_=smooth(slowEnergy_,energy,5.0f,dt);
        target_[4]=gate*attack;
        target_[5]=envelope_[1]*(.35f+.65f*std::clamp(finite(music.harmonic),0.0f,1.0f));
        target_[6]=envelope_[2]*(.5f+.5f*std::clamp(finite(music.spectralCentroid),0.0f,1.0f));
        target_[7]=slowEnergy_;
    }
    std::array<float,8> update(const MusicFrame&, float dt, bool enabled) {
        dt=std::clamp(finite(dt),0.0f,.1f);
        age_+=dt;
        target_[0]=enabled?1.0f:0.0f;
        // A lost capture cannot leave a stale held pose indefinitely.
        if(age_>.20f) for(unsigned i=1;i<8;++i) target_[i]=0;
        for(unsigned i=0;i<8;++i) {
            const float rate=i==0?10.0f:i==4?32.0f:60.0f;
            values[i]=smooth(values[i],target_[i],rate,dt);
            if(values[i]<.00001f) values[i]=0;
            values[i]=std::clamp(finite(values[i]),0.0f,1.0f);
        }
        return values;
    }
};
