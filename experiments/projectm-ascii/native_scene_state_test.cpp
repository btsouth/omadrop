#include "native_scene_state.h"

#include <cassert>
#include <iostream>
#include <set>
#include <string>

namespace {
void finishTransition(NativeSceneDirector& director, const MusicFrame& music) {
    for (int frame = 0; frame < 360; ++frame) {
        if (!director.update(music, 1.0f / 60.0f).transitioning) return;
    }
    assert(false && "native scene transition did not finish");
}

NativeSceneKind automaticChoice(MusicFrame music) {
    NativeSceneDirector director;
    music.bpm = 120.0f;
    for (int frame = 0; frame < 520; ++frame) {
        director.update(music, 1.0f / 60.0f);
    }
    music.section = 1.0f;
    const NativeSceneState chosen = director.update(music, 1.0f / 60.0f);
    assert(chosen.transitioning);
    return chosen.incomingScene;
}

NativeSceneKind automaticChoiceFrom(NativeSceneKind current, MusicFrame music) {
    NativeSceneDirector director;
    director.selectScene(current);
    music.bpm = 120.0f;
    for (int frame = 0; frame < 520; ++frame) {
        director.update(music, 1.0f / 60.0f);
    }
    music.section = 1.0f;
    const NativeSceneState chosen = director.update(music, 1.0f / 60.0f);
    assert(chosen.transitioning);
    return chosen.incomingScene;
}

NativeSceneKind automaticChoiceWithProfile(NativeSceneKind current,
                                           MusicFrame music,
                                           NativeDirectorProfile profile) {
    NativeSceneDirector director;
    director.selectScene(current);
    director.setProfile(profile);
    music.bpm = 120.0f;
    for (int frame = 0; frame < 520; ++frame) {
        director.update(music, 1.0f / 60.0f);
    }
    music.section = 1.0f;
    const NativeSceneState chosen = director.update(music, 1.0f / 60.0f);
    assert(chosen.transitioning);
    return chosen.incomingScene;
}

NativeSceneKind automaticChoiceWithPreferences(
    NativeSceneKind current, MusicFrame music,
    const std::vector<std::string>& favorites,
    const std::vector<std::string>& hidden) {
    NativeSceneDirector director;
    director.selectScene(current);
    director.setScenePreferences(favorites, hidden);
    music.bpm = 120.0f;
    for (int frame = 0; frame < 520; ++frame) {
        director.update(music, 1.0f / 60.0f);
    }
    music.section = 1.0f;
    const NativeSceneState chosen = director.update(music, 1.0f / 60.0f);
    assert(chosen.transitioning);
    return chosen.incomingScene;
}
}

int main() {
    assert(nativeSceneRegistryVersion == 14);
    std::set<std::string> sceneSlugs;
    std::set<std::string> sceneShaders;
    for (std::size_t index = 0; index < nativeSceneRegistry.size(); ++index) {
        const NativeSceneDefinition& definition = nativeSceneRegistry[index];
        assert(static_cast<std::size_t>(definition.kind) == index);
        assert(sceneSlugs.insert(std::string(definition.slug)).second);
        assert(sceneShaders.insert(std::string(definition.shader)).second);
        assert((definition.musicalRoles & transientRoles) == transientRoles);
        assert(definition.maximumQuietMotionCoverage > 0.0f);
        assert(definition.maximumQuietMotionCoverage <= 0.50f);
        assert(definition.maximumGlobalPulse > 0.0f);
        assert(definition.maximumGlobalPulse <= 0.70f);
        assert(definition.maximumFrameMilliseconds > 0.0f);
        const NativeTransitionGeometry& geometry
            = definition.transitionGeometry;
        assert(geometry.focalPoint[0] >= 0.0f
            && geometry.focalPoint[0] <= 1.0f);
        assert(geometry.focalPoint[1] >= 0.0f
            && geometry.focalPoint[1] <= 1.0f);
        const float motionLength = std::hypot(
            geometry.motionVector[0], geometry.motionVector[1]);
        assert(motionLength > 0.90f && motionLength < 1.10f);
        assert(geometry.depthStrength >= 0.0f
            && geometry.depthStrength <= 1.0f);
        if (definition.transitionAnchor
            == NativeTransitionAnchor::HorizontalAxis) {
            assert(std::abs(geometry.motionVector[0])
                > std::abs(geometry.motionVector[1]));
        } else if (definition.transitionAnchor
                   == NativeTransitionAnchor::VerticalAxis) {
            assert(std::abs(geometry.motionVector[1])
                > std::abs(geometry.motionVector[0]));
        } else if (definition.transitionAnchor
                   == NativeTransitionAnchor::DepthPoint) {
            assert(geometry.depthStrength >= 0.80f);
        }
    }
    assert(nativeSceneMotionGrammarCount(NativeMotionGrammar::Flow) == 2);
    assert(nativeSceneMotionGrammarCount(NativeMotionGrammar::Sparse) == 6);
    assert(nativeSceneMotionGrammarCount(NativeMotionGrammar::Selective) == 10);
    assert(nativeSceneVisualFamilyCount(NativeVisualFamily::Radial) == 4);
    assert(nativeSceneVisualFamilyCount(NativeVisualFamily::Filament) == 3);
    assert(nativeSceneVisualFamilyCount(NativeVisualFamily::Depth) == 2);
    assert(nativeSceneVisualFamilyCount(NativeVisualFamily::Vertical) == 2);
    assert(nativeSceneVisualFamilyCount(NativeVisualFamily::Landscape) == 2);
    NativeSceneKind parsedScene = NativeSceneKind::DepthTunnel;
    assert(nativeSceneFromName("wire", parsedScene));
    assert(parsedScene == NativeSceneKind::WireOrganism);
    assert(nativeSceneFromName("prism-garden", parsedScene));
    assert(parsedScene == NativeSceneKind::PrismGarden);
    assert(nativeSceneFromName("constellation", parsedScene));
    assert(parsedScene == NativeSceneKind::ConstellationField);
    assert(nativeSceneFromName("bloom", parsedScene));
    assert(parsedScene == NativeSceneKind::BloomEngine);
    assert(nativeSceneFromName("void", parsedScene));
    assert(parsedScene == NativeSceneKind::NegativeSpace);
    assert(nativeSceneFromName("ink", parsedScene));
    assert(parsedScene == NativeSceneKind::InkCurrent);
    assert(nativeSceneFromName("choir", parsedScene));
    assert(parsedScene == NativeSceneKind::GlassChoir);
    assert(nativeSceneFromName("architecture", parsedScene));
    assert(parsedScene == NativeSceneKind::ShadowArchitecture);
    assert(nativeSceneFromName("weave", parsedScene));
    assert(parsedScene == NativeSceneKind::ParticleWeave);
    assert(nativeSceneFromName("mosaic", parsedScene));
    assert(parsedScene == NativeSceneKind::LivingMosaic);
    assert(nativeSceneFromName("lumen", parsedScene));
    assert(parsedScene == NativeSceneKind::LumenFold);
    assert(nativeSceneFromName("paper", parsedScene));
    assert(parsedScene == NativeSceneKind::PaperHorizon);
    assert(nativeTransitionStyle(NativeSceneKind::NegativeSpace,
        NativeSceneKind::GlassChoir)
        == NativeTransitionStyle::NegativeSpaceReveal);
    assert(nativeTransitionStyle(NativeSceneKind::InkCurrent,
        NativeSceneKind::GlassChoir)
        == NativeTransitionStyle::ControlledFracture);
    assert(nativeTransitionStyle(NativeSceneKind::GlassChoir,
        NativeSceneKind::ShadowArchitecture)
        == NativeTransitionStyle::DepthTravel);
    assert(nativeTransitionStyle(NativeSceneKind::InkCurrent,
        NativeSceneKind::Centrifuge)
        == NativeTransitionStyle::FlowCarry);
    assert(nativeTransitionStyle(NativeSceneKind::Centrifuge,
        NativeSceneKind::BloomEngine)
        == NativeTransitionStyle::FocalMorph);
    NativeTransitionContext calmTransition;
    calmTransition.energy = 0.20f;
    calmTransition.rhythmicDensity = 0.10f;
    assert(nativeTransitionStyle(NativeSceneKind::PrismGarden,
        NativeSceneKind::OrbitalLoom, calmTransition)
        == NativeTransitionStyle::NegativeSpaceReveal);
    NativeTransitionContext harmonicTransition;
    harmonicTransition.energy = 0.52f;
    harmonicTransition.harmonic = 0.78f;
    harmonicTransition.harmonicChange = 0.55f;
    assert(nativeTransitionStyle(NativeSceneKind::PrismGarden,
        NativeSceneKind::OrbitalLoom, harmonicTransition)
        == NativeTransitionStyle::ControlledFracture);
    NativeTransitionContext risingTransition;
    risingTransition.energy = 0.64f;
    risingTransition.energySlope = 0.18f;
    assert(nativeTransitionStyle(NativeSceneKind::PrismGarden,
        NativeSceneKind::OrbitalLoom, risingTransition)
        == NativeTransitionStyle::FlowCarry);
    NativeTransitionContext balancedTransition;
    balancedTransition.energy = 0.52f;
    assert(nativeTransitionStyle(NativeSceneKind::PrismGarden,
        NativeSceneKind::OrbitalLoom, balancedTransition)
        == NativeTransitionStyle::FocalMorph);
    assert(nativeTransitionStyle(NativeSceneKind::DepthTunnel,
        NativeSceneKind::OrbitalLoom, harmonicTransition)
        == NativeTransitionStyle::DepthTravel);
    assert(nativeTransitionStyleName(NativeTransitionStyle::FlowCarry)
           == "flow-carry");
    assert(!nativeSceneFromName("unknown", parsedScene));
    assert(nativeSceneMaterial(NativeSceneKind::DepthTunnel).fieldExposure
           != nativeSceneMaterial(NativeSceneKind::DepthTunnel).asciiExposure);
    assert(nativeSceneMaterial(NativeSceneKind::Centrifuge).fieldExposure
           != nativeSceneMaterial(NativeSceneKind::WireOrganism).fieldExposure);

    NativeSceneDirector director;
    MusicFrame music;
    music.bpm = 120.0f;
    music.energyFast = 0.18f;
    music.energySlow = 0.20f;
    for (int frame = 0; frame < 300; ++frame) {
        director.update(music, 1.0f / 60.0f);
    }
    const NativeSceneState developed = director.update(music, 1.0f / 60.0f);
    assert(developed.sceneBeats > 9.0f);
    assert(developed.development > 0.35f);
    assert(developed.drive < 0.1f);

    director.requestNext();
    const NativeSceneState startingTransition = director.update(music, 1.0f / 60.0f);
    assert(startingTransition.transitioning);
    assert(startingTransition.transition > 0.0f);
    assert(startingTransition.transition < 0.01f);
    assert(startingTransition.currentScene == NativeSceneKind::DepthTunnel);
    assert(startingTransition.incomingScene == NativeSceneKind::Centrifuge);
    assert(startingTransition.transitionStyle
           == NativeTransitionStyle::DepthTravel);
    float previousMix = startingTransition.transition;
    for (int frame = 0; frame < 30; ++frame) {
        const float mix = director.update(music, 1.0f / 60.0f).transition;
        assert(mix >= previousMix);
        previousMix = mix;
    }
    finishTransition(director, music);
    const NativeSceneState changed = director.update(music, 1.0f / 60.0f);
    assert(!changed.transitioning);
    assert(changed.currentScene == NativeSceneKind::Centrifuge);

    NativeSceneDirector timedDirector;
    timedDirector.setTransitionDuration(0.75f);
    timedDirector.requestNext();
    for (int frame = 0; frame < 30; ++frame) {
        assert(timedDirector.update(music, 1.0f / 60.0f).transitioning);
    }
    for (int frame = 0; frame < 30
         && timedDirector.state().transitioning; ++frame) {
        timedDirector.update(music, 1.0f / 60.0f);
    }
    assert(!timedDirector.state().transitioning);
    assert(timedDirector.state().currentScene == NativeSceneKind::Centrifuge);

    NativeSceneDirector synchronizedStyleDirector;
    synchronizedStyleDirector.requestScene(NativeSceneKind::PrismGarden);
    synchronizedStyleDirector.requestTransitionStyle(
        NativeTransitionStyle::ControlledFracture);
    const NativeSceneState synchronizedStyle = synchronizedStyleDirector.update(
        music, 1.0f / 60.0f);
    assert(synchronizedStyle.transitioning);
    assert(synchronizedStyle.transitionStyle
           == NativeTransitionStyle::ControlledFracture);

    // A manual next request is one shot. It grants the requested scene a
    // minimum settled run, then the automatic director resumes on a musical
    // boundary without any hidden manual mode.
    NativeSceneDirector oneShotDirector;
    MusicFrame manualThenAutomatic = music;
    manualThenAutomatic.energyFast = 0.55f;
    manualThenAutomatic.energySlow = 0.50f;
    manualThenAutomatic.percussive = 0.45f;
    oneShotDirector.requestNext();
    oneShotDirector.update(manualThenAutomatic, 1.0f / 60.0f);
    finishTransition(oneShotDirector, manualThenAutomatic);
    for (int frame = 0; frame < 450; ++frame) {
        manualThenAutomatic.section = 0.0f;
        oneShotDirector.update(manualThenAutomatic, 1.0f / 60.0f);
    }
    manualThenAutomatic.section = 1.0f;
    assert(!oneShotDirector.update(
        manualThenAutomatic, 1.0f / 60.0f).transitioning);
    manualThenAutomatic.section = 0.0f;
    for (int frame = 0; frame < 40; ++frame) {
        oneShotDirector.update(manualThenAutomatic, 1.0f / 60.0f);
    }
    manualThenAutomatic.section = 1.0f;
    assert(oneShotDirector.update(
        manualThenAutomatic, 1.0f / 60.0f).transitioning);
    finishTransition(oneShotDirector, manualThenAutomatic);

    director.requestNext();
    director.update(music, 1.0f / 60.0f);
    finishTransition(director, music);
    assert(director.update(music, 1.0f / 60.0f).currentScene
           == NativeSceneKind::WireOrganism);
    director.requestNext();
    director.update(music, 1.0f / 60.0f);
    finishTransition(director, music);
    assert(director.update(music, 1.0f / 60.0f).currentScene
           == NativeSceneKind::PrismGarden);
    director.requestPrevious();
    director.update(music, 1.0f / 60.0f);
    finishTransition(director, music);
    assert(director.update(music, 1.0f / 60.0f).currentScene
           == NativeSceneKind::WireOrganism);

    music.energyFast = 0.86f;
    music.energySlow = 0.72f;
    music.percussive = 0.70f;
    for (int frame = 0; frame < 180; ++frame) {
        director.update(music, 1.0f / 60.0f);
    }
    const NativeSceneState driving = director.update(music, 1.0f / 60.0f);
    assert(driving.drive > 0.8f);
    assert(driving.peak > 0.8f);

    music.energySlope = -0.5f;
    for (int frame = 0; frame < 90; ++frame) {
        director.update(music, 1.0f / 60.0f);
    }
    const NativeSceneState releasing = director.update(music, 1.0f / 60.0f);
    assert(releasing.release > 0.9f);

    music.section = 1.0f;
    const NativeSceneState reset = director.update(music, 1.0f / 60.0f);
    assert(reset.sceneBeats > 8.0f);
    assert(!reset.transitioning);
    director.reset();
    director.selectScene(NativeSceneKind::Centrifuge);
    assert(director.update(MusicFrame{}, 1.0f / 60.0f).currentScene
           == NativeSceneKind::Centrifuge);
    director.reset();
    const NativeSceneState fresh = director.update(MusicFrame{}, 1.0f / 60.0f);
    assert(fresh.sceneBeats < 0.1f);

    NativeSceneDirector recurrenceDirector;
    MusicFrame recurrenceMusic;
    recurrenceMusic.bpm = 120.0f;
    recurrenceMusic.motifIdentity = 7;
    recurrenceMusic.section = 1.0f;
    const NativeSceneState firstMotif = recurrenceDirector.update(
        recurrenceMusic, 1.0f / 60.0f);
    assert(!firstMotif.motifRecalled);
    recurrenceMusic.section = 0.0f;
    recurrenceDirector.update(recurrenceMusic, 1.0f / 60.0f);
    recurrenceDirector.requestNext();
    recurrenceDirector.update(recurrenceMusic, 1.0f / 60.0f);
    finishTransition(recurrenceDirector, recurrenceMusic);
    assert(recurrenceDirector.update(recurrenceMusic, 1.0f / 60.0f).currentScene
           == NativeSceneKind::Centrifuge);
    recurrenceMusic.section = 1.0f;
    const NativeSceneState recalled = recurrenceDirector.update(
        recurrenceMusic, 1.0f / 60.0f);
    assert(recalled.motifRecalled);
    assert(!recalled.transitioning);
    recurrenceMusic.section = 0.0f;
    for (int frame = 0; frame < 260; ++frame) {
        recurrenceDirector.update(recurrenceMusic, 1.0f / 60.0f);
    }
    recurrenceMusic.section = 1.0f;
    const NativeSceneState settledRecurrence = recurrenceDirector.update(
        recurrenceMusic, 1.0f / 60.0f);
    assert(settledRecurrence.motifRecalled);
    assert(settledRecurrence.transitioning);
    assert(settledRecurrence.incomingScene == NativeSceneKind::DepthTunnel);

    NativeSceneDirector hiddenRecallDirector;
    MusicFrame hiddenRecallMusic;
    hiddenRecallMusic.bpm = 120.0f;
    hiddenRecallMusic.energyFast = 0.48f;
    hiddenRecallMusic.energySlow = 0.46f;
    hiddenRecallMusic.motifIdentity = 19;
    hiddenRecallMusic.section = 1.0f;
    hiddenRecallDirector.update(hiddenRecallMusic, 1.0f / 60.0f);
    hiddenRecallMusic.section = 0.0f;
    hiddenRecallDirector.update(hiddenRecallMusic, 1.0f / 60.0f);
    hiddenRecallDirector.requestNext();
    hiddenRecallDirector.update(hiddenRecallMusic, 1.0f / 60.0f);
    finishTransition(hiddenRecallDirector, hiddenRecallMusic);
    hiddenRecallDirector.setScenePreferences({}, {"depth-tunnel"});
    for (int frame = 0; frame < 260; ++frame) {
        hiddenRecallDirector.update(hiddenRecallMusic, 1.0f / 60.0f);
    }
    hiddenRecallMusic.section = 1.0f;
    const NativeSceneState hiddenRecall = hiddenRecallDirector.update(
        hiddenRecallMusic, 1.0f / 60.0f);
    assert(!hiddenRecall.motifRecalled);
    assert(!hiddenRecall.transitioning
           || hiddenRecall.incomingScene != NativeSceneKind::DepthTunnel);

    NativeSceneDirector trackResetDirector;
    trackResetDirector.selectScene(NativeSceneKind::WireOrganism);
    trackResetDirector.resetForTrack();
    assert(trackResetDirector.state().currentScene == NativeSceneKind::WireOrganism);
    assert(!trackResetDirector.state().transitioning);
    trackResetDirector.requestNext();
    trackResetDirector.update(recurrenceMusic, 1.0f / 60.0f);
    assert(trackResetDirector.state().transitioning);
    trackResetDirector.resetForTrack();
    assert(trackResetDirector.state().currentScene == NativeSceneKind::PrismGarden);
    assert(!trackResetDirector.state().transitioning);

    NativeSceneDirector textureDirector;
    MusicFrame drivingTexture;
    drivingTexture.bpm = 120.0f;
    drivingTexture.energyFast = 0.92f;
    drivingTexture.energySlow = 0.82f;
    drivingTexture.percussive = 0.92f;
    drivingTexture.harmonic = 0.24f;
    drivingTexture.spectralCentroid = 0.68f;
    drivingTexture.stereoWidth = 0.42f;
    for (int frame = 0; frame < 520; ++frame) {
        textureDirector.update(drivingTexture, 1.0f / 60.0f);
    }
    drivingTexture.motifIdentity = 31;
    drivingTexture.section = 1.0f;
    const NativeSceneState textureChoice = textureDirector.update(
        drivingTexture, 1.0f / 60.0f);
    assert(textureChoice.transitioning);
    assert(textureChoice.incomingScene == NativeSceneKind::Centrifuge);

    MusicFrame calmHarmonic;
    calmHarmonic.energyFast = 0.25f;
    calmHarmonic.energySlow = 0.25f;
    calmHarmonic.percussive = 0.15f;
    calmHarmonic.harmonic = 0.90f;
    calmHarmonic.spectralCentroid = 0.15f;
    calmHarmonic.stereoWidth = 0.65f;
    assert(automaticChoice(calmHarmonic) == NativeSceneKind::TidalGrid);

    MusicFrame wideHarmonic;
    wideHarmonic.energyFast = 0.60f;
    wideHarmonic.energySlow = 0.60f;
    wideHarmonic.percussive = 0.42f;
    wideHarmonic.harmonic = 0.74f;
    wideHarmonic.spectralCentroid = 0.54f;
    wideHarmonic.stereoWidth = 0.95f;
    assert(automaticChoice(wideHarmonic) == NativeSceneKind::OrbitalLoom);

    MusicFrame sparseBright;
    sparseBright.energyFast = 0.22f;
    sparseBright.energySlow = 0.22f;
    sparseBright.percussive = 0.18f;
    sparseBright.harmonic = 0.72f;
    sparseBright.spectralCentroid = 0.74f;
    sparseBright.stereoWidth = 0.76f;
    assert(automaticChoice(sparseBright)
           == NativeSceneKind::ConstellationField);

    MusicFrame radialMatch;
    radialMatch.energyFast = 0.82f;
    radialMatch.energySlow = 0.82f;
    radialMatch.percussive = 0.86f;
    radialMatch.harmonic = 0.28f;
    radialMatch.spectralCentroid = 0.66f;
    radialMatch.stereoWidth = 0.44f;
    const NativeSceneKind afterRadial = automaticChoiceFrom(
        NativeSceneKind::Centrifuge, radialMatch);
    assert(nativeSceneDefinition(afterRadial).visualFamily
           != NativeVisualFamily::Radial);

    MusicFrame landscapeMatch;
    landscapeMatch.energyFast = 0.34f;
    landscapeMatch.energySlow = 0.34f;
    landscapeMatch.percussive = 0.24f;
    landscapeMatch.harmonic = 0.84f;
    landscapeMatch.spectralCentroid = 0.20f;
    landscapeMatch.stereoWidth = 0.66f;
    const NativeSceneKind afterLandscape = automaticChoiceFrom(
        NativeSceneKind::TidalGrid, landscapeMatch);
    assert(nativeSceneDefinition(afterLandscape).visualFamily
           != NativeVisualFamily::Landscape);

    const NativeSceneKind balancedChoice = automaticChoiceWithProfile(
        NativeSceneKind::Centrifuge, wideHarmonic,
        NativeDirectorProfile::Balanced);
    const NativeSceneKind kineticChoice = automaticChoiceWithProfile(
        NativeSceneKind::Centrifuge, wideHarmonic,
        NativeDirectorProfile::Kinetic);
    const NativeSceneKind restrainedChoice = automaticChoiceWithProfile(
        NativeSceneKind::Centrifuge, wideHarmonic,
        NativeDirectorProfile::Restrained);
    const NativeSceneKind highContrastChoice = automaticChoiceWithProfile(
        NativeSceneKind::Centrifuge, wideHarmonic,
        NativeDirectorProfile::HighContrast);
    assert(balancedChoice == NativeSceneKind::ParticleWeave);
    assert(kineticChoice == NativeSceneKind::SpectralRibbons);
    assert(restrainedChoice == NativeSceneKind::WireOrganism);
    assert(highContrastChoice == NativeSceneKind::SpectralRibbons);

    NativeSceneDirector planningDirector;
    planningDirector.selectScene(NativeSceneKind::Centrifuge);
    MusicFrame planningMusic = wideHarmonic;
    planningMusic.bpm = 120.0f;
    for (int frame = 0; frame < 520; ++frame) {
        planningDirector.update(planningMusic, 1.0f / 60.0f);
    }
    planningMusic.section = 1.0f;
    const NativeSceneState plannedFirst = planningDirector.update(
        planningMusic, 1.0f / 60.0f);
    assert(plannedFirst.transitioning);
    assert(planningDirector.plannedScene());
    const NativeSceneKind plannedFollowing = *planningDirector.plannedScene();
    assert(plannedFollowing != plannedFirst.currentScene);
    assert(plannedFollowing != plannedFirst.incomingScene);
    assert(nativeSceneDefinition(plannedFollowing).visualFamily
        != nativeSceneDefinition(plannedFirst.incomingScene).visualFamily);
    finishTransition(planningDirector, planningMusic);
    planningMusic.section = 0.0f;
    for (int frame = 0; frame < 520; ++frame) {
        planningDirector.update(planningMusic, 1.0f / 60.0f);
    }
    planningMusic.section = 1.0f;
    const NativeSceneState plannedSecond = planningDirector.update(
        planningMusic, 1.0f / 60.0f);
    assert(plannedSecond.transitioning);
    assert(plannedSecond.incomingScene == plannedFollowing);
    planningDirector.requestNext();
    assert(!planningDirector.plannedScene());

    NativeSceneDirector adaptivePlanDirector;
    adaptivePlanDirector.selectScene(NativeSceneKind::Centrifuge);
    MusicFrame calmPlanMusic = calmHarmonic;
    calmPlanMusic.bpm = 120.0f;
    for (int frame = 0; frame < 520; ++frame) {
        adaptivePlanDirector.update(calmPlanMusic, 1.0f / 60.0f);
    }
    calmPlanMusic.section = 1.0f;
    adaptivePlanDirector.update(calmPlanMusic, 1.0f / 60.0f);
    assert(adaptivePlanDirector.plannedScene());
    const NativeSceneKind stalePlan = *adaptivePlanDirector.plannedScene();
    finishTransition(adaptivePlanDirector, calmPlanMusic);
    MusicFrame changedPlanMusic = radialMatch;
    changedPlanMusic.bpm = 120.0f;
    for (int frame = 0; frame < 520; ++frame) {
        changedPlanMusic.section = 0.0f;
        adaptivePlanDirector.update(changedPlanMusic, 1.0f / 60.0f);
    }
    changedPlanMusic.section = 1.0f;
    const NativeSceneState adaptedPlan = adaptivePlanDirector.update(
        changedPlanMusic, 1.0f / 60.0f);
    assert(adaptedPlan.transitioning);
    assert(adaptedPlan.incomingScene != stalePlan);

    NativeSceneDirector preferenceDirector;
    preferenceDirector.setScenePreferences(
        {"paper-horizon"}, {"centrifuge", "wire-organism"});
    assert(preferenceDirector.sceneFavorite(NativeSceneKind::PaperHorizon));
    assert(preferenceDirector.sceneHidden(NativeSceneKind::Centrifuge));
    assert(preferenceDirector.visibleSceneCount() == nativeSceneCount - 2);
    preferenceDirector.requestNext();
    preferenceDirector.update(music, 1.0f / 60.0f);
    finishTransition(preferenceDirector, music);
    assert(preferenceDirector.state().currentScene
           == NativeSceneKind::PrismGarden);
    preferenceDirector.requestPrevious();
    preferenceDirector.update(music, 1.0f / 60.0f);
    finishTransition(preferenceDirector, music);
    assert(preferenceDirector.state().currentScene
           == NativeSceneKind::DepthTunnel);

    const NativeSceneKind hiddenAutomatic = automaticChoiceWithPreferences(
        NativeSceneKind::Centrifuge, wideHarmonic, {}, {"particle-weave"});
    assert(hiddenAutomatic != NativeSceneKind::ParticleWeave);

    const NativeSceneKind favoredAutomatic = automaticChoiceWithPreferences(
        NativeSceneKind::Centrifuge, wideHarmonic, {"wire-organism"}, {});
    assert(favoredAutomatic == NativeSceneKind::WireOrganism);

    std::vector<std::string> nearlyAllHidden;
    for (std::size_t index = 0; index < nativeSceneCount - 1; ++index) {
        nearlyAllHidden.emplace_back(nativeSceneRegistry[index].slug);
    }
    preferenceDirector.setScenePreferences({}, nearlyAllHidden);
    assert(preferenceDirector.visibleSceneCount() == nativeSceneCount);
    preferenceDirector.setScenePreferences(
        {"paper-horizon"}, {"centrifuge", "wire-organism"});
    preferenceDirector.resetForTrack();
    assert(preferenceDirector.sceneFavorite(NativeSceneKind::PaperHorizon));
    assert(preferenceDirector.sceneHidden(NativeSceneKind::Centrifuge));
    preferenceDirector.reset();
    assert(!preferenceDirector.sceneFavorite(NativeSceneKind::PaperHorizon));
    assert(!preferenceDirector.sceneHidden(NativeSceneKind::Centrifuge));

    NativeSceneDirector profileResetDirector;
    profileResetDirector.setProfile(NativeDirectorProfile::Restrained);
    profileResetDirector.resetForTrack();
    assert(profileResetDirector.profile() == NativeDirectorProfile::Restrained);
    profileResetDirector.reset();
    assert(profileResetDirector.profile() == NativeDirectorProfile::Balanced);

    NativeSceneDirector recallDwellDirector;
    MusicFrame recallDwellMusic;
    recallDwellMusic.bpm = 120.0f;
    recallDwellMusic.motifIdentity = 11;
    recallDwellMusic.section = 1.0f;
    recallDwellDirector.update(recallDwellMusic, 1.0f / 60.0f);
    recallDwellMusic.section = 0.0f;
    recallDwellDirector.update(recallDwellMusic, 1.0f / 60.0f);
    recallDwellDirector.requestNext();
    recallDwellDirector.update(recallDwellMusic, 1.0f / 60.0f);
    finishTransition(recallDwellDirector, recallDwellMusic);
    recallDwellMusic.section = 1.0f;
    const NativeSceneState earlyRecall = recallDwellDirector.update(
        recallDwellMusic, 1.0f / 60.0f);
    assert(earlyRecall.motifRecalled);
    assert(!earlyRecall.transitioning);
    recallDwellMusic.section = 0.0f;
    for (int frame = 0; frame < 260; ++frame) {
        recallDwellDirector.update(recallDwellMusic, 1.0f / 60.0f);
    }
    recallDwellMusic.section = 1.0f;
    const NativeSceneState settledRecall = recallDwellDirector.update(
        recallDwellMusic, 1.0f / 60.0f);
    assert(settledRecall.motifRecalled);
    assert(settledRecall.transitioning);
    assert(settledRecall.incomingScene == NativeSceneKind::DepthTunnel);

    NativeSceneDirector fallbackDirector;
    MusicFrame fallbackMusic;
    fallbackMusic.bpm = 120.0f;
    fallbackMusic.energyFast = 0.46f;
    fallbackMusic.energySlow = 0.42f;
    fallbackMusic.harmonic = 0.62f;
    bool fallbackStarted = false;
    for (int frame = 0; frame < 1100 && !fallbackStarted; ++frame) {
        fallbackMusic.barPhase = std::fmod(frame / 120.0f, 1.0f);
        fallbackMusic.phrasePhase = std::fmod(frame / 480.0f, 1.0f);
        fallbackStarted = fallbackDirector.update(
            fallbackMusic, 1.0f / 60.0f).transitioning;
    }
    assert(fallbackStarted);

    NativeSceneDirector silentDirector;
    MusicFrame silentMusic;
    silentMusic.bpm = 120.0f;
    for (int frame = 0; frame < 2100; ++frame) {
        silentMusic.barPhase = std::fmod(frame / 120.0f, 1.0f);
        silentMusic.phrasePhase = std::fmod(frame / 480.0f, 1.0f);
        assert(!silentDirector.update(
            silentMusic, 1.0f / 60.0f).transitioning);
    }
    std::cout << "native scene state passed\n";
}
