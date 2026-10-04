#pragma once
#include "art.h"
#include "score.h"
#include <array>
#include <deque>

namespace Journey {
enum class Moment { Train, Cyclist, Tea, Cook, Gust, Toast, Star, Count };
struct Recurrence {
    double start=0, next=0, duration=0;
    std::uint64_t cycle=0;
};
struct Schedule {
    explicit Schedule(int seed=1);
    void advance(double now, const Audio& a, const Score& score);
    double age(Moment m, double now) const { return now-moments[int(m)].start; }
    double action(Moment m,double now,double authoredStart) const { return authoredStart+age(m,now)*parameter(m,0,0.85,1.15,1); }
    double parameter(Moment m,double key,double lo,double hi,double original) const;
    double combinationAt=0;
    std::uint64_t combinations=0;
    double gesture(double now,double authoredStart,double in,double end,double out) const;
    double pane(double now,double key) const;
    std::array<Recurrence,int(Moment::Count)> moments{};
    std::array<double,15> birdLand{}, birdReturn{};
    double fireworks=-1, fireworkStrength=0, fireworkReady=10, fullFireworkReady=10;
    bool fullFireworkShow = true;
    std::deque<Event> finale;
private:
    int seed_;
    std::uint64_t eventSerial_=0, fireworksCount_=0;
    double cyclePhase(double now,double key,double phaseKey,double lo,double hi,bool pane) const;
    double varied(double key,std::uint64_t cycle,double lo,double hi) const;
};
}
