#include "native_renderer.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {
constexpr int width = 320;
constexpr int height = 180;

struct LatencyResult {
    int peakFrame = -1;
    float peakDifference = 0.0f;
};

struct ContinuityResult {
    float minimumSimilarity = 1.0f;
    float meanSimilarity = 1.0f;
    float minimumLandmarkContrast = 1000.0f;
    int minimumFrame = -1;
};

struct SceneAudit {
    float idleMean = 0.0f;
    float kickRatio = 0.0f;
    float snareRatio = 0.0f;
    float hatRatio = 0.0f;
    float moderateKickRatio = 0.0f;
    float moderateSnareRatio = 0.0f;
    float moderateHatRatio = 0.0f;
    float beatRatio = 0.0f;
    float phraseMotionRatio = 0.0f;
    float kickSnareSimilarity = 1.0f;
    float kickHatSimilarity = 1.0f;
    float snareHatSimilarity = 1.0f;
    float anticipationRatio = 0.0f;
    float downbeatRatio = 0.0f;
    float meanChroma = 0.0f;
    LatencyResult kickLatency;
    LatencyResult snareLatency;
    LatencyResult hatLatency;
    ContinuityResult continuity;
};

std::vector<float> readTexture(GLuint texture, int textureWidth,
                               int textureHeight) {
    std::vector<float> pixels(textureWidth * textureHeight * 4);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
    return pixels;
}

std::vector<float> readTexture(GLuint texture) {
    return readTexture(texture, width, height);
}

std::vector<float> luminance(const std::vector<float>& pixels) {
    std::vector<float> result(width * height);
    for (std::size_t pixel = 0; pixel < result.size(); ++pixel) {
        const std::size_t offset = pixel * 4;
        result[pixel] = 0.299f * pixels[offset]
                      + 0.587f * pixels[offset + 1]
                      + 0.114f * pixels[offset + 2];
    }
    return result;
}

std::vector<float> luminanceDifference(const std::vector<float>& before,
                                       const std::vector<float>& after) {
    const std::vector<float> beforeLight = luminance(before);
    const std::vector<float> afterLight = luminance(after);
    std::vector<float> difference(beforeLight.size());
    for (std::size_t pixel = 0; pixel < difference.size(); ++pixel) {
        difference[pixel] = std::abs(afterLight[pixel] - beforeLight[pixel]);
    }
    return difference;
}

float mean(const std::vector<float>& values) {
    float sum = 0.0f;
    for (const float value : values) sum += value;
    return sum / std::max<std::size_t>(1, values.size());
}

float similarity(const std::vector<float>& a, const std::vector<float>& b) {
    float dot = 0.0f;
    float aLength = 0.0f;
    float bLength = 0.0f;
    for (std::size_t index = 0; index < a.size(); ++index) {
        dot += a[index] * b[index];
        aLength += a[index] * a[index];
        bLength += b[index] * b[index];
    }
    return dot / std::sqrt(std::max(1e-12f, aLength * bLength));
}

float meanChroma(const std::vector<float>& pixels) {
    float total = 0.0f;
    int visiblePixels = 0;
    for (std::size_t offset = 0; offset + 3 < pixels.size(); offset += 4) {
        const float maximum = std::max({pixels[offset], pixels[offset + 1],
                                        pixels[offset + 2]});
        if (maximum < 0.015f) continue;
        const float minimum = std::min({pixels[offset], pixels[offset + 1],
                                        pixels[offset + 2]});
        total += (maximum - minimum) / maximum;
        ++visiblePixels;
    }
    return total / std::max(1, visiblePixels);
}

bool writeReferencePpm(const std::filesystem::path& path,
                       const std::vector<float>& pixels, int textureWidth,
                       int textureHeight, float exposure) {
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    output << "P6\n" << textureWidth << " " << textureHeight << "\n255\n";
    for (int y = textureHeight - 1; y >= 0; --y) {
        for (int x = 0; x < textureWidth; ++x) {
            const std::size_t offset
                = static_cast<std::size_t>(y * textureWidth + x) * 4;
            for (int channel = 0; channel < 3; ++channel) {
                const float linear = pixels[offset + channel];
                const float mapped = linear / (1.0f + linear * 0.85f);
                const float exposed = std::clamp(mapped * exposure, 0.0f, 1.0f);
                const auto value = static_cast<unsigned char>(
                    std::pow(exposed, 1.0f / 2.2f) * 255.0f + 0.5f);
                output.write(reinterpret_cast<const char*>(&value), 1);
            }
        }
    }
    return static_cast<bool>(output);
}

float smoothStep(float edge0, float edge1, float value) {
    const float position = std::clamp(
        (value - edge0) / std::max(1e-7f, edge1 - edge0), 0.0f, 1.0f);
    return position * position * (3.0f - 2.0f * position);
}

std::array<float, 3> displaySample(const std::vector<float>& pixels,
                                   int textureWidth, int textureHeight,
                                   int x, int y) {
    const int sourceX = std::clamp(x * textureWidth / 1280,
                                   0, textureWidth - 1);
    const int sourceY = std::clamp(y * textureHeight / 720,
                                   0, textureHeight - 1);
    const std::size_t offset
        = static_cast<std::size_t>(sourceY * textureWidth + sourceX) * 4;
    std::array<float, 3> sample{};
    for (int channel = 0; channel < 3; ++channel) {
        const float linear = pixels[offset + channel];
        sample[channel] = linear / (1.0f + linear * 0.85f);
    }
    const float light = 0.299f * sample[0] + 0.587f * sample[1]
                      + 0.114f * sample[2];
    for (float& channel : sample) {
        channel = std::clamp(light + (channel - light) * 1.34f, 0.0f, 1.0f);
    }
    return sample;
}

bool writeAsciiReferencePpm(const std::filesystem::path& path,
                            const std::vector<float>& pixels,
                            int textureWidth, int textureHeight,
                            float exposure) {
    constexpr int outputWidth = 1280;
    constexpr int outputHeight = 720;
    constexpr int cellWidth = 12;
    constexpr int cellHeight = 24;
    constexpr std::array<float, 8> thresholds{
        0.08f, 0.58f, 0.33f, 0.83f, 0.70f, 0.20f, 0.95f, 0.45f};
    struct DotSample {
        std::array<float, 3> color{};
        float level = 0.0f;
        bool active = false;
    };
    constexpr int cellsAcross = (outputWidth + cellWidth - 1) / cellWidth;
    constexpr int cellsDown = (outputHeight + cellHeight - 1) / cellHeight;
    std::array<DotSample, cellsAcross * cellsDown * 8> dots{};
    for (int cellY = 0; cellY < cellsDown; ++cellY) {
        for (int cellX = 0; cellX < cellsAcross; ++cellX) {
            for (int dotY = 0; dotY < 4; ++dotY) {
                for (int dotX = 0; dotX < 2; ++dotX) {
                    DotSample& dot = dots[(cellY * cellsAcross + cellX) * 8
                                         + dotY * 2 + dotX];
                    const int dotOriginX = cellX * cellWidth + dotX * 6;
                    const int dotOriginY = cellY * cellHeight + dotY * 6;
                    float best = 0.0f;
                    for (int offsetY = 0; offsetY < 6; ++offsetY) {
                        for (int offsetX = 0; offsetX < 6; ++offsetX) {
                            const auto candidate = displaySample(
                                pixels, textureWidth, textureHeight,
                                dotOriginX + offsetX,
                                dotOriginY + offsetY);
                            const float sampleLight = 0.299f * candidate[0]
                                                    + 0.587f * candidate[1]
                                                    + 0.114f * candidate[2];
                            if (sampleLight > best) {
                                best = sampleLight;
                                dot.color = candidate;
                            }
                        }
                    }
                    dot.level = std::min(1.0f,
                        std::sqrt(std::max(best, 0.0f)) * 1.25f
                        * std::sqrt(exposure));
                    dot.level = std::floor(dot.level * 5.0f + 0.5f) / 5.0f;
                    dot.active = dot.level >= thresholds[dotY * 2 + dotX];
                }
            }
        }
    }
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    output << "P6\n" << outputWidth << " " << outputHeight << "\n255\n";
    for (int y = outputHeight - 1; y >= 0; --y) {
        for (int x = 0; x < outputWidth; ++x) {
            const int localX = x % cellWidth;
            const int localY = y % cellHeight;
            const int dotX = localX < 6 ? 0 : 1;
            const int dotY = std::clamp(localY / 6, 0, 3);
            const float centerX = dotX == 0 ? 3.0f : 9.0f;
            const float centerY = 3.0f + dotY * 6.0f;
            const float distance = std::hypot(localX - centerX,
                                              localY - centerY);
            const float glyphMask = 1.0f - smoothStep(1.42f, 2.38f, distance);
            const DotSample& dot = dots[((y / cellHeight) * cellsAcross
                                        + x / cellWidth) * 8
                                        + dotY * 2 + dotX];
            const float lightResponse = 0.80f + dot.level * 0.32f;
            for (int channel = 0; channel < 3; ++channel) {
                const float linear = dot.active
                    ? dot.color[channel] * lightResponse * exposure * glyphMask
                    : 0.0f;
                const float mapped = std::clamp(linear, 0.0f, 1.0f);
                const auto value = static_cast<unsigned char>(
                    std::pow(mapped, 1.0f / 2.2f) * 255.0f + 0.5f);
                output.write(reinterpret_cast<const char*>(&value), 1);
            }
        }
    }
    return static_cast<bool>(output);
}

MusicFrame baseMusic() {
    MusicFrame music;
    music.bandLevel.fill(1.0f);
    music.spectrumLevel.fill(1.0f);
    music.harmonic = 0.55f;
    music.percussive = 0.28f;
    music.energyFast = 0.46f;
    music.energySlow = 0.42f;
    music.clockConfidence = 0.85f;
    music.beatPhase = 0.32f;
    music.barPhase = 0.58f;
    music.phrasePhase = 0.25f;
    return music;
}

NativeSceneState baseScene(NativeSceneKind kind) {
    NativeSceneState scene;
    scene.development = 0.72f;
    scene.drive = 0.52f;
    scene.peak = 0.24f;
    scene.sceneBeats = 12.0f;
    scene.currentScene = kind;
    scene.incomingScene = nativeSceneOffset(kind, 1);
    return scene;
}

std::vector<float> renderGesture(NativeRenderer& renderer, MusicFrame gesture,
                                 NativeSceneKind kind, std::string& error,
                                 const NativeRenderPolicy& policy = {}) {
    renderer.reset();
    MusicFrame music = baseMusic();
    for (int frame = 0; frame < 75; ++frame) {
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error, policy)) return {};
    }
    const std::vector<float> before = readTexture(renderer.texture(kind));
    if (!renderer.render(gesture, baseScene(kind), width, height,
                         {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                         1.0f / 60.0f, error, policy)) return {};
    return luminanceDifference(before, readTexture(renderer.texture(kind)));
}

std::vector<float> renderTarget(NativeRenderer& renderer, const MusicFrame& target,
                                NativeSceneKind kind, std::string& error) {
    renderer.reset();
    MusicFrame music = baseMusic();
    for (int frame = 0; frame < 75; ++frame) {
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return {};
    }
    if (!renderer.render(target, baseScene(kind), width, height,
                         {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                         1.0f / 60.0f, error)) return {};
    return readTexture(renderer.texture(kind));
}

std::vector<std::vector<float>> renderBaseline(NativeRenderer& renderer,
                                               int frameCount,
                                               NativeSceneKind kind,
                                               std::string& error) {
    renderer.reset();
    MusicFrame music = baseMusic();
    for (int frame = 0; frame < 75; ++frame) {
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return {};
    }
    std::vector<std::vector<float>> frames;
    frames.push_back(readTexture(renderer.texture(kind)));
    for (int frame = 0; frame < frameCount; ++frame) {
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return {};
        frames.push_back(readTexture(renderer.texture(kind)));
    }
    return frames;
}

float measurePhraseMotion(NativeRenderer& renderer, NativeSceneKind kind,
                          bool musical, std::string& error) {
    renderer.reset();
    MusicFrame music = baseMusic();
    for (int frame = 0; frame < 75; ++frame) {
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return 0.0f;
    }
    std::vector<float> previous = readTexture(renderer.texture(kind));
    float total = 0.0f;
    for (int frame = 0; frame < 120; ++frame) {
        music.beatPhase = std::fmod(frame / 30.0f, 1.0f);
        music.barPhase = std::fmod(frame / 120.0f, 1.0f);
        if (musical) {
            const int beatFrame = frame % 30;
            const int snareFrame = (frame + 15) % 60;
            const int hatFrame = frame % 15;
            music.kick = beatFrame < 8
                ? 0.28f * std::exp(-beatFrame * 6.5f / 60.0f) : 0.0f;
            music.snare = snareFrame < 7
                ? 0.24f * std::exp(-snareFrame * 8.0f / 60.0f) : 0.0f;
            music.hat = hatFrame < 4
                ? 0.20f * std::exp(-hatFrame * 13.0f / 60.0f) : 0.0f;
        } else {
            music.kick = 0.0f;
            music.snare = 0.0f;
            music.hat = 0.0f;
        }
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return 0.0f;
        const std::vector<float> current = readTexture(renderer.texture(kind));
        total += mean(luminanceDifference(previous, current));
        previous = current;
    }
    return total / 120.0f;
}

float measureAmbientMotion(NativeRenderer& renderer, NativeSceneKind kind,
                           const NativeRenderPolicy& policy,
                           std::string& error) {
    renderer.reset();
    MusicFrame music = baseMusic();
    music.kick = 0.0f;
    music.snare = 0.0f;
    music.hat = 0.0f;
    music.beatPulse = 0.0f;
    music.onsetPulse = 0.0f;
    music.downbeat = 0.0f;
    music.section = 0.0f;
    music.clockConfidence = 0.0f;
    for (int frame = 0; frame < 90; ++frame) {
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error, policy)) return 0.0f;
    }
    std::vector<float> previous = readTexture(renderer.texture(kind));
    float total = 0.0f;
    for (int frame = 0; frame < 90; ++frame) {
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error, policy)) return 0.0f;
        const std::vector<float> current = readTexture(renderer.texture(kind));
        total += mean(luminanceDifference(previous, current));
        previous = current;
    }
    return total / 90.0f;
}

LatencyResult measureLatency(NativeRenderer& renderer,
                             const std::vector<std::vector<float>>& baseline,
                             int role, NativeSceneKind kind, std::string& error) {
    renderer.reset();
    MusicFrame music = baseMusic();
    for (int frame = 0; frame < 75; ++frame) {
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return {};
    }
    LatencyResult result;
    float envelope = 0.0f;
    std::vector<float> previousGesture = readTexture(renderer.texture(kind));
    for (int frame = 0; frame + 1 < static_cast<int>(baseline.size()); ++frame) {
        if (frame == 0) envelope = 1.15f;
        if (role == 0) music.kick = envelope;
        if (role == 1) music.snare = envelope;
        if (role == 2) music.hat = envelope;
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return {};
        const std::vector<float> currentGesture = readTexture(renderer.texture(kind));
        const float gestureMotion = mean(luminanceDifference(
            previousGesture, currentGesture));
        const float baselineMotion = mean(luminanceDifference(
            baseline[frame], baseline[frame + 1]));
        const float difference = std::max(0.0f, gestureMotion - baselineMotion);
        if (difference > result.peakDifference) {
            result.peakDifference = difference;
            result.peakFrame = frame;
        }
        envelope *= std::exp(-(role == 2 ? 13.0f : role == 1 ? 8.0f : 6.5f)
                             / 60.0f);
        previousGesture = currentGesture;
    }
    return result;
}

float apertureContrast(const std::vector<float>& light) {
    float centerSum = 0.0f;
    float ringSum = 0.0f;
    int centerCount = 0;
    int ringCount = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float dx = (x + 0.5f - width * 0.5f) / height;
            const float dy = (y + 0.5f - height * 0.5f) / height;
            const float radius = std::sqrt(dx * dx + dy * dy);
            if (radius < 0.055f) {
                centerSum += light[y * width + x];
                ++centerCount;
            } else if (radius >= 0.10f && radius < 0.19f) {
                ringSum += light[y * width + x];
                ++ringCount;
            }
        }
    }
    const float centerMean = centerSum / std::max(1, centerCount);
    const float ringMean = ringSum / std::max(1, ringCount);
    return ringMean / std::max(1e-6f, centerMean);
}

float filamentContrast(const std::vector<float>& light) {
    float corridorRidgeSum = 0.0f;
    float outsideRidgeSum = 0.0f;
    int rowCount = 0;
    for (int y = height / 8; y < height * 7 / 8; ++y) {
        float corridorRidge = 0.0f;
        float outsideRidge = 0.0f;
        for (int x = 0; x < width; ++x) {
            const float px = (x + 0.5f - width * 0.5f) / height;
            if (std::abs(px) < 0.22f) {
                corridorRidge = std::max(corridorRidge, light[y * width + x]);
            } else if (std::abs(px) < 0.64f) {
                outsideRidge = std::max(outsideRidge, light[y * width + x]);
            }
        }
        corridorRidgeSum += corridorRidge;
        outsideRidgeSum += outsideRidge;
        ++rowCount;
    }
    const float corridorRidgeMean = corridorRidgeSum / std::max(1, rowCount);
    const float outsideRidgeMean = outsideRidgeSum / std::max(1, rowCount);
    return corridorRidgeMean / std::max(1e-6f, outsideRidgeMean);
}

float featureContrast(const std::vector<float>& light) {
    std::vector<float> ordered = light;
    const std::size_t percentile = ordered.size() * 99 / 100;
    std::nth_element(ordered.begin(), ordered.begin() + percentile, ordered.end());
    return ordered[percentile] / std::max(1e-6f, mean(light));
}

float landmarkContrast(const std::vector<float>& light, NativeSceneKind kind) {
    float spatialContrast = featureContrast(light);
    switch (kind) {
        case NativeSceneKind::DepthTunnel:
        case NativeSceneKind::Centrifuge:
        case NativeSceneKind::OrbitalLoom:
        case NativeSceneKind::PulseCathedral:
        case NativeSceneKind::BloomEngine:
            spatialContrast = std::max(spatialContrast, apertureContrast(light));
            break;
        case NativeSceneKind::WireOrganism:
            spatialContrast = std::max(spatialContrast, filamentContrast(light));
            break;
        case NativeSceneKind::PrismGarden:
        case NativeSceneKind::TidalGrid:
        case NativeSceneKind::ConstellationField:
        case NativeSceneKind::SpectralRibbons:
        case NativeSceneKind::NegativeSpace:
        case NativeSceneKind::InkCurrent:
        case NativeSceneKind::GlassChoir:
        case NativeSceneKind::ShadowArchitecture:
        case NativeSceneKind::ParticleWeave:
        case NativeSceneKind::LivingMosaic:
        case NativeSceneKind::LumenFold:
        case NativeSceneKind::PaperHorizon:
            break;
    }
    return spatialContrast;
}

ContinuityResult measureContinuity(NativeRenderer& renderer, NativeSceneKind kind,
                                   std::string& error) {
    renderer.reset();
    MusicFrame music = baseMusic();
    std::vector<float> previous;
    ContinuityResult result;
    float similaritySum = 0.0f;
    int comparisons = 0;
    for (int frame = 0; frame < 180; ++frame) {
        music.beatPhase = std::fmod(frame / 30.0f, 1.0f);
        music.barPhase = std::fmod(frame / 120.0f, 1.0f);
        music.phrasePhase = std::fmod(frame / 480.0f, 1.0f);
        music.beatPulse = frame % 30 < 10
            ? std::exp(-(frame % 30) * 9.0f / 60.0f) : 0.0f;
        music.onsetPulse = frame % 45 >= 10 && frame % 45 < 16
            ? std::exp(-(frame % 45 - 10) * 18.0f / 60.0f) : 0.0f;
        music.kick = frame % 30 < 8 ? std::exp(-(frame % 30) * 6.5f / 60.0f) : 0.0f;
        music.snare = frame % 60 >= 30 && frame % 60 < 38
            ? std::exp(-(frame % 60 - 30) * 8.0f / 60.0f) : 0.0f;
        music.hat = frame % 15 < 5 ? std::exp(-(frame % 15) * 13.0f / 60.0f) : 0.0f;
        if (!renderer.render(music, baseScene(kind), width, height,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return {};
        const std::vector<float> current = luminance(readTexture(renderer.texture(kind)));
        if (frame >= 75) {
            result.minimumLandmarkContrast = std::min(
                result.minimumLandmarkContrast, landmarkContrast(current, kind));
            if (!previous.empty()) {
                const float value = similarity(previous, current);
                if (value < result.minimumSimilarity) {
                    result.minimumSimilarity = value;
                    result.minimumFrame = frame;
                }
                similaritySum += value;
                ++comparisons;
            }
        }
        previous = current;
    }
    result.meanSimilarity = similaritySum / std::max(1, comparisons);
    return result;
}

SceneAudit auditScene(NativeRenderer& renderer, NativeSceneKind kind,
                      std::string& error) {
    MusicFrame idle = baseMusic();
    MusicFrame kick = baseMusic();
    MusicFrame snare = baseMusic();
    MusicFrame hat = baseMusic();
    MusicFrame lateBeat = baseMusic();
    MusicFrame anticipation = baseMusic();
    MusicFrame regularBeat = baseMusic();
    MusicFrame downbeat = baseMusic();
    MusicFrame moderateKick = baseMusic();
    MusicFrame moderateSnare = baseMusic();
    MusicFrame moderateHat = baseMusic();
    MusicFrame beat = baseMusic();
    kick.kick = 1.15f;
    snare.snare = 1.15f;
    hat.hat = 1.15f;
    hat.spectrumLevel[28] = 1.8f;
    moderateKick.kick = 0.28f;
    moderateSnare.snare = 0.24f;
    moderateHat.hat = 0.20f;
    beat.beatPulse = 0.85f;
    lateBeat.beatPhase = 0.92f;
    anticipation = lateBeat;
    anticipation.beatAnticipation = 0.85f;
    regularBeat.beatPhase = 0.02f;
    downbeat = regularBeat;
    downbeat.downbeat = 1.0f;

    const auto idleMotion = renderGesture(renderer, idle, kind, error);
    const auto kickMotion = renderGesture(renderer, kick, kind, error);
    const auto snareMotion = renderGesture(renderer, snare, kind, error);
    const auto hatMotion = renderGesture(renderer, hat, kind, error);
    const auto lateBeatFrame = renderTarget(renderer, lateBeat, kind, error);
    const auto anticipationFrame = renderTarget(renderer, anticipation, kind, error);
    const auto regularBeatFrame = renderTarget(renderer, regularBeat, kind, error);
    const auto downbeatFrame = renderTarget(renderer, downbeat, kind, error);
    const auto moderateKickMotion = renderGesture(
        renderer, moderateKick, kind, error);
    const auto moderateSnareMotion = renderGesture(
        renderer, moderateSnare, kind, error);
    const auto moderateHatMotion = renderGesture(
        renderer, moderateHat, kind, error);
    const auto beatMotion = renderGesture(renderer, beat, kind, error);
    if (idleMotion.empty() || kickMotion.empty() || snareMotion.empty()
        || hatMotion.empty() || lateBeatFrame.empty() || anticipationFrame.empty()
        || regularBeatFrame.empty() || downbeatFrame.empty()
        || beatMotion.empty()) return {};

    SceneAudit audit;
    audit.idleMean = mean(idleMotion);
    audit.kickRatio = mean(kickMotion) / std::max(1e-7f, audit.idleMean);
    audit.snareRatio = mean(snareMotion) / std::max(1e-7f, audit.idleMean);
    audit.hatRatio = mean(hatMotion) / std::max(1e-7f, audit.idleMean);
    audit.moderateKickRatio = mean(moderateKickMotion)
                            / std::max(1e-7f, audit.idleMean);
    audit.moderateSnareRatio = mean(moderateSnareMotion)
                             / std::max(1e-7f, audit.idleMean);
    audit.moderateHatRatio = mean(moderateHatMotion)
                           / std::max(1e-7f, audit.idleMean);
    audit.beatRatio = mean(beatMotion) / std::max(1e-7f, audit.idleMean);
    const float idlePhraseMotion = measurePhraseMotion(
        renderer, kind, false, error);
    const float musicalPhraseMotion = measurePhraseMotion(
        renderer, kind, true, error);
    audit.phraseMotionRatio = musicalPhraseMotion
                            / std::max(1e-7f, idlePhraseMotion);
    audit.kickSnareSimilarity = similarity(kickMotion, snareMotion);
    audit.kickHatSimilarity = similarity(kickMotion, hatMotion);
    audit.snareHatSimilarity = similarity(snareMotion, hatMotion);
    audit.anticipationRatio = mean(luminanceDifference(
        lateBeatFrame, anticipationFrame)) / std::max(1e-7f, audit.idleMean);
    audit.downbeatRatio = mean(luminanceDifference(
        regularBeatFrame, downbeatFrame)) / std::max(1e-7f, audit.idleMean);
    audit.meanChroma = meanChroma(regularBeatFrame);
    const auto baseline = renderBaseline(renderer, 24, kind, error);
    if (baseline.empty()) return {};
    audit.kickLatency = measureLatency(renderer, baseline, 0, kind, error);
    audit.snareLatency = measureLatency(renderer, baseline, 1, kind, error);
    audit.hatLatency = measureLatency(renderer, baseline, 2, kind, error);
    audit.continuity = measureContinuity(renderer, kind, error);
    return audit;
}

void printAudit(NativeSceneKind kind, const SceneAudit& audit) {
    std::cout << "native " << nativeSceneName(kind)
              << " gestures idle=" << audit.idleMean
              << " kick=" << audit.kickRatio
              << " snare=" << audit.snareRatio
              << " hat=" << audit.hatRatio
              << " moderate=" << audit.moderateKickRatio << ","
              << audit.moderateSnareRatio << "," << audit.moderateHatRatio
              << " beat=" << audit.beatRatio
              << " phrase=" << audit.phraseMotionRatio
              << " similarity=" << audit.kickSnareSimilarity << ","
              << audit.kickHatSimilarity << "," << audit.snareHatSimilarity
              << " anticipation=" << audit.anticipationRatio
              << " downbeat=" << audit.downbeatRatio
              << " chroma=" << audit.meanChroma
              << " latency_frames=" << audit.kickLatency.peakFrame << ","
              << audit.snareLatency.peakFrame << ","
              << audit.hatLatency.peakFrame
              << " continuity=" << audit.continuity.minimumSimilarity << ","
              << audit.continuity.meanSimilarity
              << " continuity_frame=" << audit.continuity.minimumFrame
              << " landmark=" << audit.continuity.minimumLandmarkContrast << "\n";
}

bool auditPasses(const SceneAudit& audit) {
    if (audit.kickRatio < 1.25f || audit.snareRatio < 1.25f
        || audit.hatRatio < 1.12f) return false;
    if (audit.moderateKickRatio < 1.15f
        || audit.moderateSnareRatio < 1.15f
        || audit.moderateHatRatio < 1.08f || audit.beatRatio < 1.50f
        || audit.phraseMotionRatio < 1.18f) return false;
    if (audit.kickSnareSimilarity > 0.97f || audit.kickHatSimilarity > 0.97f
        || audit.snareHatSimilarity > 0.97f) return false;
    if (audit.anticipationRatio < 0.10f || audit.downbeatRatio < 0.10f
        || audit.meanChroma < 0.12f) return false;
    if (audit.kickLatency.peakFrame < 0 || audit.kickLatency.peakFrame > 6
        || audit.snareLatency.peakFrame < 0 || audit.snareLatency.peakFrame > 6
        || audit.hatLatency.peakFrame < 0 || audit.hatLatency.peakFrame > 6) {
        return false;
    }
    // Beat-first scenes deliberately make a large, localized silhouette
    // change on the attack. Judge the complete phrase by its high mean
    // similarity while allowing that one readable frame to move farther.
    return audit.continuity.minimumSimilarity >= 0.70f
        && audit.continuity.meanSimilarity >= 0.94f
        && audit.continuity.minimumLandmarkContrast >= 1.35f;
}

bool captureReference(NativeRenderer& renderer, NativeSceneKind kind,
                      const std::filesystem::path& outputDirectory,
                      const std::array<float, 3>& albumColor,
                      std::string& error) {
    renderer.reset();
    constexpr int referenceWidth = 1280;
    constexpr int referenceHeight = 720;
    MusicFrame music = baseMusic();
    for (int frame = 0; frame < 120; ++frame) {
        music.beatPhase = std::fmod(frame / 30.0f, 1.0f);
        music.barPhase = std::fmod(frame / 120.0f, 1.0f);
        music.kick = frame % 30 < 5
            ? std::exp(-(frame % 30) * 6.5f / 60.0f) : 0.0f;
        music.snare = frame % 60 >= 30 && frame % 60 < 35
            ? std::exp(-(frame % 60 - 30) * 8.0f / 60.0f) : 0.0f;
        music.hat = frame % 15 < 4
            ? std::exp(-(frame % 15) * 13.0f / 60.0f) : 0.0f;
        if (!renderer.render(music, baseScene(kind), referenceWidth,
                             referenceHeight,
                             albumColor, 0, 1.0f,
                             1.0f / 60.0f, error)) return false;
    }
    std::filesystem::create_directories(outputDirectory);
    std::filesystem::create_directories(outputDirectory / "ascii");
    const std::string slug(nativeSceneDefinition(kind).slug);
    const std::vector<float> pixels = readTexture(
        renderer.texture(kind), referenceWidth, referenceHeight);
    const NativeSceneMaterial material = nativeSceneMaterial(kind);
    return writeReferencePpm(outputDirectory / (slug + ".ppm"), pixels,
                             referenceWidth, referenceHeight,
                             material.fieldExposure)
        && writeAsciiReferencePpm(outputDirectory / "ascii" / (slug + ".ppm"),
                                  pixels, referenceWidth, referenceHeight,
                                  material.asciiExposure);
}

std::optional<std::array<float, 3>> referenceColorFromEnvironment() {
    const char* configured = std::getenv("OMADROP_REFERENCE_COLOR");
    if (!configured) return std::array<float, 3>{0.46f, 0.72f, 1.0f};
    const std::string hex(configured);
    if (hex.size() != 7 || hex[0] != '#') return std::nullopt;
    try {
        const unsigned long value = std::stoul(hex.substr(1), nullptr, 16);
        return std::array<float, 3>{
            ((value >> 16) & 255) / 255.0f,
            ((value >> 8) & 255) / 255.0f,
            (value & 255) / 255.0f};
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<float> renderArtworkFrame(NativeRenderer& renderer,
                                      GLuint artworkTexture,
                                      std::string& error) {
    renderer.reset();
    MusicFrame music = baseMusic();
    NativeSceneState scene = baseScene(NativeSceneKind::DepthTunnel);
    scene.development = 0.12f;
    for (int frame = 0; frame < 90; ++frame) {
        if (!renderer.render(music, scene, width, height,
                             {0.46f, 0.72f, 1.0f}, artworkTexture, 1.0f,
                             1.0f / 60.0f, error)) return {};
    }
    return readTexture(renderer.texture(NativeSceneKind::DepthTunnel));
}

float measureFrameMilliseconds(NativeRenderer& renderer, NativeSceneKind kind,
                               std::string& error) {
    constexpr int performanceWidth = 1280;
    constexpr int performanceHeight = 720;
    constexpr int warmupFrames = 30;
    constexpr int measuredFrames = 120;
    MusicFrame music = baseMusic();
    NativeSceneState scene = baseScene(kind);
    renderer.reset();
    for (int frame = 0; frame < warmupFrames; ++frame) {
        if (!renderer.render(music, scene, performanceWidth, performanceHeight,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return 1000.0f;
    }
    glFinish();
    const auto started = std::chrono::steady_clock::now();
    for (int frame = 0; frame < measuredFrames; ++frame) {
        music.beatPhase = std::fmod(frame / 30.0f, 1.0f);
        if (!renderer.render(music, scene, performanceWidth, performanceHeight,
                             {0.46f, 0.72f, 1.0f}, 0, 1.0f,
                             1.0f / 60.0f, error)) return 1000.0f;
    }
    glFinish();
    const auto elapsed = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    return elapsed / measuredFrames;
}
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: native-renderer-test SHADER_DIRECTORY [OUTPUT_DIRECTORY]\n";
        return 2;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Omadrop native renderer test", 0, 0,
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
    const auto referenceColor = referenceColorFromEnvironment();
    if (!referenceColor) {
        std::cerr << "OMADROP_REFERENCE_COLOR must be #RRGGBB\n";
        return 2;
    }

    std::array<SceneAudit, nativeSceneCount> audits{};
    for (std::size_t index = 0; index < nativeSceneCount; ++index) {
        const NativeSceneKind scene = static_cast<NativeSceneKind>(index);
        audits[index] = auditScene(renderer, scene, error);
    }
    if (!error.empty()) std::cerr << error << "\n";
    for (std::size_t index = 0; index < nativeSceneCount; ++index) {
        printAudit(static_cast<NativeSceneKind>(index), audits[index]);
    }

    const NativeRenderPolicy normalPolicy;
    const NativeRenderPolicy reducedPolicy{
        .motion = 1.0f,
        .reducedMotion = true,
    };
    for (const NativeSceneKind scene : {
             NativeSceneKind::DepthTunnel,
             NativeSceneKind::SpectralRibbons}) {
        const float normalAmbient = measureAmbientMotion(
            renderer, scene, normalPolicy, error);
        const float reducedAmbient = measureAmbientMotion(
            renderer, scene, reducedPolicy, error);
        std::cout << "native " << nativeSceneName(scene)
                  << " ambient_motion=" << normalAmbient
                  << " reduced=" << reducedAmbient << "\n";
        assert(normalAmbient > 0.0f);
        assert(reducedAmbient < normalAmbient * 0.72f);
    }
    MusicFrame policyKick = baseMusic();
    policyKick.kick = 0.72f;
    const float lowIntensityResponse = mean(renderGesture(
        renderer, policyKick, NativeSceneKind::PaperHorizon, error,
        {.intensity = 0.75f}));
    const float highIntensityResponse = mean(renderGesture(
        renderer, policyKick, NativeSceneKind::PaperHorizon, error,
        {.intensity = 1.25f}));
    const float flashLimitedResponse = mean(renderGesture(
        renderer, policyKick, NativeSceneKind::PaperHorizon, error,
        {.intensity = 1.25f, .flashLimited = true}));
    std::cout << "native Paper Horizon intensity_response="
              << lowIntensityResponse << ',' << highIntensityResponse
              << " flash_limited=" << flashLimitedResponse << "\n";
    assert(lowIntensityResponse > 0.0f);
    assert(highIntensityResponse > lowIntensityResponse * 1.08f);
    assert(flashLimitedResponse < highIntensityResponse * 0.90f);
    assert(flashLimitedResponse > 0.0f);

    std::array<unsigned char, 16 * 16 * 4> checker{};
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            const std::size_t offset = static_cast<std::size_t>(y * 16 + x) * 4;
            const bool bright = ((x / 2) + (y / 2)) % 2 == 0;
            checker[offset] = bright ? 236 : 14;
            checker[offset + 1] = bright ? 104 : 22;
            checker[offset + 2] = bright ? 44 : 92;
            checker[offset + 3] = 255;
        }
    }
    GLuint checkerTexture = 0;
    glGenTextures(1, &checkerTexture);
    glBindTexture(GL_TEXTURE_2D, checkerTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16, 16, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, checker.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const auto withoutArtwork = renderArtworkFrame(renderer, 0, error);
    const auto withArtwork = renderArtworkFrame(renderer, checkerTexture, error);
    const float artworkResponse = withoutArtwork.empty() || withArtwork.empty()
        ? 0.0f : mean(luminanceDifference(withoutArtwork, withArtwork));
    std::cout << "native Depth Tunnel artwork_response=" << artworkResponse << "\n";
    bool performancePasses = true;
    for (std::size_t index = 0; index < nativeSceneCount; ++index) {
        const NativeSceneKind kind = static_cast<NativeSceneKind>(index);
        const float frameMilliseconds = measureFrameMilliseconds(
            renderer, kind, error);
        const NativeSceneDefinition& definition = nativeSceneDefinition(kind);
        std::cout << "native " << definition.name << " 720p frame_ms="
                  << frameMilliseconds << " budget_ms="
                  << definition.maximumFrameMilliseconds << " fps_capacity="
                  << 1000.0f / frameMilliseconds << "\n";
        performancePasses = performancePasses
            && frameMilliseconds <= definition.maximumFrameMilliseconds;
    }

    bool referencesWritten = true;
    if (argc == 3) {
        for (std::size_t index = 0; index < nativeSceneCount; ++index) {
            referencesWritten = referencesWritten && captureReference(
                renderer, static_cast<NativeSceneKind>(index), argv[2],
                *referenceColor, error);
        }
        if (!referencesWritten) std::cerr << "reference capture: " << error << "\n";
    }

    renderer.shutdown();
    glDeleteTextures(1, &checkerTexture);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    bool scenesPass = true;
    for (std::size_t index = 0; index < nativeSceneCount; ++index) {
        scenesPass = scenesPass && auditPasses(audits[index]);
    }
    return scenesPass && artworkResponse >= 0.0005f
        && performancePasses && referencesWritten ? 0 : 1;
}
