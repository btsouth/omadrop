#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <projectM-4/projectM.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <signal.h>
#include <unistd.h>
#include <vector>

#include "audio_features.h"
#include "audio_output_session.h"
#include "audio_queue.h"
#include "cover_presentation.h"
#include "live_assets.h"
#include "live_compositor.h"
#include "live_projectm.h"
#include "live_settings.h"
#include "mpris_poller.h"
#include "mpris_state.h"
#include "musical_structure.h"
#include "music_frame.h"
#include "native_renderer.h"
#include "paired_display.h"
#include "paired_music_state.h"
#include "paired_transport.h"
#include "pipewire_capture.h"
#include "preset_profiles.h"
#include "preset_selector.h"
#include "session_lifecycle.h"
#include "structure_timeline.h"
#include "status_overlay.h"
#include "visual_motifs.h"

namespace {
constexpr int width = 1280;
constexpr int height = 720;
volatile sig_atomic_t stopRequested = 0;

void requestStop(int) {
    stopRequested = 1;
}

const char* vertexSource = R"GLSL(
#version 330 core
out vec2 uv;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

const char* fragmentSource = R"GLSL(
#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D sourceFrame;
uniform sampler2D nextFrame;
uniform float presetMix;
uniform sampler2D coverFrame;
uniform vec2 resolution;
uniform float coverAspect;
uniform float coverMix;
uniform vec3 albumColor;
uniform float paletteInfluence;
uniform float bassLevel;
uniform float bassImpact;
uniform float midLevel;
uniform float trebleLevel;
uniform float midImpact;
uniform float trebleImpact;
uniform int asciiEnabled;
uniform int transitionMode;
uniform int sourceReactionMode;
uniform int nextReactionMode;
uniform vec3 sourceReactionGain;
uniform vec3 nextReactionGain;
uniform float asciiExposure;
uniform float fieldExposure;
uniform int nativeRenderer;
uniform float motionScale;
uniform float contrastScale;
uniform int flashLimited;
uniform int colorVisionSafe;
uniform float visibility;

float luminance(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

vec3 limitFlashBrightness(vec3 sampleColor) {
    if (flashLimited == 0) return sampleColor;
    float light = luminance(sampleColor);
    float limitedLight = 0.68 * (1.0 - exp(-light / 0.68));
    return sampleColor * limitedLight / max(0.0001, light);
}

vec3 colorSafePalette(vec3 sampleColor) {
    if (colorVisionSafe == 0) return sampleColor;
    float light = luminance(sampleColor);
    float maximum = max(sampleColor.r, max(sampleColor.g, sampleColor.b));
    float minimum = min(sampleColor.r, min(sampleColor.g, sampleColor.b));
    float chroma = maximum - minimum;
    float warm = smoothstep(-0.10, 0.12,
                            sampleColor.r + sampleColor.g * 0.20
                          - sampleColor.b * 0.92);
    vec3 cool = vec3(0.12, 0.55, 0.88);
    vec3 gold = vec3(1.00, 0.66, 0.08);
    vec3 tint = mix(cool, gold, warm);
    tint *= light / max(0.001, luminance(tint));
    // Hue is redundant with luminance in this mode. Saturation falls toward
    // black and white so the transform remains in gamut while preserving the
    // scene's brightness structure.
    float gamut = 4.0 * light * (1.0 - light);
    float amount = (0.30 + 0.58 * smoothstep(0.015, 0.20, chroma)) * gamut;
    return clamp(mix(vec3(light), tint, amount), 0.0, 1.0);
}

vec2 reactedUv(vec2 sampleUv, int mode, vec3 gain) {
    vec2 p = sampleUv - 0.5;
    float radius = max(0.001, length(p));
    float angle = atan(p.y, p.x);
    // Quiet low-end movement stays restrained. A genuinely hard transient
    // crosses into a stronger deformation of the preset's own geometry.
    float kickAccent = smoothstep(0.52, 0.82, bassImpact);
    float kick = min(2.10, (1.10 * bassImpact + 0.75 * kickAccent) * gain.x);
    float snare = midImpact * gain.y;
    float hat = trebleImpact * gain.z;
    vec2 movement = vec2(0.0);
    if (mode == 0) {
        // Contortion: kick opens tunnel depth, snare turns its existing walls.
        movement = p * (0.026 * kick)
                 + vec2(-p.y, p.x) * (0.009 * snare);
        movement += p / radius * sin(angle * 10.0 + radius * 42.0) * (0.0018 * hat);
    } else if (mode == 1) {
        // Wire Dance: separate its connected lobes instead of shaking the camera.
        movement = vec2(sign(p.x), sign(p.y)) * (0.018 * kick)
                 * smoothstep(0.05, 0.44, radius);
        movement += vec2(-p.y, p.x) * (0.010 * snare);
        movement += vec2(sin(sampleUv.y * 41.0), cos(sampleUv.x * 37.0))
                  * (0.0022 * hat);
    } else if (mode == 2) {
        // Halls of Centrifuge: a kick travels down the vanishing point.
        movement = p * (0.027 * kick * (1.0 - 0.35 * smoothstep(0.18, 0.70, radius)));
        movement += vec2(-p.y, p.x) * (0.012 * snare * (0.35 + radius));
        movement += p / radius * sin(radius * 46.0) * (0.0020 * hat);
    } else if (mode == 3) {
        // Night Cathedral: compress the corridor and flex its opposing planes.
        movement = p * vec2(0.014, 0.025) * kick;
        movement.x += sin(sampleUv.y * 10.0) * (0.013 * snare);
        movement.y += sin(sampleUv.x * 33.0) * (0.0018 * hat);
    } else if (mode == 4) {
        // Bitterfeld: fracture the crystal field along its existing facets.
        movement.x = sin(sampleUv.y * 9.0 + angle * 2.0) * (0.021 * kick);
        movement.y = sin(sampleUv.x * 8.0 - angle * 2.0) * (0.010 * snare);
        movement += vec2(cos(angle * 12.0), sin(angle * 12.0)) * (0.0022 * hat);
    } else {
        // Airhandler: bend its connected tendrils without breaking their silhouette.
        movement = vec2(sin(sampleUv.y * 8.0 + sampleUv.x * 2.0),
                        sin(sampleUv.x * 7.0 - sampleUv.y * 1.5)) * (0.014 * kick);
        movement += vec2(-p.y, p.x) * (0.010 * snare);
        movement += vec2(cos(sampleUv.y * 39.0), sin(sampleUv.x * 43.0))
                  * (0.0020 * hat);
    }
    return clamp(sampleUv + movement, vec2(0.002), vec2(0.998));
}

vec3 sceneSample(vec2 sampleUv) {
    vec2 coverSampleUv = sampleUv;
    float easedPresetMix = presetMix * presetMix * (3.0 - 2.0 * presetMix);
    float bridge = sin(3.14159265 * easedPresetMix);
    vec2 outgoingUv = (sampleUv - 0.5) * (1.0 - 0.025 * bridge) + 0.5;
    vec2 incomingUv = (sampleUv - 0.5) * (1.025 - 0.025 * easedPresetMix) + 0.5;
    bool nativeTransition = transitionMode >= 6 && transitionMode <= 10;
    if (nativeTransition) {
        outgoingUv = sampleUv;
        incomingUv = sampleUv;
        vec2 anchor = vec2(0.51, 0.50);
        if (transitionMode == 6) {
            float carry = bridge * 0.026 * motionScale;
            outgoingUv.x += carry;
            incomingUv.x -= carry;
        } else if (transitionMode == 7) {
            outgoingUv = anchor + (sampleUv - anchor)
                       * (1.0 + 0.08 * bridge * motionScale);
            incomingUv = anchor + (sampleUv - anchor)
                       * (1.0 + 0.12 * (1.0 - easedPresetMix) * motionScale);
        } else if (transitionMode == 8) {
            anchor = vec2(0.53, 0.51);
            outgoingUv = anchor + (sampleUv - anchor)
                       * (1.0 - 0.13 * bridge * motionScale);
            incomingUv = anchor + (sampleUv - anchor)
                       * (1.0 + 0.18 * (1.0 - easedPresetMix) * motionScale);
        } else if (transitionMode == 9) {
            vec2 q = sampleUv - 0.5;
            vec2 direction = normalize(vec2(
                sin((q.x + q.y * 0.72) * 17.0),
                cos((q.x * 0.61 - q.y) * 21.0)) + vec2(0.001));
            outgoingUv += direction * bridge * 0.012 * motionScale;
            incomingUv -= direction * bridge * 0.009 * motionScale;
        }
    }
    outgoingUv = reactedUv(outgoingUv, sourceReactionMode, sourceReactionGain);
    incomingUv = reactedUv(incomingUv, nextReactionMode, nextReactionGain);
    vec3 outgoing = texture(sourceFrame, outgoingUv).rgb;
    vec3 incoming = texture(nextFrame, incomingUv).rgb;

    // Do not reveal the incoming composition as one rectangular layer. Broad
    // connected flow bands let its geometry form inside the outgoing feedback
    // while a temporary luminance match prevents a sudden palette block.
    float flow;
    float localMix;
    if (transitionMode == 6) {
        flow = 0.50 + 0.18 * sin(sampleUv.y * 8.0
                               + sin(sampleUv.x * 5.0) * 1.3)
                    + 0.10 * (sampleUv.x - 0.5);
        // Keep the carry edge broad enough to feel fluid, but narrow enough
        // that two detailed scenes do not spend the middle of the transition
        // as one low-contrast double exposure.
        localMix = smoothstep(flow - 0.16, flow + 0.16, easedPresetMix);
    } else if (transitionMode == 7) {
        float radius = length(sampleUv - vec2(0.51, 0.50));
        flow = clamp(radius * 1.18, 0.06, 0.88);
        localMix = smoothstep(flow - 0.11, flow + 0.11, easedPresetMix);
    } else if (transitionMode == 8) {
        float radius = length(sampleUv - vec2(0.53, 0.51));
        flow = 0.28 + radius * 0.72
             + 0.055 * sin(radius * 31.0);
        localMix = smoothstep(flow - 0.13, flow + 0.13, easedPresetMix);
    } else if (transitionMode == 9) {
        vec2 q = sampleUv - 0.5;
        flow = 0.50
             + 0.14 * sin((q.x + q.y * 0.68) * 18.0)
             + 0.13 * sin((q.x * 0.57 - q.y) * 23.0)
             + 0.065 * sin(q.x * 37.0 + q.y * 5.0);
        localMix = smoothstep(flow - 0.09, flow + 0.09, easedPresetMix);
    } else if (transitionMode == 10) {
        vec2 q = sampleUv - vec2(0.51, 0.50);
        flow = 0.50 + 0.17 * sin(q.y * 9.0 + sin(q.x * 7.0) * 1.5)
                    + 0.10 * sin(q.x * 15.0 - q.y * 3.0)
                    + 0.08 * length(q);
        localMix = smoothstep(flow - 0.10, flow + 0.10, easedPresetMix);
    } else if (transitionMode == 0) {
        float radius = length(sampleUv - 0.5);
        flow = 0.38 + radius * 0.42 + 0.09 * sin(radius * 35.0);
        localMix = smoothstep(flow - 0.46, flow + 0.46, easedPresetMix);
    } else if (transitionMode == 1) {
        flow = 0.5 + 0.17 * sin((sampleUv.x + sampleUv.y) * 12.0)
                         + 0.07 * sin(sampleUv.y * 31.0);
        localMix = smoothstep(flow - 0.46, flow + 0.46, easedPresetMix);
    } else if (transitionMode == 2) {
        flow = 0.5 + 0.19 * sin(sampleUv.y * 10.0 + sin(sampleUv.x * 7.0) * 1.6)
                         + 0.07 * sin(sampleUv.y * 27.0 - sampleUv.x * 5.0);
        localMix = smoothstep(flow - 0.46, flow + 0.46, easedPresetMix);
    } else {
        flow = 0.5 + 0.12 * sin(sampleUv.y * 13.0 + sampleUv.x * 4.0)
                         + 0.07 * sin(sampleUv.y * 29.0 - sampleUv.x * 7.0);
        localMix = smoothstep(flow - 0.46, flow + 0.46, easedPresetMix);
    }
    localMix *= smoothstep(0.0, 0.10, easedPresetMix);
    localMix = mix(localMix, 1.0,
                   smoothstep(0.90, 1.0, easedPresetMix));
    float nativeAnchor = 0.0;
    if (transitionMode == 6 || transitionMode == 7 || transitionMode == 8) {
        float radius = length(sampleUv - vec2(0.51, 0.50));
        nativeAnchor = 1.0 - smoothstep(0.055, 0.23, radius);
        localMix = mix(localMix, easedPresetMix, nativeAnchor);
    }
    float outgoingLight = luminance(outgoing);
    float incomingLight = max(0.08, luminance(incoming));
    vec3 matchedIncoming = incoming * clamp((0.35 + outgoingLight * 0.9) / incomingLight,
                                             0.55, 1.35);
    incoming = mix(incoming, matchedIncoming, (1.0 - easedPresetMix) * 0.42);
    vec3 visual = mix(outgoing, incoming, localMix);
    if (transitionMode == 7 || transitionMode == 8) {
        visual = mix(visual, max(outgoing, incoming),
                     nativeAnchor * bridge * 0.08);
    } else if (transitionMode == 9) {
        float fractureSeam = 1.0 - smoothstep(
            0.025, 0.095, abs(easedPresetMix - flow));
        visual += mix(outgoing, incoming, 0.5) * fractureSeam * bridge * 0.06;
    } else if (transitionMode == 10) {
        float carvedSeam = 1.0 - smoothstep(
            0.018, 0.090, abs(easedPresetMix - flow));
        visual *= 1.0 - carvedSeam * bridge * 0.32;
    }
    float visualLight = luminance(visual);
    vec3 albumGrade = visual * mix(vec3(1.0), albumColor * 1.65, 0.58)
                    + albumColor * visualLight * 0.16;
    visual = mix(visual, albumGrade, paletteInfluence);
    if (nativeRenderer != 0) {
        visual = visual / (vec3(1.0) + visual * 0.85);
        float nativeLight = luminance(visual);
        visual = clamp(mix(vec3(nativeLight), visual,
                           1.34 * contrastScale), 0.0, 1.0);
    }
    visual = colorSafePalette(visual);
    if (coverMix <= 0.0) return visual;
    vec2 p = coverSampleUv - 0.5;
    float screenAspect = resolution.x / resolution.y;
    if (screenAspect > coverAspect) p.x *= screenAspect / coverAspect;
    else p.y *= coverAspect / screenAspect;
    vec2 coverUv = p + 0.5;
    vec3 cover = vec3(0.0);
    float dissolve = 1.0 - coverMix;
    // The artwork leaves in broad coordinated ribbons, not random pixels.
    // Adjacent rows travel together so the cover remains recognizable through
    // most of the transition and appears to become the feedback field.
    float ribbon = 0.5 + 0.23 * sin(coverUv.y * 17.0)
                       + 0.12 * sin(coverUv.y * 43.0 + 1.7);
    float coverPresence = smoothstep(ribbon - 0.18, ribbon + 0.18, coverMix);
    coverUv.x += dissolve * dissolve * 0.055 * sin(coverUv.y * 31.0 + coverMix * 5.0);
    if (all(greaterThanEqual(coverUv, vec2(0.0))) && all(lessThanEqual(coverUv, vec2(1.0)))) {
        cover = texture(coverFrame, vec2(coverUv.x, 1.0 - coverUv.y)).rgb;
    }
    return mix(visual, cover, coverPresence);
}

void main() {
    if (asciiEnabled == 0) {
        color = vec4(limitFlashBrightness(
            sceneSample(uv) * fieldExposure) * visibility, 1.0);
        return;
    }
    // ASCII mode keeps its identity on the cover, but starts from the same
    // full-resolution source used by continuous mode. A sharp image underlay
    // carries typography, faces, and fine illustration beneath the dots.
    vec3 cleanSource = sceneSample(uv);
    vec2 pixel = uv * resolution;
    // Deliberately coarse 2x4 braille cells. The earlier 6x12 grid preserved
    // so much source detail that it read as a slightly dotted normal image.
    vec2 cellSize = vec2(12.0, 24.0);
    vec2 cell = floor(pixel / cellSize);
    vec2 local = mod(pixel, cellSize);

    int dx = local.x < 6.0 ? 0 : 1;
    int dy = clamp(int(local.y / 6.0), 0, 3);
    vec2 center = vec2(dx == 0 ? 3.0 : 9.0, 3.0 + float(dy) * 6.0);
    // Keep transients bounded. Large glyph growth turns every lit cell into a
    // white screen on bright presets and destroys the underlying composition.
    float glyphDistance = length(local - center);
    float kickAccent = smoothstep(0.52, 0.82, bassImpact);
    float coverColorLock = smoothstep(0.0, 0.85, coverMix);
    float visualReaction = 1.0 - coverColorLock;
    // Album artwork needs more than a binary dot mask to survive difficult
    // photography. A dim, color-faithful underlay retains faces, typography,
    // and fine texture while the braille field remains the dominant material.
    // It disappears early in the cover dissolve, so presets stay pure ASCII.
    float underlayMix = smoothstep(0.62, 0.96, coverMix);
    vec3 underlaySource = cleanSource;
    float underlayLight = luminance(underlaySource);
    float underlayTarget = pow(clamp(underlayLight, 0.0, 1.0), 0.88);
    float underlayMaximum = max(underlaySource.r,
                                max(underlaySource.g, underlaySource.b));
    float underlayLift = min(underlayTarget / max(0.01, underlayLight),
                             0.98 / max(0.01, underlayMaximum));
    vec3 coverUnderlay = underlaySource * underlayLift * 0.70 * underlayMix;
    float glyphRadius = min(3.05, 1.90 + visualReaction
                          * (0.52 * bassImpact + 0.70 * kickAccent
                             + 0.09 * midImpact));
    float glyphMask = 1.0 - smoothstep(glyphRadius - 0.48, glyphRadius + 0.48,
                                      glyphDistance);
    if (glyphMask <= 0.01) {
        color = vec4(limitFlashBrightness(coverUnderlay) * visibility, 1.0);
        return;
    }

    vec2 dotOrigin = cell * cellSize + vec2(float(dx) * 6.0, float(dy) * 6.0);
    vec3 brightest = vec3(0.0);
    vec3 coverColorSum = vec3(0.0);
    float coverLightSum = 0.0;
    float best = 0.0;
    for (int oy = 0; oy < 6; ++oy) {
        for (int ox = 0; ox < 6; ++ox) {
            vec2 sampleUv = (dotOrigin + vec2(float(ox) + 0.5, float(oy) + 0.5)) / resolution;
            // Keep the viewpoint stable. Bass affects glyph weight and light,
            // never the camera, so the eye can continue following the subject.
            vec3 candidate = sceneSample(sampleUv);
            float level = luminance(candidate);
            if (level > best) { best = level; brightest = candidate; }
            if (coverMix > 0.0) {
                coverColorSum += candidate;
                coverLightSum += level;
            }
        }
    }

    vec3 sourceColor = brightest;
    float coverLevel = best;
    if (coverMix > 0.0) {
        // A cell average retains the artwork's real RGB balance and local
        // shading. Brightest-sample pooling turned photographs into flat white
        // masks whenever a small neutral highlight crossed a cell.
        vec3 coverAverage = coverColorSum / 36.0;
        float coverAverageLight = coverLightSum / 36.0;
        float targetLight = pow(clamp(coverAverageLight, 0.0, 1.0), 0.82);
        float coverMaximum = max(coverAverage.r,
                                 max(coverAverage.g, coverAverage.b));
        float lift = min(targetLight / max(0.01, coverAverageLight),
                         0.98 / max(0.01, coverMaximum));
        sourceColor = mix(brightest, coverAverage * lift, coverColorLock);
        // Keep one dot in reserve even for pure white. The ordered braille
        // texture remains visible instead of becoming a solid white field.
        coverLevel = min(0.92, targetLight * 0.96);
    }

    const float threshold[8] = float[8](0.08, 0.58, 0.33, 0.83, 0.70, 0.20, 0.95, 0.45);
    float materialExposure = mix(asciiExposure, 1.0, coverMix);
    float presetLevel = min(1.0, sqrt(max(best, 0.0)) * 1.25
                                 * sqrt(materialExposure));
    presetLevel = floor(presetLevel * 5.0 + 0.5) / 5.0;
    float level = mix(presetLevel, coverLevel, coverColorLock);
    // Bass briefly reveals more of the preset's own dim structure. This makes
    // the active subject feel heavier without adding a ring or moving the
    // camera independently of the composition.
    // Treble exposes the smallest source details while mids enrich the color
    // already present in the preset. Each band changes a different material
    // property, so the result reads as music rather than one global pulse.
    float activeThreshold = threshold[dy * 2 + dx] - visualReaction
                          * (0.055 * bassImpact + 0.060 * kickAccent
                             + 0.025 * trebleLevel + 0.045 * trebleImpact);
    if (level < activeThreshold) {
        color = vec4(limitFlashBrightness(coverUnderlay) * visibility, 1.0);
        return;
    }
    vec3 energized = mix(sourceColor, sourceColor * sourceColor * 1.12,
                         0.16 * bassImpact * (1.0 - coverColorLock));
    float grey = luminance(energized);
    energized = mix(vec3(grey), energized,
                    1.0 + (0.08 * midLevel + 0.12 * midImpact)
                        * (1.0 - coverColorLock));
    // The source preset owns luminance. Audio changes glyph weight, detail,
    // and chroma, but must never globally flash the frame toward white.
    float visualLightResponse = 0.80 + level * 0.32 + 0.035 * bassLevel
                              + 0.025 * trebleLevel;
    float outputScale = mix(visualLightResponse, 1.0, coverColorLock);
    vec3 dotColor = energized * outputScale * materialExposure;
    float dotOpacity = mix(1.0, 0.78, coverColorLock);
    vec3 asciiMaterial = mix(coverUnderlay, dotColor,
                             glyphMask * dotOpacity);
    color = vec4(limitFlashBrightness(asciiMaterial) * visibility, 1.0);
}
)GLSL";

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: projectm-ascii-live PRESET.milk [PRESET.milk ...]\n";
        return 2;
    }
    const std::filesystem::path projectRoot
        = std::filesystem::canonical(argv[0]).parent_path() / ".." / "..";
    const std::string rendererName = std::getenv("OMADROP_ENGINE")
        ? std::getenv("OMADROP_ENGINE") : "native";
    if (rendererName != "native" && rendererName != "projectm") {
        std::cerr << "renderer: unknown OMADROP_ENGINE value '"
                  << rendererName << "'\n";
        return 2;
    }
    const bool nativeEnabled = rendererName == "native";
    const bool calibrationMode = std::getenv("OMADROP_CALIBRATION") != nullptr;
    LivePreferences preferences = loadLivePreferences();
    bool asciiEnabled = std::getenv("OMADROP_ASCII")
        ? std::string(std::getenv("OMADROP_ASCII")) != "0"
        : preferences.asciiEnabled;
    unsigned int syncDelayMs = 0;
    bool closeRequested = false;
    std::optional<NativeSceneKind> selectedNativeScene;
    std::vector<NativeSceneKind> scriptedScenes;
    float scriptedSceneDwellSeconds = 5.0f;
    float scriptedSceneMaximumSeconds = 7.0f;
    float scriptedTransitionSeconds = 2.0f;
    bool scriptedSequenceOnce = false;
    if (nativeEnabled) {
        if (const char* requestedScene = std::getenv("OMADROP_NATIVE_SCENE")) {
            NativeSceneKind selectedScene;
            if (!nativeSceneFromName(requestedScene, selectedScene)) {
                std::cerr << "native scene: unknown OMADROP_NATIVE_SCENE value '"
                          << requestedScene << "'\n";
                return 2;
            }
            selectedNativeScene = selectedScene;
        }
        if (const char* requestedSequence
            = std::getenv("OMADROP_NATIVE_SEQUENCE")) {
            std::istringstream input(requestedSequence);
            std::string name;
            while (std::getline(input, name, ',')) {
                name.erase(name.begin(), std::find_if(name.begin(), name.end(),
                    [](unsigned char character) { return !std::isspace(character); }));
                name.erase(std::find_if(name.rbegin(), name.rend(),
                    [](unsigned char character) { return !std::isspace(character); }).base(),
                    name.end());
                NativeSceneKind scene;
                if (name.empty() || !nativeSceneFromName(name, scene)) {
                    std::cerr << "native sequence: unknown scene '" << name << "'\n";
                    return 2;
                }
                if (scriptedScenes.empty() || scriptedScenes.back() != scene) {
                    scriptedScenes.push_back(scene);
                }
            }
            if (scriptedScenes.size() < 2) {
                std::cerr << "native sequence: provide at least two distinct scenes\n";
                return 2;
            }
            selectedNativeScene = scriptedScenes.front();
            if (const char* dwell = std::getenv("OMADROP_NATIVE_SEQUENCE_SECONDS")) {
                scriptedSceneDwellSeconds = std::clamp(
                    std::strtof(dwell, nullptr), 1.0f, 30.0f);
            }
            scriptedSceneMaximumSeconds = scriptedSceneDwellSeconds + 2.0f;
            if (const char* maximum
                = std::getenv("OMADROP_NATIVE_SEQUENCE_MAX_SECONDS")) {
                scriptedSceneMaximumSeconds = std::clamp(
                    std::strtof(maximum, nullptr),
                    scriptedSceneDwellSeconds, 32.0f);
            }
            if (const char* duration
                = std::getenv("OMADROP_NATIVE_TRANSITION_SECONDS")) {
                scriptedTransitionSeconds = std::clamp(
                    std::strtof(duration, nullptr), 0.6f, 8.0f);
            }
            scriptedSequenceOnce
                = std::getenv("OMADROP_NATIVE_SEQUENCE_ONCE")
               && std::string(std::getenv("OMADROP_NATIVE_SEQUENCE_ONCE")) != "0";
        }
    }
    std::optional<StructureTimeline> timeline;
    if (const char* timelinePath = std::getenv("OMADROP_TIMELINE_PATH")) {
        StructureTimeline loaded;
        std::string error;
        if (!loaded.load(timelinePath, error)) {
            std::cerr << "timeline: " << error << "\n";
            return 2;
        }
        std::cerr << "timeline: " << timelinePath << " ("
                  << loaded.sections().size() << " sections)\n";
        timeline = std::move(loaded);
    }
    signal(SIGTERM, requestStop);
    signal(SIGINT, requestStop);
    SDL_SetHint(SDL_HINT_SCREENSAVER_INHIBIT_ACTIVITY_NAME, "Visualizing music");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    if (const char* borderless = std::getenv("OMADROP_BORDERLESS");
        borderless && std::string(borderless) != "0") {
        windowFlags |= SDL_WINDOW_BORDERLESS;
    }
    if (const char* fullscreen = std::getenv("OMADROP_FULLSCREEN");
        fullscreen && std::string(fullscreen) != "0") {
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    int displayIndex = 0;
    if (const char* requestedDisplay = std::getenv("OMADROP_DISPLAY_INDEX")) {
        displayIndex = std::clamp(std::atoi(requestedDisplay), 0,
                                  std::max(0, SDL_GetNumVideoDisplays() - 1));
    }
    const int windowPosition = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
    SDL_Window* window = SDL_CreateWindow("Omadrop",
        windowPosition, windowPosition, width, height,
        windowFlags);
    if (!window) {
        std::cerr << "window creation failed: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "OpenGL context failed: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return 1;

    std::array<GLuint, 2> frameTextures{};
    glGenTextures(2, frameTextures.data());
    for (GLuint texture : frameTextures) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    int sourceWidth = width;
    int sourceHeight = height;
    GLuint coverTexture = 0;
    glGenTextures(1, &coverTexture);
    glBindTexture(GL_TEXTURE_2D, coverTexture);
    const std::array<unsigned char, 4> blackPixel{0, 0, 0, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackPixel.data());
    LiveCompositor compositor;
    std::string compositorError;
    if (!compositor.initialize(vertexSource, fragmentSource, compositorError)) {
        std::cerr << "display compositor: " << compositorError << "\n";
        return 1;
    }
    StatusOverlay statusOverlay;
    if (!statusOverlay.initialize(compositorError)) {
        std::cerr << "status overlay: " << compositorError << "\n";
        return 1;
    }
    std::unique_ptr<NativeRenderer> nativeRenderer;
    if (nativeEnabled) {
        nativeRenderer = std::make_unique<NativeRenderer>();
        std::string error;
        if (!nativeRenderer->initialize(projectRoot / "shaders" / "native", error)) {
            std::cerr << "native renderer: " << error << "\n";
            return 1;
        }
        std::cerr << "renderer: Omadrop native scenes\n";
    }

    const char* texturePaths[] = {"/usr/share/projectM/textures", "/usr/share/projectM/presets"};
    std::array<projectm_handle, 2> engines{projectm_create(), projectm_create()};
    if (!engines[0] || !engines[1]) return 1;
    for (projectm_handle engine : engines) {
        projectm_set_window_size(engine, width, height);
        projectm_set_mesh_size(engine, 48, 36);
        projectm_set_fps(engine, 60);
        projectm_set_preset_locked(engine, true);
        projectm_set_hard_cut_enabled(engine, false);
        projectm_set_texture_search_paths(engine, texturePaths, 2);
    }
    std::vector<std::string> presets(argv + 1, argv + argc);
    for (const auto& preset : presets) {
        if (!findProfileForPreset(preset)) {
            std::cerr << "unprofiled preset: " << presetBasename(preset) << "\n";
            return 2;
        }
    }
    const unsigned int randomSeed = std::getenv("OMADROP_RANDOM_SEED")
        ? static_cast<unsigned int>(std::strtoul(std::getenv("OMADROP_RANDOM_SEED"), nullptr, 10))
        : std::random_device{}();
    std::mt19937 randomEngine(randomSeed);
    const std::filesystem::path pairedStatePath = std::getenv("OMADROP_PAIR_STATE")
        ? std::getenv("OMADROP_PAIR_STATE") : "";
    const std::string pairedRole = std::getenv("OMADROP_PAIR_ROLE")
        ? std::getenv("OMADROP_PAIR_ROLE") : "";
    const bool pairedLeader = !pairedStatePath.empty() && pairedRole == "leader";
    const bool pairedFollower = !pairedStatePath.empty() && pairedRole == "follower";
    bool fullscreenEnabled
        = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0
       || pairedLeader || pairedFollower;
    const PairedTransport pairedTransport(pairedStatePath);
    std::uint64_t pairedSerial = 0;
    std::uint64_t pairedMusicSerial = 0;
    PairedDisplayFollower pairedDisplayFollower;
    PairedMusicFollower pairedMusicFollower;
    NativeSceneDirector nativeSceneDirector;
    nativeSceneDirector.setProfile(preferences.directorProfile);
    nativeSceneDirector.setScenePreferences(
        preferences.favoriteScenes, preferences.hiddenScenes);
    auto publishPairedState = [&](std::size_t index, std::uint64_t duration,
                                  int mode, bool hardSync, int nativeScene = -1,
                                  int nativeSourceScene = -1,
                                  bool manualSceneCue = false) {
        if (!pairedLeader) return;
        // Display state is a replaceable snapshot, not an event queue. Include
        // the current native transition in every control update so a key press
        // can never overwrite the only copy of a scene change before a
        // follower reads it.
        if (nativeEnabled && nativeScene < 0) {
            const NativeSceneState& scene = nativeSceneDirector.state();
            nativeSourceScene = static_cast<int>(scene.currentScene);
            nativeScene = static_cast<int>(scene.transitioning
                ? scene.incomingScene : scene.currentScene);
            mode = static_cast<int>(scene.transitioning
                ? scene.transitionStyle : NativeTransitionStyle::FlowCarry);
        }
        pairedTransport.publishDisplay({
            .serial = ++pairedSerial,
            .presetIndex = index,
            .durationMs = duration,
            .transitionMode = mode,
            .hardSync = hardSync,
            .nativeScene = nativeScene,
            .nativeSourceScene = nativeSourceScene,
            .asciiMode = asciiEnabled ? 1 : 0,
            .fullscreenMode = fullscreenEnabled ? 1 : 0,
            .syncDelayMs = static_cast<int>(syncDelayMs),
            .closeMode = closeRequested ? 1 : 0,
            .intensityPercent = static_cast<int>(
                std::lround(preferences.intensity * 100.0f)),
            .brightnessPercent = static_cast<int>(
                std::lround(preferences.brightness * 100.0f)),
            .motionPercent = static_cast<int>(
                std::lround(preferences.motion * 100.0f)),
            .reducedMotionMode = preferences.reducedMotion ? 1 : 0,
            .highContrastMode = preferences.highContrast ? 1 : 0,
            .directorProfile = static_cast<int>(preferences.directorProfile),
            .manualSceneCue = manualSceneCue ? 1 : 0,
            .flashLimitMode = preferences.flashLimited ? 1 : 0,
            .colorVisionSafeMode = preferences.colorVisionSafe ? 1 : 0,
        });
    };
    auto publishPairedMusic = [&](const MusicFrame& frame, float flowTime) {
        if (!pairedLeader) return;
        const PairedMusicState state{
            .serial = ++pairedMusicSerial,
            .flowTime = flowTime,
            .frame = frame,
        };
        pairedTransport.publishMusic(state);
    };
    std::uniform_int_distribution<std::size_t> openingPreset(0, presets.size() - 1);
    std::size_t presetIndex = std::getenv("OMADROP_START_PRESET")
        ? static_cast<std::size_t>(std::max(0, std::atoi(std::getenv("OMADROP_START_PRESET")))) % presets.size()
        : openingPreset(randomEngine);
    std::deque<std::size_t> recentPresets{presetIndex};
    auto chooseAutomaticPreset = [&](PresetEnergy targetEnergy, int preferredFamily = -1) {
        return choosePresetVariant(presets, presetIndex, recentPresets, targetEnergy,
                                   preferredFamily, randomEngine);
    };
    auto energyForTrack = [](float energy) {
        if (energy < 0.90f) return PresetEnergy::Calm;
        if (energy < 1.25f) return PresetEnergy::Medium;
        return PresetEnergy::Driving;
    };
    int activeEngine = 0;
    bool presetTransitionActive = false;
    uint64_t presetTransitionStartedAt = 0;
    uint64_t presetTransitionDuration = 6500;
    int transitionMode = 3;
    std::size_t transitionOutgoingPresetIndex = presetIndex;
    TimelineDirector timelineDirector;
    VisualMotifMemory visualMotifs;
    std::optional<std::size_t> pendingTimelinePreset;
    if (!loadPresetAtVisualTempo(engines[activeEngine], presets[presetIndex], false)) return 1;
    std::cerr << "preset: " << presets[presetIndex] << "\n";
    publishPairedState(presetIndex, 0, 0, true);
    if (pairedLeader || pairedFollower) {
        std::cerr << "paired display: " << pairedRole << "\n";
    }
    uint64_t transitionWindowAt = SDL_GetTicks64() + 9000;
    uint64_t transitionDeadlineAt = SDL_GetTicks64() + 13000;

    std::string sink = defaultSinkName();
    syncDelayMs = loadSyncDelay(sink);
    std::deque<float> delayedPcm;
    std::cerr << "audio sync delay: " << syncDelayMs << " ms\n";
    publishPairedState(presetIndex, 0, 0, true);
    PipeWireCapture audioCapture;
    if (!audioCapture.start(sink)) return 1;
    std::vector<float> pcm(4096 * 2);
    std::vector<float> visualPcm(4096 * 2);
    const unsigned int projectmSampleLimit = projectm_pcm_get_max_samples();
    std::vector<float> projectmPcm(projectmSampleLimit * 2);
    double visualBassPhase = 0.0;
    double visualMidPhase = 0.0;
    double visualTreblePhase = 0.0;
    double fallbackPhase = 0.0;
    const bool syntheticAudio = std::getenv("OMADROP_SYNTHETIC_AUDIO") != nullptr;
    bool lastFallback = false;
    bool reportedAudioMode = false;
    bool reportedPairedMusic = false;
    uint64_t discardedAudioFrames = 0;
    uint64_t lastNonSilentAudioAt = SDL_GetTicks64();
    uint64_t previousFrameAt = SDL_GetTicks64();
    AudioFeatureBus featureBus;
    AudioFeatures audioFeatures;
    MusicalStructureTracker structureTracker;
    MusicFrameBuilder musicFrameBuilder;
    MusicFrame musicFrame;
    NativeSceneState nativeSceneState;
    bool nativeTransitionWasActive = false;
    NativeSceneKind initialNativeScene = NativeSceneKind::DepthTunnel;
    if (nativeEnabled) {
        if (selectedNativeScene) {
            initialNativeScene = *selectedNativeScene;
        } else {
            std::vector<NativeSceneKind> openingCandidates;
            for (std::size_t index = 0; index < nativeSceneCount; ++index) {
                const NativeSceneKind scene
                    = static_cast<NativeSceneKind>(index);
                if (!nativeSceneDirector.sceneHidden(scene)) {
                    openingCandidates.push_back(scene);
                }
            }
            std::uniform_int_distribution<std::size_t> openingNativeScene(
                0, openingCandidates.size() - 1);
            initialNativeScene = openingCandidates[openingNativeScene(randomEngine)];
        }
        nativeSceneDirector.selectScene(initialNativeScene);
        if (!scriptedScenes.empty()) {
            nativeSceneDirector.setTransitionDuration(scriptedTransitionSeconds);
        }
        std::cerr << "native scene: " << nativeSceneName(initialNativeScene)
                  << (!scriptedScenes.empty() ? " (scripted opening)"
                      : selectedNativeScene ? " (selected)"
                      : " (random opening)")
                  << "\n";
        publishPairedState(presetIndex, 0, 6, true,
                           static_cast<int>(initialNativeScene),
                           static_cast<int>(initialNativeScene));
    }
    std::size_t scriptedSceneIndex = 0;
    uint64_t scriptedSceneSettledAt = 0;
    float scriptedPreviousBarPhase = 0.0f;
    bool structureClockLocked = false;
    float bassImpact = 0.0f;
    float midImpact = 0.0f;
    float trebleImpact = 0.0f;
    float musicalEnergy = 1.0f;
    const bool debugAudio = std::getenv("OMADROP_DEBUG_AUDIO") != nullptr;
    const float reactionScale = std::getenv("OMADROP_REACTION_SCALE")
        ? std::clamp(std::strtof(std::getenv("OMADROP_REACTION_SCALE"), nullptr), 0.0f, 3.0f)
        : 1.0f;
    uint64_t lastClockLogAt = 0;
    float coverAspect = 1.0f;
    std::array<float, 3> albumColor{0.72f, 0.82f, 1.0f};
    std::string currentArtPath;
    const float coverHoldSeconds = std::getenv("OMADROP_COVER_HOLD_SECONDS")
        ? std::clamp(std::strtof(
            std::getenv("OMADROP_COVER_HOLD_SECONDS"), nullptr), 0.0f, 20.0f)
        : 5.0f;
    const float coverDissolveSeconds = std::getenv("OMADROP_COVER_DISSOLVE_SECONDS")
        ? std::clamp(std::strtof(
            std::getenv("OMADROP_COVER_DISSOLVE_SECONDS"), nullptr), 0.5f, 20.0f)
        : 5.0f;
    CoverPresentation coverPresentation(
        coverHoldSeconds, coverDissolveSeconds);
    uint64_t nextMprisPollAt = 0;
    const std::filesystem::path mprisHelper = projectRoot / "bin" / "mpris-state";
    MprisPoller mprisPoller(mprisHelper);
    PlaybackClock playbackClock;
    bool timelineMismatchReported = false;
    uint64_t timelineClockStartedAt = SDL_GetTicks64();
    double forcedTimelinePosition = -1.0;
    if (const char* position = std::getenv("OMADROP_TIMELINE_POSITION")) {
        char* end = nullptr;
        const double parsed = std::strtod(position, &end);
        if (end != position && *end == '\0' && std::isfinite(parsed) && parsed >= 0.0) {
            forcedTimelinePosition = parsed;
        } else {
            std::cerr << "timeline: OMADROP_TIMELINE_POSITION must be a non-negative number\n";
            return 2;
        }
    }

    bool running = true;
    bool windowShown = false;
    const std::filesystem::path startGatePath = std::getenv("OMADROP_START_GATE")
        ? std::getenv("OMADROP_START_GATE") : "";
    const std::filesystem::path startReadyPath = startGatePath.empty()
        ? std::filesystem::path{}
        : std::filesystem::path(startGatePath.string() + "."
            + std::to_string(displayIndex) + ".ready");
    const std::filesystem::path readyPath = std::getenv("OMADROP_READY_FILE")
        ? std::getenv("OMADROP_READY_FILE") : "";
    const std::filesystem::path recordStopPath
        = std::getenv("OMADROP_RECORD_STOP_FILE")
        ? std::getenv("OMADROP_RECORD_STOP_FILE") : "";
    const uint64_t closeDurationMs = recordStopPath.empty() ? 420u : 920u;
    bool recordStopSignaled = false;
    const auto signalRecordingMarker = [](const std::filesystem::path& path) {
        if (path.empty()) return;
        std::ofstream marker(path, std::ios::trunc);
        if (marker) marker << getpid() << '\n';
    };
    bool startGateOpened = startGatePath.empty();
    uint64_t revealStartedAt = 0;
    bool closing = false;
    uint64_t closeStartedAt = 0;
    uint64_t calibrationStatusAt = 0;
    const std::string forcedCoverPath = std::getenv("OMADROP_COVER_PATH")
        ? std::getenv("OMADROP_COVER_PATH") : "";
    const bool disableArt = std::getenv("OMADROP_DISABLE_ART") != nullptr
                         || !forcedCoverPath.empty();
    bool artLookupComplete = disableArt;
    const uint64_t initialArtDeadlineAt = SDL_GetTicks64() + 4500;
    bool suppressLateInitialCover = false;
    if (!forcedCoverPath.empty()
        && loadPngTexture(forcedCoverPath, coverTexture, coverAspect)) {
        currentArtPath = forcedCoverPath;
        albumColor = loadPaletteColor(forcedCoverPath);
        const uint64_t coverLoadedAt = SDL_GetTicks64();
        coverPresentation.show(coverLoadedAt);
        transitionWindowAt = coverLoadedAt + 19000;
        transitionDeadlineAt = coverLoadedAt + 23000;
        std::cerr << "cover: " << forcedCoverPath << " (forced)\n";
    }
    const uint64_t automaticQuitAt = std::getenv("OMADROP_AUTO_QUIT_MS")
        ? SDL_GetTicks64() + static_cast<uint64_t>(std::max(0, std::atoi(std::getenv("OMADROP_AUTO_QUIT_MS"))))
        : 0;
    uint64_t automaticNextAt = std::getenv("OMADROP_AUTO_NEXT_MS")
        ? SDL_GetTicks64() + static_cast<uint64_t>(std::max(0, std::atoi(std::getenv("OMADROP_AUTO_NEXT_MS"))))
        : 0;
    auto cycleLevel = [](float current, const std::array<float, 3>& levels) {
        for (const float level : levels) {
            if (level > current + 0.01f) return level;
        }
        return levels.front();
    };
    auto applyVisualPreference = [&](const std::string& request,
                                     std::uint64_t statusAt) {
        std::string status;
        if (request == "intensity") {
            preferences.intensity = cycleLevel(
                preferences.intensity, {0.75f, 1.0f, 1.25f});
            std::cerr << "visual intensity: "
                      << std::lround(preferences.intensity * 100.0f) << "%\n";
            status = "INTENSITY: "
                   + std::to_string(std::lround(
                       preferences.intensity * 100.0f)) + "%";
        } else if (request == "brightness") {
            preferences.brightness = cycleLevel(
                preferences.brightness, {0.70f, 1.0f, 1.15f});
            std::cerr << "visual brightness: "
                      << std::lround(preferences.brightness * 100.0f) << "%\n";
            status = "BRIGHTNESS: "
                   + std::to_string(std::lround(
                       preferences.brightness * 100.0f)) + "%";
        } else if (request == "motion") {
            preferences.motion = cycleLevel(
                preferences.motion, {0.35f, 0.65f, 1.0f});
            std::cerr << "ambient motion: "
                      << std::lround(preferences.motion * 100.0f) << "%\n";
            status = "AMBIENT MOTION: "
                   + std::to_string(std::lround(
                       preferences.motion * 100.0f)) + "%";
        } else if (request == "reduced-motion") {
            preferences.reducedMotion = !preferences.reducedMotion;
            std::cerr << "reduced motion: "
                      << (preferences.reducedMotion ? "on" : "off") << "\n";
            status = std::string("REDUCED MOTION: ")
                   + (preferences.reducedMotion ? "ON" : "OFF");
        } else if (request == "flash-limit") {
            preferences.flashLimited = !preferences.flashLimited;
            std::cerr << "flash limit: "
                      << (preferences.flashLimited ? "on" : "off") << "\n";
            status = std::string("FLASH LIMIT: ")
                   + (preferences.flashLimited ? "ON" : "OFF");
        } else if (request == "high-contrast") {
            preferences.highContrast = !preferences.highContrast;
            std::cerr << "high contrast: "
                      << (preferences.highContrast ? "on" : "off") << "\n";
            status = std::string("HIGH CONTRAST: ")
                   + (preferences.highContrast ? "ON" : "OFF");
        } else if (request == "color-vision-safe") {
            preferences.colorVisionSafe = !preferences.colorVisionSafe;
            std::cerr << "color-safe palette: "
                      << (preferences.colorVisionSafe ? "on" : "off") << "\n";
            status = std::string("COLOR SAFE: ")
                   + (preferences.colorVisionSafe ? "ON" : "OFF");
        } else if (request == "director") {
            const int next = (static_cast<int>(preferences.directorProfile) + 1)
                           % 4;
            preferences.directorProfile = static_cast<DirectorProfile>(next);
            nativeSceneDirector.setProfile(preferences.directorProfile);
            std::cerr << "director profile: "
                      << directorProfileName(preferences.directorProfile) << "\n";
            status = std::string("DIRECTOR: ")
                   + directorProfileName(preferences.directorProfile);
        } else if (request == "favorite" && nativeEnabled) {
            const NativeSceneKind scene = nativeSceneDirector.state().transitioning
                ? nativeSceneDirector.state().incomingScene
                : nativeSceneDirector.state().currentScene;
            const std::string slug(nativeSceneDefinition(scene).slug);
            auto found = std::find(preferences.favoriteScenes.begin(),
                                   preferences.favoriteScenes.end(), slug);
            if (found == preferences.favoriteScenes.end()) {
                preferences.favoriteScenes.push_back(slug);
                status = std::string("FAVORITE: ") + nativeSceneName(scene);
            } else {
                preferences.favoriteScenes.erase(found);
                status = std::string("FAVORITE OFF: ") + nativeSceneName(scene);
            }
            nativeSceneDirector.setScenePreferences(
                preferences.favoriteScenes, preferences.hiddenScenes);
        } else if (request == "hide" && nativeEnabled) {
            const NativeSceneKind scene = nativeSceneDirector.state().transitioning
                ? nativeSceneDirector.state().incomingScene
                : nativeSceneDirector.state().currentScene;
            if (nativeSceneDirector.visibleSceneCount() <= 2) {
                status = "KEEP AT LEAST TWO SCENES";
            } else {
                const std::string slug(nativeSceneDefinition(scene).slug);
                if (std::find(preferences.hiddenScenes.begin(),
                              preferences.hiddenScenes.end(), slug)
                    == preferences.hiddenScenes.end()) {
                    preferences.hiddenScenes.push_back(slug);
                }
                preferences.favoriteScenes.erase(std::remove(
                    preferences.favoriteScenes.begin(),
                    preferences.favoriteScenes.end(), slug),
                    preferences.favoriteScenes.end());
                nativeSceneDirector.setScenePreferences(
                    preferences.favoriteScenes, preferences.hiddenScenes);
                nativeSceneDirector.requestNext();
                status = std::string("HIDDEN: ") + nativeSceneName(scene);
            }
        } else if (request == "clear-hidden" && nativeEnabled) {
            preferences.hiddenScenes.clear();
            nativeSceneDirector.setScenePreferences(
                preferences.favoriteScenes, preferences.hiddenScenes);
            status = "HIDDEN SCENES: CLEARED";
        } else {
            return false;
        }
        if (!saveLivePreferences(preferences)) {
            std::cerr << "preferences: could not save\n";
        }
        statusOverlay.show(status, statusAt);
        return true;
    };
    uint64_t nextSinkPollAt = SDL_GetTicks64() + 2000;
    using FrameClock = std::chrono::steady_clock;
    constexpr auto frameInterval = std::chrono::nanoseconds(1000000000 / 60);
    auto nextFrame = FrameClock::now() + frameInterval;
    SessionResumeDetector resumeDetector;
    resumeDetector.observe(bootTimeMilliseconds(), SDL_GetTicks64());
    while (running) {
        const uint64_t now = SDL_GetTicks64();
        if (resumeDetector.observe(bootTimeMilliseconds(), now)) {
            // Do not replay buffered pre-suspend audio or leave a transient
            // envelope frozen on screen. Keep the scene and feedback image,
            // then rebuild capture and analysis from the resumed output.
            delayedPcm.clear();
            featureBus.resetAnalysis();
            audioFeatures = AudioFeatures{};
            structureTracker.reset();
            musicFrameBuilder.reset();
            musicFrame = MusicFrame{};
            bassImpact = 0.0f;
            midImpact = 0.0f;
            trebleImpact = 0.0f;
            musicalEnergy = 0.0f;
            structureClockLocked = false;
            lastNonSilentAudioAt = now;
            previousFrameAt = now;
            reportedAudioMode = false;
            mprisPoller.stop();
            nextMprisPollAt = now;
            audioCapture.stop();
            const std::string resumedDefaultSink = defaultSinkName();
            const std::string resumedSink = resumedDefaultSink.empty()
                ? sink : resumedDefaultSink;
            const bool captureResumed = !resumedSink.empty()
                                     && audioCapture.start(resumedSink);
            if (captureResumed) {
                sink = resumedSink;
                syncDelayMs = loadSyncDelay(sink);
                nextSinkPollAt = now + 2000;
                std::cerr << "session: resumed capture on " << sink << "\n";
                statusOverlay.show("SESSION RESUMED", now);
            } else {
                nextSinkPollAt = now;
                std::cerr << "session: capture unavailable after resume; retrying\n";
                statusOverlay.show("AUDIO OUTPUT: RETRYING", now);
            }
            nextFrame = FrameClock::now() + frameInterval;
        }
        if (stopRequested) closeRequested = true;
        bool skipPreset = false;
        bool previousPreset = false;
        bool bassHitThisFrame = false;
        bool phraseBoundaryThisFrame = false;
        bool sectionBoundaryThisFrame = false;
        float structureNovelty = 0.0f;
        int structureMotifIdentity = -1;
        bool structureMotifRecalled = false;
        bool pairedControlsChanged = false;
        std::string pairedControlRequest;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const bool calibrationDelayKey = calibrationMode
                && event.type == SDL_KEYDOWN
                && (event.key.keysym.sym == SDLK_LEFTBRACKET
                    || event.key.keysym.sym == SDLK_RIGHTBRACKET);
            if (event.type == SDL_KEYDOWN && event.key.repeat != 0
                && !calibrationDelayKey) continue;
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                if (pairedFollower) pairedControlRequest = "quit";
                else {
                    closeRequested = true;
                    pairedControlsChanged = true;
                }
            }
            if (calibrationMode && event.type == SDL_KEYDOWN
                && !calibrationDelayKey
                && event.key.keysym.sym != SDLK_ESCAPE) continue;
            if (!calibrationMode && event.type == SDL_KEYDOWN
                && event.key.keysym.sym == SDLK_n) skipPreset = true;
            if (!calibrationMode && event.type == SDL_KEYDOWN
                && event.key.keysym.sym == SDLK_p) previousPreset = true;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_a) {
                if (pairedFollower) pairedControlRequest = "ascii";
                else {
                    asciiEnabled = !asciiEnabled;
                    preferences.asciiEnabled = asciiEnabled;
                    saveLivePreferences(preferences);
                    pairedControlsChanged = true;
                    std::cerr << "display: "
                              << (asciiEnabled ? "Omadrop ASCII" : "continuous")
                              << "\n";
                    statusOverlay.show(
                        asciiEnabled ? "ASCII: ON" : "ASCII: OFF", now);
                }
            }
            const char* visualPreferenceRequest = nullptr;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_i) {
                visualPreferenceRequest = "intensity";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_b) {
                visualPreferenceRequest = "brightness";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_m) {
                visualPreferenceRequest = "motion";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_r) {
                visualPreferenceRequest = "reduced-motion";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_h) {
                visualPreferenceRequest = "high-contrast";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_c) {
                visualPreferenceRequest = "color-vision-safe";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_s) {
                visualPreferenceRequest = "flash-limit";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_d) {
                visualPreferenceRequest = "director";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_f) {
                visualPreferenceRequest = "favorite";
            } else if (event.type == SDL_KEYDOWN
                       && event.key.keysym.sym == SDLK_x) {
                visualPreferenceRequest
                    = (event.key.keysym.mod & KMOD_SHIFT)
                    ? "clear-hidden" : "hide";
            }
            if (visualPreferenceRequest) {
                if (pairedFollower) pairedControlRequest = visualPreferenceRequest;
                else {
                    applyVisualPreference(visualPreferenceRequest, now);
                    pairedControlsChanged = true;
                }
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11) {
                if (pairedFollower) pairedControlRequest = "fullscreen";
                else {
                    fullscreenEnabled = !fullscreenEnabled;
                    SDL_SetWindowFullscreen(window, fullscreenEnabled
                        ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    pairedControlsChanged = true;
                    statusOverlay.show(fullscreenEnabled
                        ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", now);
                }
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFTBRACKET) {
                if (pairedFollower) pairedControlRequest = "delay-down";
                else {
                    syncDelayMs = syncDelayMs >= 10 ? syncDelayMs - 10 : 0;
                    saveSyncDelay(syncDelayMs, sink);
                    pairedControlsChanged = true;
                    std::cerr << "audio sync delay: " << syncDelayMs << " ms\n";
                    statusOverlay.show(
                        "SYNC: " + std::to_string(syncDelayMs) + " MS", now);
                    calibrationStatusAt = 0;
                }
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RIGHTBRACKET) {
                if (pairedFollower) pairedControlRequest = "delay-up";
                else {
                    syncDelayMs = std::min(500u, syncDelayMs + 10);
                    saveSyncDelay(syncDelayMs, sink);
                    pairedControlsChanged = true;
                    std::cerr << "audio sync delay: " << syncDelayMs << " ms\n";
                    statusOverlay.show(
                        "SYNC: " + std::to_string(syncDelayMs) + " MS", now);
                    calibrationStatusAt = 0;
                }
            }
        }
        if (pairedFollower
            && (skipPreset || previousPreset || !pairedControlRequest.empty())) {
            const std::string request = previousPreset ? "previous"
                : skipPreset ? "next" : pairedControlRequest;
            pairedTransport.publishRequest(request);
            skipPreset = false;
            previousPreset = false;
        }
        if (pairedLeader) {
            if (const auto pairedRequest = pairedTransport.consumeRequest()) {
                const std::string& request = *pairedRequest;
                skipPreset = request == "next";
                previousPreset = request == "previous";
                if (request == "ascii") {
                    asciiEnabled = !asciiEnabled;
                    preferences.asciiEnabled = asciiEnabled;
                    saveLivePreferences(preferences);
                    pairedControlsChanged = true;
                    std::cerr << "display: "
                              << (asciiEnabled ? "Omadrop ASCII" : "continuous")
                              << "\n";
                    statusOverlay.show(
                        asciiEnabled ? "ASCII: ON" : "ASCII: OFF", now);
                } else if (request == "fullscreen") {
                    fullscreenEnabled = !fullscreenEnabled;
                    SDL_SetWindowFullscreen(window, fullscreenEnabled
                        ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    pairedControlsChanged = true;
                    statusOverlay.show(fullscreenEnabled
                        ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", now);
                } else if (request == "delay-down") {
                    syncDelayMs = syncDelayMs >= 10 ? syncDelayMs - 10 : 0;
                    saveSyncDelay(syncDelayMs, sink);
                    pairedControlsChanged = true;
                    std::cerr << "audio sync delay: " << syncDelayMs << " ms\n";
                    statusOverlay.show(
                        "SYNC: " + std::to_string(syncDelayMs) + " MS", now);
                } else if (request == "delay-up") {
                    syncDelayMs = std::min(500u, syncDelayMs + 10);
                    saveSyncDelay(syncDelayMs, sink);
                    pairedControlsChanged = true;
                    std::cerr << "audio sync delay: " << syncDelayMs << " ms\n";
                    statusOverlay.show(
                        "SYNC: " + std::to_string(syncDelayMs) + " MS", now);
                } else if (request == "quit") {
                    closeRequested = true;
                    pairedControlsChanged = true;
                } else if (applyVisualPreference(request, now)) {
                    pairedControlsChanged = true;
                }
            }
        }
        if (pairedLeader && pairedControlsChanged) {
            publishPairedState(presetIndex, 0, nativeEnabled ? 6 : transitionMode,
                               false);
        }
        if (closeRequested && !closing) {
            closing = true;
            closeStartedAt = now;
        }
        if (closing && now - closeStartedAt >= closeDurationMs) {
            running = false;
            continue;
        }
        if (!startGateOpened && std::filesystem::exists(startGatePath)) {
            startGateOpened = true;
            revealStartedAt = now;
            coverPresentation.restart(now);
        }
        if (!artLookupComplete && now >= initialArtDeadlineAt) {
            artLookupComplete = true;
            suppressLateInitialCover = true;
            std::cerr << "cover: startup artwork unavailable; continuing cleanly"
                      << " without a late cover\n";
        }
        if (now >= nextSinkPollAt) {
            nextSinkPollAt = now + 2000;
            const std::string currentSink = defaultSinkName();
            const auto resetAudioState = [&](const std::string& newSink) {
                syncDelayMs = loadSyncDelay(newSink);
                delayedPcm.clear();
                featureBus.resetAnalysis();
                musicFrameBuilder.reset();
                lastNonSilentAudioAt = now;
                reportedAudioMode = false;
            };
            AudioOutputFollowResult followResult;
            bool restartedCapture = false;
            if ((currentSink.empty() || currentSink == sink)
                && !audioCapture.running()) {
                restartedCapture = audioCapture.start(sink);
                followResult = restartedCapture
                    ? AudioOutputFollowResult::Followed
                    : AudioOutputFollowResult::Failed;
                if (restartedCapture) resetAudioState(sink);
            } else {
                followResult = followAudioOutput(
                    currentSink, sink,
                    [&] { audioCapture.stop(); }, resetAudioState,
                    [&](const std::string& newSink) {
                        return audioCapture.start(newSink);
                    });
            }
            if (followResult == AudioOutputFollowResult::Failed) {
                std::cerr << "audio: capture unavailable for " << currentSink
                          << "; retrying\n";
                statusOverlay.show("AUDIO OUTPUT: RETRYING", now);
            } else if (followResult == AudioOutputFollowResult::Followed) {
                std::cerr << "audio: "
                          << (restartedCapture ? "capture resumed on "
                                               : "followed default sink ")
                          << sink
                          << ", sync delay " << syncDelayMs << " ms\n";
                statusOverlay.show(restartedCapture
                    ? "AUDIO CAPTURE: RESUMED" : "AUDIO OUTPUT: FOLLOWED", now);
            }
        }
        if (automaticQuitAt > 0 && now >= automaticQuitAt) closeRequested = true;
        if (automaticNextAt > 0 && now >= automaticNextAt) {
            skipPreset = true;
            automaticNextAt = 0;
        }
        bool seekedThisFrame = false;
        if (now >= nextMprisPollAt && !mprisPoller.running()) {
            mprisPoller.start(disableArt, now);
        }
        if (const auto poll = mprisPoller.update()) {
            nextMprisPollAt = now + (!artLookupComplete ? 250
                                         : timeline ? 500 : 1000);
            if (!poll->error.empty()) {
                std::cerr << "mpris: " << poll->error << "\n";
                if (!artLookupComplete) {
                    artLookupComplete = true;
                    suppressLateInitialCover = true;
                }
            } else if (poll->state) {
                const MprisState& state = *poll->state;
                const PlaybackObservation observation
                    = playbackClock.observe(state, now / 1000.0);
                // A player that appears after Omadrop has already started is
                // a new presentation, not a late result from the launch-time
                // artwork lookup. Let that first track use its cover normally.
                if (observation.first && suppressLateInitialCover
                    && poll->startedAtMs >= initialArtDeadlineAt) {
                    suppressLateInitialCover = false;
                }
                if (observation.trackChanged) suppressLateInitialCover = false;
                seekedThisFrame = observation.seeked;
                if (observation.first || observation.trackChanged) {
                    timelineDirector.reset();
                    featureBus.resetAnalysis();
                    structureTracker.reset();
                    musicFrameBuilder.reset();
                    nativeSceneDirector.resetForTrack();
                    visualMotifs.reset();
                    structureClockLocked = false;
                    pendingTimelinePreset.reset();
                    timelineClockStartedAt = now;
                    timelineMismatchReported = false;
                }
                if (observation.trackChanged) {
                    coverPresentation.clear();
                    currentArtPath.clear();
                }
                if (observation.trackChanged && !pairedFollower) {
                    presetIndex = chooseAutomaticPreset(PresetEnergy::Medium);
                    recentPresets.clear();
                    recentPresets.push_back(presetIndex);
                    presetTransitionActive = false;
                    loadPresetAtVisualTempo(
                        engines[activeEngine], presets[presetIndex], false);
                    transitionWindowAt = now + 19000;
                    transitionDeadlineAt = now + 23000;
                    publishPairedState(
                        presetIndex, 0, nativeEnabled ? 6 : 0, true,
                        nativeEnabled
                            ? static_cast<int>(
                                nativeSceneDirector.state().currentScene)
                            : -1,
                        nativeEnabled
                            ? static_cast<int>(
                                nativeSceneDirector.state().currentScene)
                            : -1);
                    std::cerr << "track: " << state.identity << "\n";
                }
                if (!suppressLateInitialCover
                    && !state.artPath.empty()
                    && state.artPath != currentArtPath
                    && loadPngTexture(state.artPath, coverTexture, coverAspect)) {
                    currentArtPath = state.artPath;
                    albumColor = loadPaletteColor(state.artPath);
                    coverPresentation.show(now);
                    std::cerr << "cover: " << state.artPath << "\n";
                }
                if (!artLookupComplete && coverPresentation.hasArtwork()) {
                    artLookupComplete = true;
                }
            } else if (!artLookupComplete) {
                // No MPRIS player is active, so there is no artwork to wait
                // for. Begin with the native scene and never reverse into a
                // cover from this launch attempt.
                artLookupComplete = true;
                suppressLateInitialCover = true;
            }
        }
        unsigned int capturedFrames = 0;
        float peak = 0.0f;
        std::size_t sampleCount = 0;
        while ((sampleCount = audioCapture.read(pcm.data(), pcm.size())) > 0) {
            sampleCount -= sampleCount % 2;
            capturedFrames += static_cast<unsigned int>(sampleCount / 2);
            for (std::size_t i = 0; i < sampleCount; ++i) {
                peak = std::max(peak, std::abs(pcm[i]));
                if (!syntheticAudio) delayedPcm.push_back(pcm[i]);
            }
        }
        if (capturedFrames > 0 && peak >= 1e-5f) lastNonSilentAudioAt = now;
        // A nonblocking PipeWire fd normally has empty reads between packets.
        // Only call it silence after a sustained gap, or the analyzer receives
        // alternating real and synthetic audio and visibly loses the music.
        const bool fallback = syntheticAudio || now - lastNonSilentAudioAt >= 250;
        if (!reportedAudioMode || fallback != lastFallback) {
            std::cerr << "audio: "
                      << (fallback ? (syntheticAudio ? "synthetic test" : "silence")
                                   : "PipeWire") << "\n";
            lastFallback = fallback;
            reportedAudioMode = true;
        }
        constexpr unsigned int analysisHopFrames = 44100 / 60;
        constexpr std::size_t analysisHopSamples = analysisHopFrames * 2;
        constexpr std::size_t maximumAnalysisHops = 8;
        unsigned int audioHops = 0;
        if (fallback) {
            audioHops = 1;
            std::fill_n(pcm.begin(), analysisHopSamples, 0.0f);
            if (syntheticAudio) {
                const double seconds = SDL_GetTicks64() / 1000.0;
                constexpr double tau = 6.28318530717958647692;
                const double kickPulse = std::pow(std::max(0.0, std::cos(seconds * tau * 2.0)), 18.0);
                const double snarePulse = std::pow(std::max(0.0, std::cos((seconds - 0.25) * tau * 2.0)), 18.0);
                const double hatPulse = std::pow(std::max(0.0, std::cos((seconds - 0.125) * tau * 4.0)), 24.0);
                for (unsigned int i = 0; i < analysisHopFrames; ++i) {
                    const double t = fallbackPhase + i / 44100.0;
                    const double kick = (0.08 + 0.62 * kickPulse) * std::sin(t * tau * 62.0);
                    const double snare = 0.34 * snarePulse * std::sin(t * tau * 2100.0);
                    const double hat = 0.17 * hatPulse * std::sin(t * tau * 7000.0);
                    pcm[i * 2] = static_cast<float>(kick + snare + hat);
                    pcm[i * 2 + 1] = static_cast<float>(kick * 0.96 + snare * 1.04 + hat * 0.92);
                }
                fallbackPhase += analysisHopFrames / 44100.0;
            }
            delayedPcm.clear();
        } else {
            const std::size_t syncDelaySamples
                = static_cast<std::size_t>(44100 * syncDelayMs / 1000) * 2;
            // Analyze every complete 60 Hz hop that arrived since the last
            // video frame. Only discard audio after an exceptional stall, so
            // normal PipeWire packet bursts do not starve the beat clock.
            const auto prepared = prepareAudioHops(
                delayedPcm, syncDelaySamples, analysisHopSamples, maximumAnalysisHops);
            discardedAudioFrames += prepared.discardedSamples / 2;
            audioHops = static_cast<unsigned int>(prepared.readableSamples / analysisHopSamples);
        }
        for (unsigned int audioHop = 0; audioHop < audioHops; ++audioHop) {
            if (!fallback) {
                for (std::size_t i = 0; i < analysisHopSamples; ++i) {
                    pcm[i] = delayedPcm.front();
                    delayedPcm.pop_front();
                }
            }
            audioFeatures = featureBus.processStereo(pcm.data(), analysisHopFrames);
            const MusicalStructureState& structure = structureTracker.update(audioFeatures);
            musicFrame = musicFrameBuilder.update(
                audioFeatures, structure, 1.0f / 60.0f,
                syncDelayMs / 1000.0f);
            structureClockLocked = structure.clockLocked;
            phraseBoundaryThisFrame = phraseBoundaryThisFrame || structure.phraseCrossed;
            sectionBoundaryThisFrame = sectionBoundaryThisFrame || structure.sectionCrossed;
            if (structure.phraseCrossed || structure.sectionCrossed) {
                structureNovelty = structure.novelty;
                if (structure.sectionCrossed) {
                    structureMotifIdentity = structure.motifIdentity;
                    structureMotifRecalled = structure.motifRecalled;
                }
                if (debugAudio) {
                    std::cerr << "structure bar=" << structure.barIndex
                              << " novelty=" << structure.novelty
                              << " threshold=" << structure.noveltyThreshold
                              << (structure.sectionCrossed ? " section" : " phrase") << "\n";
                }
            }
            bassImpact = std::max(bassImpact, audioFeatures.kickImpact);
            midImpact = std::max(midImpact, audioFeatures.snareImpact);
            trebleImpact = std::max(trebleImpact, audioFeatures.hatImpact);
            bassHitThisFrame = bassHitThisFrame || (!fallback && audioFeatures.kick);
            if (debugAudio && audioFeatures.kick) {
                std::cerr << "kick hit " << audioFeatures.kickImpact << "\n";
            }
            if (debugAudio && audioFeatures.snare) {
                std::cerr << "snare hit " << audioFeatures.snareImpact << "\n";
            }
            if (debugAudio && audioFeatures.hat) {
                std::cerr << "hat hit " << audioFeatures.hatImpact << "\n";
            }
            const float energySample = std::clamp(
                0.34f * (audioFeatures.level[0] + audioFeatures.level[1])
                + 0.20f * audioFeatures.level[3]
                + 0.12f * audioFeatures.level[5], 0.0f, 3.0f);
            musicalEnergy += (energySample - musicalEnergy) * 0.006f;
            if (debugAudio && now - lastClockLogAt >= 2000) {
                std::cerr << "clock bpm=" << audioFeatures.bpm
                          << " confidence=" << audioFeatures.beatConfidence
                          << " beat=" << audioFeatures.beatPhase
                          << " bar=" << audioFeatures.barPhase
                          << " phrase=" << audioFeatures.phrasePhase
                          << " dropped=" << discardedAudioFrames << "\n";
                lastClockLogAt = now;
            }

            // projectM derives bass/mid/treb and waveform motion from the PCM
            // it receives. Give it a visual-only sidechain whose transients
            // have more dynamic range, so each preset's own equations react.
            // This never touches the audio sent to the speakers.
            const float kickAccentPosition = std::clamp(
                (bassImpact - 0.52f) / (0.82f - 0.52f), 0.0f, 1.0f);
            const float kickAccent = kickAccentPosition * kickAccentPosition
                                   * (3.0f - 2.0f * kickAccentPosition);
            const float visualDrive = 1.0f + 0.40f * bassImpact
                                             + 0.45f * kickAccent
                                             + 0.28f * midImpact
                                             + 0.14f * trebleImpact;
            constexpr double tau = 6.28318530717958647692;
            for (unsigned int i = 0; i < analysisHopFrames; ++i) {
                // Short tone bursts put unmistakable energy into the same
                // bands exposed to MilkDrop equations. They are control
                // signals only, not audible output.
                const float bassBurst = (0.28f * bassImpact + 0.28f * kickAccent)
                    * static_cast<float>(std::sin(visualBassPhase));
                const float midBurst = 0.20f * midImpact
                    * static_cast<float>(std::sin(visualMidPhase));
                const float trebleBurst = 0.10f * trebleImpact
                    * static_cast<float>(std::sin(visualTreblePhase));
                visualBassPhase += tau * 62.0 / 44100.0;
                visualMidPhase += tau * 520.0 / 44100.0;
                visualTreblePhase += tau * 4200.0 / 44100.0;
                if (visualBassPhase >= tau) visualBassPhase -= tau;
                if (visualMidPhase >= tau) visualMidPhase -= tau;
                if (visualTreblePhase >= tau) visualTreblePhase -= tau;
                const float control = bassBurst + midBurst + trebleBurst;
                visualPcm[i * 2] = pcm[i * 2] * visualDrive + control;
                visualPcm[i * 2 + 1] = pcm[i * 2 + 1] * visualDrive + control;
            }
            float visualPeak = 0.0f;
            for (std::size_t i = 0; i < analysisHopSamples; ++i) {
                visualPeak = std::max(visualPeak, std::abs(visualPcm[i]));
            }
            const float visualScale = visualPeak > 0.95f ? 0.95f / visualPeak : 1.0f;
            for (std::size_t i = 0; i < analysisHopSamples; ++i) {
                visualPcm[i] *= visualScale;
            }
            // projectM stores at most 480 samples per update. Resample the
            // complete 735-sample video interval instead of letting it discard
            // the final third of every frame.
            const unsigned int forwardedFrames = std::min(analysisHopFrames, projectmSampleLimit);
            const float sourceSpan = static_cast<float>(analysisHopFrames - 1);
            const float targetSpan = static_cast<float>(std::max(1u, forwardedFrames - 1));
            for (unsigned int i = 0; i < forwardedFrames; ++i) {
                const float sourcePosition = sourceSpan * i / targetSpan;
                const unsigned int left = static_cast<unsigned int>(sourcePosition);
                const unsigned int right = std::min(left + 1, analysisHopFrames - 1);
                const float fraction = sourcePosition - left;
                for (unsigned int channel = 0; channel < 2; ++channel) {
                    projectmPcm[i * 2 + channel]
                        = visualPcm[left * 2 + channel] * (1.0f - fraction)
                        + visualPcm[right * 2 + channel] * fraction;
                }
            }
            if (!nativeEnabled) {
                projectm_pcm_add_float(
                    engines[0], projectmPcm.data(), forwardedFrames, PROJECTM_STEREO);
                projectm_pcm_add_float(
                    engines[1], projectmPcm.data(), forwardedFrames, PROJECTM_STEREO);
            }
        }

        // Publish once per displayed frame. The synchronized flow clock keeps
        // autonomous motion from accumulating a different phase on monitors
        // whose compositor frame timing is slightly uneven.
        if (nativeEnabled && pairedLeader) {
            publishPairedMusic(musicFrame, nativeRenderer->flowTime());
        }
        if (nativeEnabled && pairedFollower) {
            if (const auto synchronized = pairedMusicFollower.consume(
                    pairedTransport.readMusic())) {
                musicFrame = synchronized->frame;
                nativeRenderer->synchronizeFlowTime(synchronized->flowTime);
                if (!reportedPairedMusic) {
                    std::cerr << "paired music: synchronized to leader\n";
                    reportedPairedMusic = true;
                }
            }
        }

        const bool timelineEligible = timeline
            && timeline->appliesTo(playbackClock.identity());
        if (timeline && !timelineEligible && playbackClock.hasState()
            && !timelineMismatchReported) {
            std::cerr << "timeline: track identity does not match "
                      << playbackClock.identity() << "; using live director\n";
            timelineMismatchReported = true;
        }
        if (timelineEligible && !pairedFollower) {
            const double timelinePosition = forcedTimelinePosition >= 0.0
                ? forcedTimelinePosition
                : playbackClock.hasState()
                    ? playbackClock.positionAt(now / 1000.0)
                    : (now - timelineClockStartedAt) / 1000.0;
            const auto decision = timelineDirector.sync(
                *timeline, timelinePosition, presetIndex, presets.size(), seekedThisFrame,
                [&](int preferredFamily) {
                    return chooseAutomaticPreset(energyForTrack(musicalEnergy), preferredFamily);
                },
                [&](std::size_t index) {
                    return visualFamily(profileForPreset(presets[index]));
                });
            if (decision) {
                const TimelineSection& section = timeline->sections()[decision->sectionIndex];
                if (decision->sectionChanged) {
                    std::cerr << "section: " << section.identity;
                    if (!section.label.empty()) std::cerr << " (" << section.label << ")";
                    std::cerr << " at " << timelinePosition << "s\n";
                }
                if (decision->hardSync
                    && (presetTransitionActive || decision->targetPreset != presetIndex)) {
                    presetIndex = decision->targetPreset;
                    presetTransitionActive = false;
                    pendingTimelinePreset.reset();
                    recentPresets.clear();
                    recentPresets.push_back(presetIndex);
                    loadPresetAtVisualTempo(engines[activeEngine], presets[presetIndex], false);
                    transitionWindowAt = UINT64_MAX;
                    transitionDeadlineAt = UINT64_MAX;
                    publishPairedState(presetIndex, 0, 0, true);
                    std::cerr << "preset: " << presets[presetIndex]
                              << " (timeline seek)\n";
                } else if (decision->sectionChanged && !decision->initial) {
                    pendingTimelinePreset = decision->targetPreset;
                }
            }
        }
        if (pendingTimelinePreset.value_or(presets.size()) == presetIndex) {
            pendingTimelinePreset.reset();
        }

        if (pairedFollower) {
            const auto pairedState = pairedDisplayFollower.consume(
                pairedTransport.readDisplay(), presets.size(), nativeSceneCount);
            if (pairedState) {
                if (pairedState->asciiMode >= 0) {
                    const bool synchronizedAscii = pairedState->asciiMode == 1;
                    if (asciiEnabled != synchronizedAscii) {
                        asciiEnabled = synchronizedAscii;
                        std::cerr << "display: "
                                  << (asciiEnabled ? "Omadrop ASCII" : "continuous")
                                  << " (paired)\n";
                        statusOverlay.show(
                            asciiEnabled ? "ASCII: ON" : "ASCII: OFF", now);
                    }
                }
                if (pairedState->fullscreenMode >= 0) {
                    const bool synchronizedFullscreen
                        = pairedState->fullscreenMode == 1;
                    if (fullscreenEnabled != synchronizedFullscreen) {
                        fullscreenEnabled = synchronizedFullscreen;
                        SDL_SetWindowFullscreen(window, fullscreenEnabled
                            ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                        statusOverlay.show(fullscreenEnabled
                            ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", now);
                    }
                }
                if (pairedState->syncDelayMs >= 0
                    && syncDelayMs
                       != static_cast<unsigned int>(pairedState->syncDelayMs)) {
                    syncDelayMs = static_cast<unsigned int>(pairedState->syncDelayMs);
                    std::cerr << "audio sync delay: " << syncDelayMs
                              << " ms (paired)\n";
                    statusOverlay.show(
                        "SYNC: " + std::to_string(syncDelayMs) + " MS", now);
                }
                if (pairedState->intensityPercent >= 0) {
                    const float synchronizedIntensity
                        = pairedState->intensityPercent / 100.0f;
                    const float synchronizedBrightness
                        = pairedState->brightnessPercent / 100.0f;
                    const float synchronizedMotion
                        = pairedState->motionPercent / 100.0f;
                    std::string synchronizedStatus;
                    if (std::abs(preferences.intensity
                                 - synchronizedIntensity) > 0.001f) {
                        synchronizedStatus = "INTENSITY: "
                            + std::to_string(pairedState->intensityPercent) + "%";
                    } else if (std::abs(preferences.brightness
                                        - synchronizedBrightness) > 0.001f) {
                        synchronizedStatus = "BRIGHTNESS: "
                            + std::to_string(pairedState->brightnessPercent) + "%";
                    } else if (std::abs(preferences.motion
                                        - synchronizedMotion) > 0.001f) {
                        synchronizedStatus = "AMBIENT MOTION: "
                            + std::to_string(pairedState->motionPercent) + "%";
                    } else if (preferences.reducedMotion
                               != (pairedState->reducedMotionMode == 1)) {
                        synchronizedStatus = pairedState->reducedMotionMode == 1
                            ? "REDUCED MOTION: ON" : "REDUCED MOTION: OFF";
                    } else if (preferences.highContrast
                               != (pairedState->highContrastMode == 1)) {
                        synchronizedStatus = pairedState->highContrastMode == 1
                            ? "HIGH CONTRAST: ON" : "HIGH CONTRAST: OFF";
                    } else if (pairedState->flashLimitMode >= 0
                               && preferences.flashLimited
                                  != (pairedState->flashLimitMode == 1)) {
                        synchronizedStatus = pairedState->flashLimitMode == 1
                            ? "FLASH LIMIT: ON" : "FLASH LIMIT: OFF";
                    } else if (pairedState->colorVisionSafeMode >= 0
                               && preferences.colorVisionSafe
                                  != (pairedState->colorVisionSafeMode == 1)) {
                        synchronizedStatus
                            = pairedState->colorVisionSafeMode == 1
                            ? "COLOR SAFE: ON" : "COLOR SAFE: OFF";
                    } else if (preferences.directorProfile
                               != static_cast<DirectorProfile>(
                                   pairedState->directorProfile)) {
                        synchronizedStatus = std::string("DIRECTOR: ")
                            + directorProfileName(static_cast<DirectorProfile>(
                                pairedState->directorProfile));
                    }
                    preferences.intensity
                        = synchronizedIntensity;
                    preferences.brightness
                        = synchronizedBrightness;
                    preferences.motion = synchronizedMotion;
                    preferences.reducedMotion
                        = pairedState->reducedMotionMode == 1;
                    preferences.highContrast
                        = pairedState->highContrastMode == 1;
                    if (pairedState->flashLimitMode >= 0) {
                        preferences.flashLimited
                            = pairedState->flashLimitMode == 1;
                    }
                    if (pairedState->colorVisionSafeMode >= 0) {
                        preferences.colorVisionSafe
                            = pairedState->colorVisionSafeMode == 1;
                    }
                    preferences.directorProfile = static_cast<DirectorProfile>(
                        pairedState->directorProfile);
                    nativeSceneDirector.setProfile(preferences.directorProfile);
                    if (!synchronizedStatus.empty()) {
                        statusOverlay.show(synchronizedStatus, now);
                    }
                }
                const LivePreferences storedPreferences = loadLivePreferences();
                if (preferences.favoriteScenes
                    != storedPreferences.favoriteScenes) {
                    statusOverlay.show("FAVORITES UPDATED", now);
                } else if (preferences.hiddenScenes
                           != storedPreferences.hiddenScenes) {
                    statusOverlay.show(storedPreferences.hiddenScenes.empty()
                        ? "HIDDEN SCENES: CLEARED"
                        : "HIDDEN SCENES UPDATED", now);
                }
                preferences.favoriteScenes = storedPreferences.favoriteScenes;
                preferences.hiddenScenes = storedPreferences.hiddenScenes;
                nativeSceneDirector.setScenePreferences(
                    preferences.favoriteScenes, preferences.hiddenScenes);
                if (pairedState->closeMode == 1) closeRequested = true;
            }
            if (nativeEnabled && pairedState && pairedState->nativeScene >= 0) {
                const NativeSceneKind target = static_cast<NativeSceneKind>(
                    pairedState->nativeScene);
                if (pairedState->manualSceneCue == 1) {
                    statusOverlay.show(
                        std::string("AUTO: ") + nativeSceneName(target), now);
                }
                if (pairedState->hardSync) nativeSceneDirector.selectScene(target);
                else {
                    if (pairedState->nativeSourceScene >= 0) {
                        const NativeSceneKind source = static_cast<NativeSceneKind>(
                            pairedState->nativeSourceScene);
                        const NativeSceneState& followerState
                            = nativeSceneDirector.state();
                        if (!followerState.transitioning
                            && followerState.currentScene != source) {
                            nativeSceneDirector.selectScene(source);
                        }
                    }
                    if (pairedState->transitionMode
                            >= static_cast<int>(NativeTransitionStyle::FlowCarry)
                        && pairedState->transitionMode
                            <= static_cast<int>(
                                NativeTransitionStyle::NegativeSpaceReveal)) {
                        nativeSceneDirector.requestTransitionStyle(
                            static_cast<NativeTransitionStyle>(
                                pairedState->transitionMode));
                    }
                    nativeSceneDirector.requestScene(target);
                }
            } else if (pairedState && pairedState->hardSync) {
                presetIndex = pairedState->presetIndex;
                presetTransitionActive = false;
                pendingTimelinePreset.reset();
                recentPresets.clear();
                recentPresets.push_back(presetIndex);
                loadPresetAtVisualTempo(engines[activeEngine], presets[presetIndex], false);
                transitionWindowAt = UINT64_MAX;
                transitionDeadlineAt = UINT64_MAX;
                std::cerr << "preset: " << presets[presetIndex]
                          << " (paired hard sync)\n";
            } else if (pairedState && pairedState->presetIndex != presetIndex
                       && !presetTransitionActive) {
                transitionOutgoingPresetIndex = presetIndex;
                presetIndex = pairedState->presetIndex;
                presetTransitionDuration = pairedState->durationMs;
                transitionMode = pairedState->transitionMode;
                recentPresets.push_back(presetIndex);
                const std::size_t historyLimit = presetHistoryLimit(presets.size());
                while (recentPresets.size() > historyLimit) recentPresets.pop_front();
                const int incomingEngine = 1 - activeEngine;
                if (!loadPresetAtVisualTempo(
                        engines[incomingEngine], presets[presetIndex], false)) {
                    std::cerr << "could not load paired preset: "
                              << presets[presetIndex] << "\n";
                }
                presetTransitionActive = true;
                presetTransitionStartedAt = now;
                transitionWindowAt = UINT64_MAX;
                transitionDeadlineAt = UINT64_MAX;
                std::cerr << "preset: " << presets[presetIndex]
                          << " (paired transition)\n";
            }
        }

        // Without an authored timeline, confident audio follows sustained
        // structural changes and phrase fallbacks. Uncertain audio keeps the
        // conservative bass-onset and wall-clock fallback.
        const bool fallbackOnset = !structureClockLocked && bassHitThisFrame;
        const bool confidentStructure = structureClockLocked;
        const bool musicalTransition = !timelineEligible && now >= transitionWindowAt
            && (confidentStructure
                    ? sectionBoundaryThisFrame
                      || (now >= transitionDeadlineAt && phraseBoundaryThisFrame)
                    : fallbackOnset);
        const bool deadlineTransition = !timelineEligible && !confidentStructure
                                     && now >= transitionDeadlineAt;
        const bool timelineTransition = pendingTimelinePreset.has_value();
        if (!nativeEnabled && !pairedFollower && !presetTransitionActive && presets.size() > 1
            && (skipPreset || previousPreset || timelineTransition
                || musicalTransition || deadlineTransition)) {
            const std::size_t outgoingPreset = presetIndex;
            transitionOutgoingPresetIndex = outgoingPreset;
            const PresetEnergy targetEnergy = energyForTrack(musicalEnergy);
            const bool nativeSectionTransition = musicalTransition
                                              && sectionBoundaryThisFrame
                                              && !skipPreset && !previousPreset;
            const int preferredFamily = nativeSectionTransition
                ? visualMotifs.familyFor(structureMotifIdentity).value_or(-1) : -1;
            presetIndex = previousPreset ? (presetIndex + presets.size() - 1) % presets.size()
                : skipPreset ? (presetIndex + 1) % presets.size()
                : timelineTransition ? *pendingTimelinePreset
                : chooseAutomaticPreset(targetEnergy, preferredFamily);
            if (nativeSectionTransition && structureMotifIdentity >= 0) {
                visualMotifs.remember(
                    structureMotifIdentity,
                    visualFamily(profileForPreset(presets[presetIndex])));
            }
            pendingTimelinePreset.reset();
            const PresetProfile& outgoing = profileForPreset(presets[outgoingPreset]);
            const PresetProfile& incoming = profileForPreset(presets[presetIndex]);
            auto musicalDuration = [&](float bars, uint64_t fallbackMs) {
                if (audioFeatures.beatConfidence < 0.35f) return fallbackMs;
                const float milliseconds = bars * 4.0f * 60000.0f
                                         / std::max(60.0f, audioFeatures.bpm);
                return static_cast<uint64_t>(std::clamp(milliseconds, 2500.0f, 7000.0f));
            };
            if (outgoing.family == incoming.family) {
                presetTransitionDuration = musicalDuration(1.0f, 4000);
                transitionMode = 0;
            }
            else if (directionsCompatible(outgoing.direction, incoming.direction)) {
                presetTransitionDuration = musicalDuration(1.5f, 5000);
                transitionMode = 1;
            } else if (outgoing.bridge == incoming.bridge) {
                presetTransitionDuration = musicalDuration(1.5f, 5500);
                transitionMode = 2;
            } else {
                presetTransitionDuration = musicalDuration(2.0f, 6000);
                transitionMode = 3;
            }
            recentPresets.push_back(presetIndex);
            const std::size_t historyLimit = presetHistoryLimit(presets.size());
            while (recentPresets.size() > historyLimit) recentPresets.pop_front();
            const int incomingEngine = 1 - activeEngine;
            if (!loadPresetAtVisualTempo(engines[incomingEngine], presets[presetIndex], false)) {
                std::cerr << "could not load preset: " << presets[presetIndex] << "\n";
            }
            presetTransitionActive = true;
            presetTransitionStartedAt = now;
            transitionWindowAt = UINT64_MAX;
            transitionDeadlineAt = UINT64_MAX;
            publishPairedState(
                presetIndex, presetTransitionDuration, transitionMode, false);
            const char* energyName = targetEnergy == PresetEnergy::Driving ? "driving"
                : targetEnergy == PresetEnergy::Medium ? "medium" : "calm";
            std::cerr << "preset: " << presets[presetIndex]
                      << (previousPreset ? " (previous)" : skipPreset ? " (manual)"
                          : timelineTransition ? " (timeline section)"
                          : musicalTransition ? " (automatic on boundary)"
                          : " (automatic on deadline)")
                      << ", energy: " << energyName << "\n";
            if (musicalTransition && confidentStructure) {
                std::cerr << "structure: "
                          << (sectionBoundaryThisFrame ? "section" : "phrase fallback")
                          << ", novelty=" << structureNovelty << "\n";
                if (nativeSectionTransition) {
                    std::cerr << "motif: " << structureMotifIdentity
                              << (structureMotifRecalled ? " recalled" : " new")
                              << ", family="
                              << visualFamily(profileForPreset(presets[presetIndex]))
                              << "\n";
                }
            }
        }

        if (presetTransitionActive
            && now - presetTransitionStartedAt >= presetTransitionDuration) {
            activeEngine = 1 - activeEngine;
            presetTransitionActive = false;
            if (pairedFollower || timelineEligible) {
                transitionWindowAt = UINT64_MAX;
                transitionDeadlineAt = UINT64_MAX;
            } else if (structureClockLocked) {
                const float barMilliseconds = 4.0f * 60000.0f
                                            / std::max(60.0f, audioFeatures.bpm);
                transitionWindowAt = now + static_cast<uint64_t>(6.0f * barMilliseconds);
                transitionDeadlineAt = now + static_cast<uint64_t>(12.0f * barMilliseconds);
            } else {
                const PresetProfile& profile = profileForPreset(presets[presetIndex]);
                std::uniform_int_distribution<uint64_t> dwellTime(profile.dwellMinMs,
                                                                   profile.dwellMaxMs);
                const uint64_t dwell = dwellTime(randomEngine);
                transitionWindowAt = now + dwell;
                transitionDeadlineAt = transitionWindowAt + 4000;
            }
        }

        const float frameSeconds = std::min(0.1f, (now - previousFrameAt) / 1000.0f);
        previousFrameAt = now;
        if (nativeEnabled) {
            const bool manualSceneRequest
                = !calibrationMode && (skipPreset || previousPreset);
            if (skipPreset) nativeSceneDirector.requestNext();
            if (previousPreset) nativeSceneDirector.requestPrevious();
            const bool scriptedLeader = !scriptedScenes.empty() && !pairedFollower;
            const bool coverPresentationComplete
                = coverPresentation.frame(now).complete;
            const bool scriptedBarBoundary
                = musicFrame.clockConfidence >= 0.35f
               && scriptedPreviousBarPhase > 0.72f
               && musicFrame.barPhase < 0.28f;
            if (scriptedLeader && windowShown && coverPresentationComplete
                && !nativeSceneDirector.state().transitioning) {
                if (scriptedSceneSettledAt == 0) {
                    scriptedSceneSettledAt = now;
                } else {
                    const float scriptedDwell
                        = (now - scriptedSceneSettledAt) / 1000.0f;
                    const bool scriptedCue = scriptedDwell
                            >= scriptedSceneMaximumSeconds
                        || (scriptedDwell >= scriptedSceneDwellSeconds
                            && scriptedBarBoundary);
                    if (scriptedCue) {
                        if (scriptedSceneIndex + 1 < scriptedScenes.size()) {
                            ++scriptedSceneIndex;
                            nativeSceneDirector.requestScene(
                                scriptedScenes[scriptedSceneIndex]);
                            scriptedSceneSettledAt = 0;
                        } else if (scriptedSequenceOnce) {
                            if (!recordStopSignaled) {
                                signalRecordingMarker(recordStopPath);
                                recordStopSignaled = true;
                            }
                            closeRequested = true;
                            publishPairedState(
                                presetIndex, 0, 6, false,
                                static_cast<int>(
                                    nativeSceneDirector.state().currentScene),
                                static_cast<int>(
                                    nativeSceneDirector.state().currentScene));
                        } else {
                            scriptedSceneIndex = 0;
                            nativeSceneDirector.requestScene(
                                scriptedScenes.front());
                            scriptedSceneSettledAt = 0;
                        }
                    }
                }
            }
            nativeSceneState = nativeSceneDirector.update(
                musicFrame, frameSeconds,
                !pairedFollower && scriptedScenes.empty() && !calibrationMode);
            if (nativeSceneState.transitioning && !nativeTransitionWasActive) {
                const int authoredTransitionMode
                    = static_cast<int>(nativeSceneState.transitionStyle);
                std::cerr << "native scene: "
                          << nativeSceneName(nativeSceneState.currentScene) << " -> "
                          << nativeSceneName(nativeSceneState.incomingScene)
                          << (nativeSceneState.motifRecalled ? " (motif recall)" : "")
                          << "\n";
                publishPairedState(
                    presetIndex, 0, authoredTransitionMode, false,
                    static_cast<int>(nativeSceneState.incomingScene),
                    static_cast<int>(nativeSceneState.currentScene),
                    manualSceneRequest);
                if (manualSceneRequest) {
                    statusOverlay.show(
                        std::string("AUTO: ")
                            + nativeSceneName(nativeSceneState.incomingScene),
                        now);
                }
            } else if (!nativeSceneState.transitioning && nativeTransitionWasActive) {
                std::cerr << "native scene: "
                          << nativeSceneName(nativeSceneState.currentScene) << "\n";
                if (scriptedLeader) scriptedSceneSettledAt = now;
            }
            nativeTransitionWasActive = nativeSceneState.transitioning;
            scriptedPreviousBarPhase = musicFrame.barPhase;
        }
        bassImpact *= std::exp(-6.5f * frameSeconds);
        midImpact *= std::exp(-8.0f * frameSeconds);
        trebleImpact *= std::exp(-13.0f * frameSeconds);
        const float normalizedBass = std::clamp(
            (std::max(audioFeatures.level[0], audioFeatures.level[1]) - 0.75f) / 1.75f,
            0.0f, 1.0f);
        const float normalizedMid = std::clamp(
            (std::max({audioFeatures.level[2], audioFeatures.level[3],
                       audioFeatures.level[4]}) - 0.75f) / 1.75f,
            0.0f, 1.0f);
        const float normalizedTreble = std::clamp(
            (audioFeatures.level[5] - 0.75f) / 1.75f, 0.0f, 1.0f);
        const CoverPresentationFrame coverFrame = coverPresentation.frame(now);
        const float coverBlend = coverFrame.coverMix;
        const float paletteInfluence = coverFrame.paletteInfluence;

        int outputW = 0, outputH = 0;
        SDL_GL_GetDrawableSize(window, &outputW, &outputH);
        if (outputW > 0 && outputH > 0 && (outputW != sourceWidth || outputH != sourceHeight)) {
            sourceWidth = outputW;
            sourceHeight = outputH;
            for (int i = 0; i < 2; ++i) {
                projectm_set_window_size(engines[i], sourceWidth, sourceHeight);
                glBindTexture(GL_TEXTURE_2D, frameTextures[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sourceWidth, sourceHeight,
                             0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            }
        }
        auto renderEngine = [&](int index) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, sourceWidth, sourceHeight);
            projectm_opengl_render_frame(engines[index]);
            glReadBuffer(GL_BACK);
            glBindTexture(GL_TEXTURE_2D, frameTextures[index]);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sourceWidth, sourceHeight);
        };
        GLuint renderedSourceTexture = frameTextures[activeEngine];
        GLuint renderedNextTexture
            = frameTextures[presetTransitionActive ? 1 - activeEngine : activeEngine];
        if (nativeEnabled) {
            std::string error;
            const NativeRenderPolicy renderPolicy{
                .intensity = preferences.intensity,
                .motion = preferences.motion,
                .reducedMotion = preferences.reducedMotion,
                .flashLimited = preferences.flashLimited,
            };
            if (!nativeRenderer->render(musicFrame, nativeSceneState,
                                        sourceWidth, sourceHeight, albumColor,
                                        coverPresentation.hasArtwork()
                                            ? coverTexture : 0,
                                        coverAspect,
                                        frameSeconds, error, renderPolicy)) {
                std::cerr << "native renderer: " << error << "\n";
                running = false;
                continue;
            }
            renderedSourceTexture
                = nativeRenderer->texture(nativeSceneState.currentScene);
            renderedNextTexture = nativeSceneState.transitioning
                ? nativeRenderer->texture(nativeSceneState.incomingScene)
                : renderedSourceTexture;
        } else {
            renderEngine(activeEngine);
            if (presetTransitionActive) renderEngine(1 - activeEngine);
        }

        const float presetBlend = nativeEnabled ? nativeSceneState.transition
            : presetTransitionActive
                ? std::clamp((now - presetTransitionStartedAt)
                             / static_cast<float>(presetTransitionDuration), 0.0f, 1.0f)
                : 0.0f;

        const PresetProfile& sourceProfile = profileForPreset(presets[
            presetTransitionActive ? transitionOutgoingPresetIndex : presetIndex]);
        const PresetProfile& nextProfile = profileForPreset(presets[presetIndex]);
        float displayAsciiExposure = sourceProfile.asciiExposure
                                   * (1.0f - presetBlend)
                                   + nextProfile.asciiExposure * presetBlend;
        float displayFieldExposure = 1.0f;
        if (nativeEnabled) {
            const NativeSceneMaterial sourceMaterial
                = nativeSceneMaterial(nativeSceneState.currentScene);
            const NativeSceneMaterial nextMaterial = nativeSceneMaterial(
                nativeSceneState.transitioning ? nativeSceneState.incomingScene
                                               : nativeSceneState.currentScene);
            displayAsciiExposure = sourceMaterial.asciiExposure
                                 * (1.0f - presetBlend)
                                 + nextMaterial.asciiExposure * presetBlend;
            displayFieldExposure = sourceMaterial.fieldExposure
                                 * (1.0f - presetBlend)
                                 + nextMaterial.fieldExposure * presetBlend;
        }
        const auto ease = [](float value) {
            const float position = std::clamp(value, 0.0f, 1.0f);
            return position * position * (3.0f - 2.0f * position);
        };
        const float entrance = startGateOpened && revealStartedAt > 0
            ? ease((now - revealStartedAt) / 480.0f) : 0.0f;
        const float exit = closing
            ? 1.0f - ease((now - closeStartedAt)
                / static_cast<float>(closeDurationMs)) : 1.0f;
        const int displayTransitionMode = nativeEnabled
            ? nativeSceneState.transitioning
                ? static_cast<int>(nativeSceneState.transitionStyle)
                : static_cast<int>(NativeTransitionStyle::FlowCarry)
            : transitionMode;
        const LiveCompositorFrame displayFrame{
            .sourceTexture = renderedSourceTexture,
            .nextTexture = renderedNextTexture,
            .coverTexture = coverTexture,
            .width = outputW,
            .height = outputH,
            .sceneMix = presetBlend,
            .transitionMode = displayTransitionMode,
            .sourceReactionMode = reactionMode(sourceProfile),
            .nextReactionMode = reactionMode(nextProfile),
            .sourceReactionGain = nativeEnabled
                ? std::array<float, 3>{}
                : std::array<float, 3>{
                    sourceProfile.kickGain * reactionScale,
                    sourceProfile.snareGain * reactionScale,
                    sourceProfile.hatGain * reactionScale},
            .nextReactionGain = nativeEnabled
                ? std::array<float, 3>{}
                : std::array<float, 3>{
                    nextProfile.kickGain * reactionScale,
                    nextProfile.snareGain * reactionScale,
                    nextProfile.hatGain * reactionScale},
            .asciiExposure = displayAsciiExposure
                * (preferences.flashLimited
                    ? std::min(preferences.brightness, 0.90f)
                    : preferences.brightness),
            .fieldExposure = displayFieldExposure
                * (preferences.flashLimited
                    ? std::min(preferences.brightness, 0.90f)
                    : preferences.brightness),
            .nativeRenderer = nativeEnabled,
            .coverAspect = coverAspect,
            .coverMix = coverBlend,
            .albumColor = albumColor,
            .paletteInfluence = nativeEnabled ? 0.0f : paletteInfluence,
            .bassLevel = normalizedBass,
            .bassImpact = bassImpact,
            .midLevel = normalizedMid,
            .trebleLevel = normalizedTreble,
            .midImpact = midImpact,
            .trebleImpact = trebleImpact,
            .asciiEnabled = asciiEnabled,
            .motionScale = preferences.reducedMotion
                ? std::min(preferences.motion, 0.35f) : preferences.motion,
            .contrastScale = preferences.highContrast
                && !preferences.flashLimited ? 1.16f : 1.0f,
            .flashLimited = preferences.flashLimited,
            .colorVisionSafe = preferences.colorVisionSafe,
            .visibility = entrance * exit,
        };
        if (!compositor.render(displayFrame, compositorError)) {
            std::cerr << "display compositor: " << compositorError << "\n";
            running = false;
            continue;
        }
        if (calibrationMode
            && (calibrationStatusAt == 0 || now - calibrationStatusAt >= 1100)) {
            statusOverlay.show(
                "SYNC " + std::to_string(syncDelayMs)
                    + " MS  [ EARLIER  ] LATER  ESC DONE",
                now);
            calibrationStatusAt = now;
        }
        if (!statusOverlay.render(outputW, outputH, now, compositorError)) {
            std::cerr << "status overlay: " << compositorError << "\n";
            running = false;
            continue;
        }
        if (!startGateOpened) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        SDL_GL_SwapWindow(window);
        if (!windowShown
            && (coverPresentation.hasArtwork() || artLookupComplete)
            && (!nativeEnabled || !pairedFollower || reportedPairedMusic)) {
            if (!startReadyPath.empty()) {
                std::ofstream ready(startReadyPath, std::ios::trunc);
                ready << getpid() << '\n';
            }
            SDL_ShowWindow(window);
            signalRecordingMarker(readyPath);
            if (startGatePath.empty()) revealStartedAt = now;
            SDL_DisableScreenSaver();
            windowShown = true;
            std::cerr << "idle: inhibition requested while Omadrop is visible\n";
        }
        // SDL's swap interval is not reliably honored by every Wayland path.
        // MilkDrop presets contain equations that advance once per rendered
        // frame, so an uncapped 250-300 FPS loop looks roughly five times too
        // fast. Keep the engine on a real 60 Hz clock regardless of compositor.
        const auto afterSwap = FrameClock::now();
        if (nextFrame > afterSwap) std::this_thread::sleep_until(nextFrame);
        nextFrame += frameInterval;
        if (nextFrame < FrameClock::now() - frameInterval) {
            nextFrame = FrameClock::now() + frameInterval;
        }
    }

    mprisPoller.stop();
    audioCapture.stop();
    nativeRenderer.reset();
    projectm_destroy(engines[0]);
    projectm_destroy(engines[1]);
    compositor.shutdown();
    statusOverlay.shutdown();
    glDeleteTextures(2, frameTextures.data());
    glDeleteTextures(1, &coverTexture);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
