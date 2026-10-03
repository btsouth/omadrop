#include "score.h"

#include <algorithm>
#include <cmath>

namespace Journey {
void Score::build(const AudioTimeline& audio, double duration, bool hasAudio) {
    onsets.clear(); bassHits.clear(); midPeaks.clear(); surges.clear(); bandIntegral.clear();
    if (!hasAudio) return;
    constexpr double dt = 1.0 / 120;
    const int n = int(std::ceil(duration / dt)) + 1;
    std::vector<Audio> s(n);
    for (int i = 0; i < n; ++i) s[i] = audio.at(i * dt);
    bandIntegral.resize(n);
    std::array<double, 6> acc{};
    for (int i = 0; i < n; ++i) {
        for (int b = 0; b < 6; ++b) { bandIntegral[i][b] = float(acc[b]); acc[b] += s[i].bands[b] * dt; }
    }
    double lastOnset = -1, lastBass = -1, lastMid = -1, midMean = 0, bassMean = 0;
    bool surgeArmed = true;
    for (int i = 1; i + 1 < n; ++i) {
        const double t = i * dt;
        const Audio& a = s[i];
        bassMean += (a.bass - bassMean) * 0.004;
        midMean += (a.bands[3] - midMean) * 0.01;
        // Onset: a local accent maximum standing clearly above the trough of
        // the previous 0.2 s (busy mixes never let the envelope fall to zero).
        if (a.accent >= s[i - 1].accent && a.accent > s[i + 1].accent && t - lastOnset > 0.16) {
            double trough = a.accent;
            for (int k = std::max(0, i - 24); k < i; ++k) trough = std::min(trough, s[k].accent);
            const double prominence = a.accent - trough;
            if (prominence > 0.045) {
                onsets.push_back({t, std::clamp(prominence * 7.0, 0.0, 1.0)});
                lastOnset = t;
            }
        }
        // Bass hit: a fast rise of the bass body above its running mean.
        const double rise = a.bass - s[std::max(0, i - 6)].bass;
        if (rise > 0.07 && a.bass > bassMean * 1.12 && a.bass > 0.25 && t - lastBass > 0.30) {
            bassHits.push_back({t, std::clamp(a.bass, 0.0, 1.0)});
            lastBass = t;
        }
        if (a.bands[3] >= s[i - 1].bands[3] && a.bands[3] > s[i + 1].bands[3]
            && a.bands[3] > midMean * 1.08 && a.bands[3] > 0.12 && t - lastMid > 0.22) {
            midPeaks.push_back({t, std::clamp(a.bands[3], 0.0, 1.0)});
            lastMid = t;
        }
        if (a.surge < 0.12) surgeArmed = true;
        if (surgeArmed && a.surge > 0.25) {
            surges.push_back({t, a.surge});
            surgeArmed = false;
        }
    }
}

double Score::integral(int band, double t) const {
    if (bandIntegral.empty()) return 0;
    const double x = std::max(0.0, t * 120);
    const std::size_t i = std::min(bandIntegral.size() - 1, std::size_t(x));
    const std::size_t j = std::min(bandIntegral.size() - 1, i + 1);
    const double f = std::min(1.0, x - double(i));
    return bandIntegral[i][band] + f * (bandIntegral[j][band] - bandIntegral[i][band]);
}

int Score::count(const std::vector<Event>& list, double from, double to) {
    int c = 0;
    for (const Event& e : list) if (e.t >= from && e.t < to) ++c;
    return c;
}

const Event* Score::last(const std::vector<Event>& list, double t) {
    auto it = std::upper_bound(list.begin(), list.end(), t, [](double v, const Event& e) { return v < e.t; });
    if (it == list.begin()) return nullptr;
    return &*(it - 1);
}

const Event* Score::first(const std::vector<Event>& list, double from, double to) {
    for (const Event& e : list) if (e.t >= from && e.t < to) return &e;
    return nullptr;
}
}
