#include "audio_features.h"
#include "music_frame.h"
#include "musical_structure.h"
#include "native_renderer.h"
#include "native_scene_state.h"
#include "signal_monitor.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr int defaultWidth = 480;
constexpr int defaultHeight = 270;

int replayDimension(const char* name, int fallback, int minimum, int maximum) {
    const char* value = std::getenv(name);
    if (!value || !*value) return fallback;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (!end || *end != '\0') {
        std::cerr << "invalid " << name << ": " << value << "\n";
        return fallback;
    }
    return static_cast<int>(std::clamp(parsed,
        static_cast<long>(minimum), static_cast<long>(maximum)));
}

float replayLevel(const char* name, float fallback, float minimum,
                  float maximum) {
    const char* value = std::getenv(name);
    if (!value || !*value) return fallback;
    char* end = nullptr;
    const float parsed = std::strtof(value, &end);
    if (!end || *end != '\0' || !std::isfinite(parsed)) {
        std::cerr << "invalid " << name << ": " << value << "\n";
        return fallback;
    }
    return std::clamp(parsed, minimum, maximum);
}

std::vector<float> readTexture(GLuint texture, int width, int height) {
    std::vector<float> pixels(width * height * 4);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
    return pixels;
}

bool writeRgb(std::ostream& output, const std::vector<float>& source,
              const std::vector<float>& incoming, float transition,
              float exposure, int width, int height,
              const MusicFrame* signalMonitor = nullptr) {
    const float mix = std::clamp(transition, 0.0f, 1.0f);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t offset = static_cast<std::size_t>(y * width + x) * 4;
            std::array<float, 3> pixel{};
            for (int channel = 0; channel < 3; ++channel) {
                const float linear = source[offset + channel] * (1.0f - mix)
                                   + incoming[offset + channel] * mix;
                const float mapped = linear / (1.0f + linear * 0.85f);
                pixel[channel] = std::clamp(mapped * exposure, 0.0f, 1.0f);
            }
            if (signalMonitor) {
                const int displayY = height - 1 - y;
                pixel = signalMonitorPixel(
                    *signalMonitor, x, displayY, width, height, pixel);
            }
            for (int channel = 0; channel < 3; ++channel) {
                const auto value = static_cast<unsigned char>(
                    std::pow(pixel[channel], 1.0f / 2.2f) * 255.0f + 0.5f);
                output.write(reinterpret_cast<const char*>(&value), 1);
            }
        }
    }
    return static_cast<bool>(output);
}

bool writePpm(const std::filesystem::path& path,
              const std::vector<float>& source,
              const std::vector<float>& incoming, float transition,
              float exposure, int width, int height,
              const MusicFrame* signalMonitor = nullptr) {
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    output << "P6\n" << width << " " << height << "\n255\n";
    return writeRgb(output, source, incoming, transition, exposure,
                    width, height, signalMonitor);
}

std::string sceneSlug(NativeSceneKind scene) {
    return std::string(nativeSceneDefinition(scene).slug);
}

struct FrameDelta {
    float motion = 0.0f;
    float coverage = 0.0f;
    float coherence = 0.0f;
};

FrameDelta measureFrameDelta(const std::vector<float>& before,
                             const std::vector<float>& after) {
    if (before.size() != after.size() || before.empty()) return {};
    float total = 0.0f;
    float signedTotal = 0.0f;
    int changed = 0;
    for (std::size_t index = 0; index < before.size(); index += 4) {
        const float beforeLight = 0.299f * before[index]
                                + 0.587f * before[index + 1]
                                + 0.114f * before[index + 2];
        const float afterLight = 0.299f * after[index]
                               + 0.587f * after[index + 1]
                               + 0.114f * after[index + 2];
        const float signedDelta = afterLight - beforeLight;
        const float delta = std::abs(signedDelta);
        total += delta;
        signedTotal += signedDelta;
        changed += delta >= 0.0125f;
    }
    const float pixels = static_cast<float>(before.size() / 4);
    const float motion = total / pixels;
    return {motion, changed / pixels,
            std::abs(signedTotal / pixels) / std::max(1e-7f, motion)};
}

struct MotionBucket {
    float total = 0.0f;
    float coverageTotal = 0.0f;
    float coherenceTotal = 0.0f;
    float globalPulseTotal = 0.0f;
    int pulseFrames10 = 0;
    int pulseFrames20 = 0;
    int count = 0;
    void add(const FrameDelta& delta) {
        total += delta.motion;
        coverageTotal += delta.coverage;
        coherenceTotal += delta.coherence;
        const float globalPulse = delta.coverage * delta.coherence;
        globalPulseTotal += globalPulse;
        pulseFrames10 += globalPulse >= 0.10f;
        pulseFrames20 += globalPulse >= 0.20f;
        ++count;
    }
    float mean() const { return count > 0 ? total / count : 0.0f; }
    float meanCoverage() const {
        return count > 0 ? coverageTotal / count : 0.0f;
    }
    float meanCoherence() const {
        return count > 0 ? coherenceTotal / count : 0.0f;
    }
    float meanGlobalPulse() const {
        return count > 0 ? globalPulseTotal / count : 0.0f;
    }
    float pulseDuty10() const {
        return count > 0 ? static_cast<float>(pulseFrames10) / count : 0.0f;
    }
    float pulseDuty20() const {
        return count > 0 ? static_cast<float>(pulseFrames20) / count : 0.0f;
    }
};
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: native-song-replay SHADER_DIRECTORY "
                     "RAW_F32_STEREO OUTPUT_DIRECTORY\n";
        return 2;
    }
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) {
        std::cerr << "could not open: " << argv[2] << "\n";
        return 1;
    }
    const std::uintmax_t inputBytes = std::filesystem::file_size(argv[2]);
    const std::uintmax_t bytesPerHop
        = AudioFeatureBus::hopSize * 2u * sizeof(float);
    const std::size_t totalHops = static_cast<std::size_t>(
        inputBytes / bytesPerHop);
    const std::filesystem::path outputDirectory = argv[3];
    std::filesystem::create_directories(outputDirectory);
    const int width = replayDimension(
        "OMADROP_REPLAY_WIDTH", defaultWidth, 320, 1920);
    const int height = replayDimension(
        "OMADROP_REPLAY_HEIGHT", defaultHeight, 180, 1080);
    const NativeRenderPolicy renderPolicy{
        .intensity = replayLevel(
            "OMADROP_REPLAY_INTENSITY", 1.0f, 0.50f, 1.50f),
        .motion = replayLevel(
            "OMADROP_REPLAY_MOTION", 1.0f, 0.0f, 1.0f),
        .reducedMotion
            = std::getenv("OMADROP_REPLAY_REDUCED_MOTION") != nullptr,
        .flashLimited
            = std::getenv("OMADROP_REPLAY_FLASH_LIMITED") != nullptr,
        .quality = replayLevel(
            "OMADROP_REPLAY_QUALITY", 1.0f, 0.50f, 1.0f),
    };
    std::ofstream frameStream;
    if (const char* streamPath = std::getenv("OMADROP_REPLAY_FRAME_STREAM")) {
        frameStream.open(streamPath, std::ios::binary);
        if (!frameStream) {
            std::cerr << "could not open frame stream: " << streamPath << "\n";
            return 1;
        }
    }
    std::ofstream timeline(outputDirectory / "timeline.tsv");
    timeline << "seconds\tscene\tkick\tsnare\that\tonset_pulse\tbeat_pulse\tbeat_phase"
                "\tbpm\tclock_confidence\tbar\tsection\tflux_sub\tflux_bass"
                "\tflux_low_mid\tflux_mid"
                "\tflux_presence\tflux_high\trhythmic_density\tsyncopation"
                "\ttonal_motion\tharmonic_change\tenergy_fast\tenergy_slow"
                "\tenergy_slope\tarrangement\tarrangement_confidence"
                "\tmotion\tmotion_coverage"
                "\tmotion_coherence\tglobal_pulse\n";

    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Omadrop native song replay", 0, 0,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) return 1;
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) return 1;
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return 1;

    NativeRenderer renderer;
    std::string error;
    if (!renderer.initialize(argv[1], error)) {
        std::cerr << error << "\n";
        return 1;
    }
    AudioFeatureBus bus;
    MusicalStructureTracker structureTracker;
    MusicFrameBuilder musicFrameBuilder;
    NativeSceneDirector sceneDirector;
    bool fixedScene = false;
    if (const char* requestedScene = std::getenv("OMADROP_REPLAY_SCENE")) {
        NativeSceneKind selectedScene;
        if (!nativeSceneFromName(requestedScene, selectedScene)) {
            std::cerr << "unknown OMADROP_REPLAY_SCENE: "
                      << requestedScene << "\n";
            return 2;
        }
        sceneDirector.selectScene(selectedScene);
        fixedScene = true;
    }
    const bool measureMotion = std::getenv("OMADROP_REPLAY_MEASURE") != nullptr;
    const bool showSignalMonitor
        = std::getenv("OMADROP_REPLAY_SIGNAL_MONITOR") != nullptr;
    std::vector<float> pcm(AudioFeatureBus::hopSize * 2);
    std::size_t hops = 0;
    int captures = 0;
    int sections = 0;
    NativeSceneKind reportedScene = sceneDirector.state().currentScene;
    std::vector<float> previousFrame;
    MotionBucket quietMotion;
    MotionBucket allMotion;
    MotionBucket beatMotion;
    MotionBucket kickMotion;
    MotionBucket snareMotion;
    MotionBucket hatMotion;
    MotionBucket onsetMotion;
    MotionBucket kickRecoveryMotion;
    MotionBucket snareRecoveryMotion;
    MotionBucket hatRecoveryMotion;
    int kickRecoveryFrames = 0;
    int snareRecoveryFrames = 0;
    int hatRecoveryFrames = 0;
    float previousBeat = 0.0f;
    float previousKick = 0.0f;
    float previousSnare = 0.0f;
    float previousHat = 0.0f;
    float previousOnset = 0.0f;
    bool reportedTransitioning = false;
    const std::array<float, 3> color{0.44f, 0.70f, 1.0f};
    const std::size_t captureInterval = std::getenv("OMADROP_REPLAY_CAPTURE_HOPS")
        ? static_cast<std::size_t>(std::max(
            1, std::atoi(std::getenv("OMADROP_REPLAY_CAPTURE_HOPS"))))
        : 120;
    const std::size_t maximumHops = std::getenv("OMADROP_REPLAY_MAX_SECONDS")
        ? static_cast<std::size_t>(std::max(
            1, std::atoi(std::getenv("OMADROP_REPLAY_MAX_SECONDS")))) * 60
        : std::numeric_limits<std::size_t>::max();

    while (hops < maximumHops
           && input.read(reinterpret_cast<char*>(pcm.data()),
                      static_cast<std::streamsize>(pcm.size() * sizeof(float)))) {
        const AudioFeatures features = bus.processStereo(
            pcm.data(), AudioFeatureBus::hopSize);
        const MusicalStructureState& structure = structureTracker.update(features);
        const float trackProgress = totalHops > 1
            ? static_cast<float>(hops) / static_cast<float>(totalHops - 1)
            : -1.0f;
        const MusicFrame& music = musicFrameBuilder.update(
            features, structure, 1.0f / 60.0f, 0.0f, trackProgress);
        const NativeSceneState& scene = sceneDirector.update(
            music, 1.0f / 60.0f, !fixedScene);
        if (!renderer.render(music, scene, width, height, color,
                             0, 1.0f, 1.0f / 60.0f, error,
                             renderPolicy)) {
            std::cerr << error << "\n";
            return 1;
        }

        const double seconds = hops * AudioFeatureBus::hopSize
                             / static_cast<double>(AudioFeatureBus::sampleRate);
        if (scene.currentScene != reportedScene) {
            reportedScene = scene.currentScene;
            std::cout << "scene " << seconds << " sec "
                      << nativeSceneName(reportedScene) << "\n";
        }
        if (scene.transitioning && !reportedTransitioning) {
            std::cout << "transition " << seconds << " sec "
                      << nativeSceneName(scene.currentScene) << " -> "
                      << nativeSceneName(scene.incomingScene) << ' '
                      << nativeTransitionStyleName(scene.transitionStyle)
                      << " energy="
                      << (0.58f * music.energyFast + 0.42f * music.energySlow)
                      << " slope=" << music.energySlope
                      << " density=" << music.rhythmicDensity
                      << " harmonic-change=" << music.harmonicChange
                      << " arrangement="
                      << arrangementRoleName(music.arrangementRole)
                      << " confidence=" << music.arrangementConfidence << "\n";
        }
        reportedTransitioning = scene.transitioning;
        if (music.arrangementChanged) {
            std::cout << "arrangement " << seconds << " sec "
                      << arrangementRoleName(music.arrangementRole)
                      << " confidence=" << music.arrangementConfidence << "\n";
        }
        if (structure.sectionCrossed) ++sections;

        if (frameStream.is_open()) {
            const std::vector<float> source = readTexture(
                renderer.texture(scene.currentScene), width, height);
            const std::vector<float> incoming = scene.transitioning
                ? readTexture(renderer.texture(scene.incomingScene), width, height)
                : source;
            const NativeSceneMaterial sourceMaterial
                = nativeSceneMaterial(scene.currentScene);
            const NativeSceneMaterial incomingMaterial = nativeSceneMaterial(
                scene.transitioning ? scene.incomingScene : scene.currentScene);
            const float transition = scene.transitioning ? scene.transition : 0.0f;
            const float exposure = sourceMaterial.fieldExposure * (1.0f - transition)
                                 + incomingMaterial.fieldExposure * transition;
            if (!writeRgb(frameStream, source, incoming, transition, exposure,
                          width, height, showSignalMonitor ? &music : nullptr)) {
                std::cerr << "could not write replay frame stream\n";
                return 1;
            }
        }

        float frameMotion = -1.0f;
        float frameCoverage = -1.0f;
        float frameCoherence = -1.0f;
        if (measureMotion) {
            std::vector<float> currentFrame = readTexture(
                renderer.texture(scene.currentScene), width, height);
            if (!previousFrame.empty()) {
                const FrameDelta delta = measureFrameDelta(previousFrame, currentFrame);
                frameMotion = delta.motion;
                frameCoverage = delta.coverage;
                frameCoherence = delta.coherence;
                allMotion.add(delta);
                const bool kickRaised = music.kick > previousKick + 0.15f;
                const bool snareRaised = music.snare > previousSnare + 0.15f;
                const bool hatRaised = music.hat > previousHat + 0.15f;
                if (kickRaised || snareRaised || hatRaised) {
                    kickRecoveryFrames = 0;
                    snareRecoveryFrames = 0;
                    hatRecoveryFrames = 0;
                } else {
                    if (kickRecoveryFrames > 0 && --kickRecoveryFrames == 0) {
                        kickRecoveryMotion.add(delta);
                    }
                    if (snareRecoveryFrames > 0 && --snareRecoveryFrames == 0) {
                        snareRecoveryMotion.add(delta);
                    }
                    if (hatRecoveryFrames > 0 && --hatRecoveryFrames == 0) {
                        hatRecoveryMotion.add(delta);
                    }
                }
                if (music.beatPulse > previousBeat + 0.20f) beatMotion.add(delta);
                if (kickRaised) {
                    kickMotion.add(delta);
                    kickRecoveryFrames = 10;
                }
                if (snareRaised) {
                    snareMotion.add(delta);
                    snareRecoveryFrames = 10;
                }
                if (hatRaised) {
                    hatMotion.add(delta);
                    hatRecoveryFrames = 10;
                }
                if (music.onsetPulse > previousOnset + 0.15f) {
                    onsetMotion.add(delta);
                }
                if (music.beatPulse < 0.035f && music.kick < 0.035f
                    && music.snare < 0.035f && music.hat < 0.035f
                    && music.onsetPulse < 0.035f) {
                    quietMotion.add(delta);
                }
            }
            previousFrame = std::move(currentFrame);
            previousBeat = music.beatPulse;
            previousKick = music.kick;
            previousSnare = music.snare;
            previousHat = music.hat;
            previousOnset = music.onsetPulse;
        }
        timeline << seconds << '\t' << nativeSceneName(scene.currentScene)
                 << '\t' << music.kick << '\t' << music.snare << '\t' << music.hat
                 << '\t' << music.onsetPulse << '\t' << music.beatPulse
                 << '\t' << music.beatPhase
                 << '\t' << music.bpm << '\t' << music.clockConfidence
                 << '\t' << music.barPhase << '\t' << music.section
                 << '\t' << music.bandFlux[0] << '\t' << music.bandFlux[1]
                 << '\t' << music.bandFlux[2] << '\t' << music.bandFlux[3]
                 << '\t' << music.bandFlux[4] << '\t' << music.bandFlux[5]
                 << '\t' << music.rhythmicDensity << '\t' << music.syncopation
                 << '\t' << music.tonalMotion << '\t' << music.harmonicChange
                 << '\t' << music.energyFast << '\t' << music.energySlow
                 << '\t' << music.energySlope
                 << '\t' << arrangementRoleName(music.arrangementRole)
                 << '\t' << music.arrangementConfidence
                 << '\t' << frameMotion << '\t' << frameCoverage
                 << '\t' << frameCoherence << '\t'
                 << (frameCoverage >= 0.0f
                     ? frameCoverage * frameCoherence : -1.0f) << '\n';

        const bool periodicCapture = hops % captureInterval == 0;
        const bool structuralCapture = structure.sectionCrossed;
        if (periodicCapture || structuralCapture) {
            const std::vector<float> source = readTexture(
                renderer.texture(scene.currentScene), width, height);
            const std::vector<float> incoming = scene.transitioning
                ? readTexture(renderer.texture(scene.incomingScene), width, height)
                : source;
            std::ostringstream filename;
            filename << std::setw(6) << std::setfill('0') << hops << '-'
                     << sceneSlug(scene.currentScene)
                     << (structuralCapture ? "-section" : "") << ".ppm";
            const NativeSceneMaterial sourceMaterial
                = nativeSceneMaterial(scene.currentScene);
            const NativeSceneMaterial incomingMaterial = nativeSceneMaterial(
                scene.transitioning ? scene.incomingScene : scene.currentScene);
            const float transition = scene.transitioning ? scene.transition : 0.0f;
            const float exposure = sourceMaterial.fieldExposure * (1.0f - transition)
                                 + incomingMaterial.fieldExposure * transition;
            if (!writePpm(outputDirectory / filename.str(), source, incoming,
                          transition, exposure, width, height,
                          showSignalMonitor ? &music : nullptr)) {
                std::cerr << "could not write replay frame\n";
                return 1;
            }
            ++captures;
        }
        ++hops;
    }

    renderer.shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    const double duration = hops * AudioFeatureBus::hopSize
                          / static_cast<double>(AudioFeatureBus::sampleRate);
    std::cout << "native song replay " << duration << " sec, " << captures
              << " frames, " << sections << " sections, " << width << "x"
              << height << "\n";
    if (measureMotion) {
        const float quiet = std::max(1e-7f, quietMotion.mean());
        const NativeSceneDefinition& definition = nativeSceneDefinition(
            sceneDirector.state().currentScene);
        std::cout << "motion " << nativeSceneName(sceneDirector.state().currentScene)
                  << " quiet=" << quietMotion.mean() << " (" << quietMotion.count
                  << ") beat=" << beatMotion.mean() / quiet << "x ("
                  << beatMotion.count << ") kick=" << kickMotion.mean() / quiet
                  << "x (" << kickMotion.count << ") snare="
                  << snareMotion.mean() / quiet << "x (" << snareMotion.count
                  << ") hat=" << hatMotion.mean() / quiet << "x ("
                  << hatMotion.count << ") onset=" << onsetMotion.mean() / quiet
                  << "x (" << onsetMotion.count << ")"
                  << " quiet_coverage=" << quietMotion.meanCoverage()
                  << " grammar=" << nativeMotionGrammarName(
                         definition.motionGrammar)
                  << " quiet_coverage_limit="
                  << definition.maximumQuietMotionCoverage
                  << " global_pulse_limit=" << definition.maximumGlobalPulse
                  << " mean_global_pulse=" << allMotion.meanGlobalPulse()
                  << " pulse_duty_10=" << allMotion.pulseDuty10()
                  << " pulse_duty_20=" << allMotion.pulseDuty20()
                  << " beat_coverage=" << beatMotion.meanCoverage()
                  << " kick_coverage=" << kickMotion.meanCoverage()
                  << " snare_coverage=" << snareMotion.meanCoverage()
                  << " hat_coverage=" << hatMotion.meanCoverage()
                  << " onset_coverage=" << onsetMotion.meanCoverage()
                  << " quiet_coherence=" << quietMotion.meanCoherence()
                  << " beat_coherence=" << beatMotion.meanCoherence()
                  << " kick_coherence=" << kickMotion.meanCoherence()
                  << " snare_coherence=" << snareMotion.meanCoherence()
                  << " hat_coherence=" << hatMotion.meanCoherence()
                  << " kick_recovery=" << kickRecoveryMotion.mean()
                      / std::max(1e-7f, kickMotion.mean())
                  << " snare_recovery=" << snareRecoveryMotion.mean()
                      / std::max(1e-7f, snareMotion.mean())
                  << " hat_recovery=" << hatRecoveryMotion.mean()
                      / std::max(1e-7f, hatMotion.mean()) << "\n";
    }
    return hops > 0 && captures > 0 ? 0 : 1;
}
