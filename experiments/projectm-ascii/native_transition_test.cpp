#include "live_compositor.h"
#include "native_renderer.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {
constexpr int width = 640;
constexpr int height = 360;

constexpr const char* vertexSource = R"GLSL(
#version 330 core
out vec2 uv;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

std::string compositorFragment(const std::filesystem::path& liveSource) {
    const std::string source = readFile(liveSource);
    const std::string opening = "const char* fragmentSource = R\"GLSL(\n";
    const std::string closing = "\n)GLSL\";";
    const std::size_t begin = source.find(opening);
    if (begin == std::string::npos) return {};
    const std::size_t content = begin + opening.size();
    const std::size_t end = source.find(closing, content);
    if (end == std::string::npos) return {};
    return source.substr(content, end - content);
}

GLuint blackTexture() {
    constexpr std::array<unsigned char, 4> pixel{0, 0, 0, 255};
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

std::vector<unsigned char> readFrame() {
    std::vector<unsigned char> pixels(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}

bool writePpm(const std::filesystem::path& path,
              const std::vector<unsigned char>& pixels) {
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t offset
                = static_cast<std::size_t>(y * width + x) * 4;
            output.write(reinterpret_cast<const char*>(pixels.data() + offset), 3);
        }
    }
    return static_cast<bool>(output);
}

float meanDifference(const std::vector<unsigned char>& a,
                     const std::vector<unsigned char>& b) {
    assert(a.size() == b.size());
    double difference = 0.0;
    for (std::size_t index = 0; index < a.size(); index += 4) {
        difference += std::abs(static_cast<int>(a[index]) - b[index]);
        difference += std::abs(static_cast<int>(a[index + 1]) - b[index + 1]);
        difference += std::abs(static_cast<int>(a[index + 2]) - b[index + 2]);
    }
    return static_cast<float>(difference / (a.size() * 0.75 * 255.0));
}

MusicFrame reviewMusic(int frame) {
    MusicFrame music;
    music.bpm = 120.0f;
    music.clockConfidence = 0.92f;
    music.beatPhase = std::fmod(frame / 30.0f, 1.0f);
    music.barPhase = std::fmod(frame / 120.0f, 1.0f);
    music.phrasePhase = std::fmod(frame / 480.0f, 1.0f);
    music.energyFast = 0.46f;
    music.energySlow = 0.42f;
    music.harmonic = 0.62f;
    music.percussive = 0.38f;
    music.spectralCentroid = 0.48f;
    music.stereoWidth = 0.54f;
    music.tonalMotion = 0.24f;
    music.harmonicChange = 0.18f;
    music.rhythmicDensity = 0.44f;
    music.syncopation = 0.30f;
    for (std::size_t index = 0; index < music.bandLevel.size(); ++index) {
        music.bandLevel[index] = 0.34f + 0.035f * static_cast<float>(index);
    }
    for (std::size_t index = 0; index < music.spectrumLevel.size(); ++index) {
        music.spectrumLevel[index] = 0.26f
            + 0.16f * std::sin(static_cast<float>(index) * 0.43f + 0.7f);
    }
    const int beatFrame = frame % 30;
    music.beatPulse = beatFrame < 9
        ? std::exp(-beatFrame * 8.0f / 60.0f) : 0.0f;
    music.kick = beatFrame < 7
        ? std::exp(-beatFrame * 7.0f / 60.0f) : 0.0f;
    const int snareFrame = frame % 60;
    music.snare = snareFrame >= 30 && snareFrame < 37
        ? std::exp(-(snareFrame - 30) * 8.0f / 60.0f) : 0.0f;
    const int hatFrame = frame % 15;
    music.hat = hatFrame < 4
        ? std::exp(-hatFrame * 13.0f / 60.0f) : 0.0f;
    return music;
}

struct TransitionReview {
    NativeSceneKind source;
    NativeSceneKind incoming;
    NativeTransitionStyle expectedStyle;
    NativeTransitionContext context{};
    bool contextual = false;
};
}

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "usage: native-transition-test SHADER_DIRECTORY "
                     "LIVE_CPP [OUTPUT_DIRECTORY]\n";
        return 2;
    }
    const std::string fragment = compositorFragment(argv[2]);
    if (fragment.empty()) {
        std::cerr << "could not extract production compositor shader\n";
        return 1;
    }
    const std::filesystem::path output = argc == 4 ? argv[3] : "";
    if (!output.empty()) std::filesystem::create_directories(output);

    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Omadrop native transition test",
        0, 0, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    assert(context);
    glewExperimental = GL_TRUE;
    assert(glewInit() == GLEW_OK);

    std::string error;
    NativeRenderer renderer;
    assert(renderer.initialize(argv[1], error));
    LiveCompositor compositor;
    assert(compositor.initialize(vertexSource, fragment.c_str(), error));
    const GLuint black = blackTexture();

    const std::array<TransitionReview, 9> reviews{{
        {NativeSceneKind::InkCurrent, NativeSceneKind::Centrifuge,
         NativeTransitionStyle::FlowCarry},
        {NativeSceneKind::Centrifuge, NativeSceneKind::BloomEngine,
         NativeTransitionStyle::FocalMorph},
        {NativeSceneKind::GlassChoir, NativeSceneKind::ShadowArchitecture,
         NativeTransitionStyle::DepthTravel},
        {NativeSceneKind::InkCurrent, NativeSceneKind::GlassChoir,
         NativeTransitionStyle::ControlledFracture},
        {NativeSceneKind::NegativeSpace, NativeSceneKind::ShadowArchitecture,
         NativeTransitionStyle::NegativeSpaceReveal},
        {NativeSceneKind::PrismGarden, NativeSceneKind::OrbitalLoom,
         NativeTransitionStyle::NegativeSpaceReveal,
         {.energy = 0.20f, .rhythmicDensity = 0.10f}, true},
        {NativeSceneKind::LivingMosaic, NativeSceneKind::LumenFold,
         NativeTransitionStyle::ControlledFracture,
         {.energy = 0.52f, .harmonic = 0.78f,
          .harmonicChange = 0.55f}, true},
        {NativeSceneKind::ParticleWeave, NativeSceneKind::PrismGarden,
         NativeTransitionStyle::FlowCarry,
         {.energy = 0.64f, .energySlope = 0.18f}, true},
        {NativeSceneKind::OrbitalLoom, NativeSceneKind::TidalGrid,
         NativeTransitionStyle::FocalMorph,
         {.energy = 0.52f}, true},
    }};
    for (int frame = 0; frame < 180; ++frame) {
        for (std::size_t sceneIndex = 0;
             sceneIndex < nativeSceneCount; ++sceneIndex) {
            const NativeSceneKind kind
                = static_cast<NativeSceneKind>(sceneIndex);
            NativeSceneState state;
            state.currentScene = kind;
            state.incomingScene = kind;
            state.development = 0.58f;
            state.drive = 0.42f;
            state.peak = 0.22f;
            state.sceneBeats = frame / 30.0f;
            assert(renderer.render(reviewMusic(frame), state, width, height,
                {0.44f, 0.70f, 1.0f}, 0, 1.0f, 1.0f / 60.0f, error));
        }
    }

    auto renderPolicyFrame = [&](float motion, float contrast) {
        LiveCompositorFrame frame;
        frame.sourceTexture = renderer.texture(NativeSceneKind::Centrifuge);
        frame.nextTexture = renderer.texture(NativeSceneKind::BloomEngine);
        frame.coverTexture = black;
        frame.width = width;
        frame.height = height;
        frame.sceneMix = 0.5f;
        frame.transitionMode = static_cast<int>(
            NativeTransitionStyle::FocalMorph);
        frame.fieldExposure = 0.96f;
        frame.asciiExposure = 1.02f;
        frame.nativeRenderer = true;
        frame.motionScale = motion;
        frame.contrastScale = contrast;
        frame.visibility = 1.0f;
        assert(compositor.render(frame, error));
        glFinish();
        return readFrame();
    };
    const std::vector<unsigned char> standardPolicy
        = renderPolicyFrame(1.0f, 1.0f);
    const float reducedMotionDifference = meanDifference(
        standardPolicy, renderPolicyFrame(0.35f, 1.0f));
    const float highContrastDifference = meanDifference(
        standardPolicy, renderPolicyFrame(1.0f, 1.16f));
    assert(reducedMotionDifference > 0.0005f);
    assert(highContrastDifference > 0.0005f);
    std::cout << "transition policy reduced_motion_difference="
              << reducedMotionDifference << " high_contrast_difference="
              << highContrastDifference << '\n';

    constexpr std::array<float, 6> progress{0.0f, 0.2f, 0.4f,
                                            0.6f, 0.8f, 1.0f};
    for (const TransitionReview& review : reviews) {
        const NativeTransitionStyle selectedStyle = review.contextual
            ? nativeTransitionStyle(review.source, review.incoming,
                                    review.context)
            : nativeTransitionStyle(review.source, review.incoming);
        assert(selectedStyle == review.expectedStyle);
        std::vector<unsigned char> first;
        std::vector<unsigned char> middle;
        std::vector<unsigned char> last;
        for (std::size_t index = 0; index < progress.size(); ++index) {
            const float mix = progress[index];
            const NativeSceneMaterial sourceMaterial
                = nativeSceneMaterial(review.source);
            const NativeSceneMaterial incomingMaterial
                = nativeSceneMaterial(review.incoming);
            LiveCompositorFrame frame;
            frame.sourceTexture = renderer.texture(review.source);
            frame.nextTexture = renderer.texture(review.incoming);
            frame.coverTexture = black;
            frame.width = width;
            frame.height = height;
            frame.sceneMix = mix;
            frame.transitionMode = static_cast<int>(selectedStyle);
            frame.fieldExposure = sourceMaterial.fieldExposure * (1.0f - mix)
                                + incomingMaterial.fieldExposure * mix;
            frame.asciiExposure = sourceMaterial.asciiExposure * (1.0f - mix)
                                + incomingMaterial.asciiExposure * mix;
            frame.nativeRenderer = true;
            frame.visibility = 1.0f;
            assert(compositor.render(frame, error));
            glFinish();
            std::vector<unsigned char> pixels = readFrame();
            if (index == 0) first = pixels;
            if (index == progress.size() / 2) middle = pixels;
            if (index + 1 == progress.size()) last = pixels;
            if (!output.empty()) {
                std::string name
                    = std::string(nativeSceneDefinition(review.source).slug)
                    + "-to-"
                    + std::string(nativeSceneDefinition(review.incoming).slug);
                if (review.contextual) {
                    name += "-state-"
                          + std::string(nativeTransitionStyleName(selectedStyle));
                }
                name += '-' + std::to_string(index) + ".ppm";
                assert(writePpm(output / name, pixels));
            }
        }
        const float firstHalf = meanDifference(first, middle);
        const float secondHalf = meanDifference(middle, last);
        assert(firstHalf > 0.002f);
        assert(secondHalf > 0.002f);
        std::cout << nativeSceneName(review.source) << " -> "
                  << nativeSceneName(review.incoming) << " mode="
                  << static_cast<int>(selectedStyle)
                  << (review.contextual ? " state-aware" : "")
                  << " halves=" << firstHalf << ',' << secondHalf << '\n';
    }

    // Every scene gets two deterministic outgoing paths. Because both offsets
    // are permutations of the registry, every scene also gets two incoming
    // paths. This exercises the exact production compositor and style chooser,
    // not a simplified transition shader.
    std::array<unsigned int, nativeSceneCount> outgoing{};
    std::array<unsigned int, nativeSceneCount> incoming{};
    constexpr std::array<int, 2> coverageOffsets{1, 7};
    constexpr std::array<float, 3> coverageProgress{0.0f, 0.5f, 1.0f};
    std::size_t coveragePaths = 0;
    for (std::size_t sourceIndex = 0;
         sourceIndex < nativeSceneCount; ++sourceIndex) {
        const NativeSceneKind source
            = static_cast<NativeSceneKind>(sourceIndex);
        for (const int offset : coverageOffsets) {
            const NativeSceneKind target = nativeSceneOffset(source, offset);
            assert(source != target);
            const NativeTransitionStyle style
                = nativeTransitionStyle(source, target);
            std::vector<unsigned char> first;
            std::vector<unsigned char> middle;
            std::vector<unsigned char> last;
            for (std::size_t progressIndex = 0;
                 progressIndex < coverageProgress.size(); ++progressIndex) {
                const float mix = coverageProgress[progressIndex];
                const NativeSceneMaterial sourceMaterial
                    = nativeSceneMaterial(source);
                const NativeSceneMaterial targetMaterial
                    = nativeSceneMaterial(target);
                LiveCompositorFrame frame;
                frame.sourceTexture = renderer.texture(source);
                frame.nextTexture = renderer.texture(target);
                frame.coverTexture = black;
                frame.width = width;
                frame.height = height;
                frame.sceneMix = mix;
                frame.transitionMode = static_cast<int>(style);
                frame.fieldExposure = sourceMaterial.fieldExposure * (1.0f - mix)
                                    + targetMaterial.fieldExposure * mix;
                frame.asciiExposure = sourceMaterial.asciiExposure * (1.0f - mix)
                                    + targetMaterial.asciiExposure * mix;
                frame.nativeRenderer = true;
                frame.visibility = 1.0f;
                assert(compositor.render(frame, error));
                glFinish();
                std::vector<unsigned char> pixels = readFrame();
                if (progressIndex == 0) first = pixels;
                if (progressIndex == 1) middle = pixels;
                if (progressIndex == 2) last = pixels;
                if (!output.empty()) {
                    const std::string name = "coverage-"
                        + std::string(nativeSceneDefinition(source).slug)
                        + "-to-"
                        + std::string(nativeSceneDefinition(target).slug)
                        + '-' + std::to_string(progressIndex) + ".ppm";
                    assert(writePpm(output / name, pixels));
                }
            }
            const float firstHalf = meanDifference(first, middle);
            const float secondHalf = meanDifference(middle, last);
            if (firstHalf <= 0.002f || secondHalf <= 0.002f) {
                std::cerr << "weak transition coverage "
                          << nativeSceneName(source) << " -> "
                          << nativeSceneName(target) << " mode="
                          << static_cast<int>(style) << " halves="
                          << firstHalf << ',' << secondHalf << '\n';
                return 1;
            }
            ++outgoing[sourceIndex];
            ++incoming[static_cast<std::size_t>(target)];
            ++coveragePaths;
        }
    }
    for (std::size_t index = 0; index < nativeSceneCount; ++index) {
        assert(outgoing[index] >= 2);
        assert(incoming[index] >= 2);
    }
    std::cout << "transition coverage scenes=" << nativeSceneCount
              << " paths=" << coveragePaths
              << " incoming_per_scene=2 outgoing_per_scene=2\n";

    compositor.shutdown();
    renderer.shutdown();
    glDeleteTextures(1, &black);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "native transitions passed\n";
    return 0;
}
