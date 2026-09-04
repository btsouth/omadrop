#include "arrangement_classifier.h"

#include <cassert>
#include <iostream>

namespace {
ArrangementObservation passage(int bar, float energy) {
    ArrangementObservation observation;
    observation.barIndex = bar;
    observation.musicActive = true;
    observation.energyFast = energy;
    observation.energySlow = energy;
    observation.percussive = 0.30f;
    observation.rhythmicDensity = 0.28f;
    return observation;
}

void sustain(ArrangementClassifier& classifier,
             ArrangementObservation observation, float seconds) {
    const int frames = static_cast<int>(seconds * 60.0f);
    for (int frame = 0; frame < frames; ++frame) {
        observation.barAnalyzed = false;
        classifier.update(observation, 1.0f / 60.0f);
    }
}
}

int main() {
    ArrangementClassifier classifier;
    const ArrangementState silence = classifier.update({}, 1.0f / 60.0f);
    assert(silence.role == ArrangementRole::Unknown);
    assert(!silence.changed);

    ArrangementObservation intro = passage(0, 0.18f);
    intro.trackProgress = 0.01f;
    const ArrangementState started = classifier.update(intro, 1.0f / 60.0f);
    assert(started.role == ArrangementRole::Intro);
    assert(started.changed);
    assert(arrangementRoleName(started.role) == "intro");
    assert(!classifier.update(intro, 1.0f / 60.0f).changed);

    ArrangementClassifier midSongClassifier;
    ArrangementObservation midSong = passage(18, 0.40f);
    midSong.trackProgress = 0.52f;
    midSong.barAnalyzed = true;
    assert(midSongClassifier.update(midSong, 1.0f / 60.0f).role
           == ArrangementRole::Verse);

    ArrangementObservation verse = passage(4, 0.34f);
    verse.trackProgress = 0.12f;
    verse.barAnalyzed = true;
    assert(classifier.update(verse, 1.0f / 60.0f).role
           == ArrangementRole::Verse);

    ArrangementObservation chorus = passage(12, 0.60f);
    chorus.trackProgress = 0.34f;
    chorus.barAnalyzed = true;
    chorus.sectionCrossed = true;
    chorus.motifRecalled = true;
    assert(classifier.update(chorus, 1.0f / 60.0f).role
           == ArrangementRole::Chorus);

    classifier.reset();
    ArrangementObservation baseline = passage(8, 0.42f);
    baseline.barAnalyzed = true;
    baseline.sectionCrossed = true;
    classifier.update(baseline, 1.0f / 60.0f);
    ArrangementObservation novel = passage(20, 0.40f);
    novel.sectionCrossed = true;
    novel.barAnalyzed = true;
    novel.novelty = 0.21f;
    novel.noveltyThreshold = 0.12f;
    novel.harmonicChange = 0.14f;
    assert(classifier.update(novel, 1.0f / 60.0f).role
           == ArrangementRole::Bridge);

    classifier.reset();
    baseline = passage(8, 0.48f);
    baseline.barAnalyzed = true;
    classifier.update(baseline, 1.0f / 60.0f);
    ArrangementObservation low = passage(12, 0.16f);
    low.percussive = 0.12f;
    sustain(classifier, low, 2.6f);
    low.barAnalyzed = true;
    assert(classifier.update(low, 1.0f / 60.0f).role
           == ArrangementRole::Breakdown);

    classifier.reset();
    baseline = passage(8, 0.34f);
    baseline.barAnalyzed = true;
    classifier.update(baseline, 1.0f / 60.0f);
    ArrangementObservation build = passage(12, 0.49f);
    build.energyFast = 0.53f;
    build.energySlow = 0.45f;
    build.energySlope = 0.08f;
    sustain(classifier, build, 2.1f);
    build.barAnalyzed = true;
    assert(classifier.update(build, 1.0f / 60.0f).role
           == ArrangementRole::Build);

    classifier.reset();
    baseline = passage(8, 0.32f);
    baseline.barAnalyzed = true;
    classifier.update(baseline, 1.0f / 60.0f);
    ArrangementObservation peak = passage(12, 0.72f);
    peak.percussive = 0.70f;
    peak.rhythmicDensity = 0.64f;
    sustain(classifier, peak, 2.6f);
    peak.barAnalyzed = true;
    assert(classifier.update(peak, 1.0f / 60.0f).role
           == ArrangementRole::Peak);

    ArrangementObservation outro = passage(28, 0.18f);
    outro.trackProgress = 0.93f;
    outro.energySlope = -0.06f;
    outro.barAnalyzed = true;
    assert(classifier.update(outro, 1.0f / 60.0f).role
           == ArrangementRole::Outro);

    // A transient cannot relabel a passage because only analyzed bars count.
    ArrangementClassifier transientClassifier;
    transientClassifier.update(intro, 1.0f / 60.0f);
    ArrangementObservation hit = passage(2, 0.95f);
    hit.percussive = 1.0f;
    hit.rhythmicDensity = 1.0f;
    hit.energySlope = 2.0f;
    assert(transientClassifier.update(hit, 1.0f / 60.0f).role
           == ArrangementRole::Intro);

    std::cout << "arrangement classifier passed\n";
}
