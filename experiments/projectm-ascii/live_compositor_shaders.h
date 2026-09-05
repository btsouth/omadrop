#pragma once

// Shared verbatim by the live window and offline musical review.
namespace LiveCompositorShaders {
inline constexpr const char* vertexSource = R"GLSL(
#version 330 core
out vec2 uv;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

inline constexpr const char* fragmentSource = R"GLSL(
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
uniform vec2 sourceTransitionAnchor;
uniform vec2 incomingTransitionAnchor;
uniform vec2 sourceTransitionMotion;
uniform vec2 incomingTransitionMotion;
uniform float sourceTransitionDepth;
uniform float incomingTransitionDepth;
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
        vec2 anchor = mix(sourceTransitionAnchor,
                          incomingTransitionAnchor, easedPresetMix);
        vec2 transitionMotion = mix(sourceTransitionMotion,
                                    incomingTransitionMotion, easedPresetMix);
        transitionMotion /= max(0.001, length(transitionMotion));
        float transitionDepth = mix(sourceTransitionDepth,
                                    incomingTransitionDepth, easedPresetMix);
        if (transitionMode == 6) {
            float carry = bridge * 0.026 * motionScale;
            outgoingUv += transitionMotion * carry;
            incomingUv -= transitionMotion * carry;
        } else if (transitionMode == 7) {
            outgoingUv = sourceTransitionAnchor
                       + (sampleUv - sourceTransitionAnchor)
                       * (1.0 + 0.08 * bridge * motionScale);
            incomingUv = incomingTransitionAnchor
                       + (sampleUv - incomingTransitionAnchor)
                       * (1.0 + 0.12 * (1.0 - easedPresetMix) * motionScale);
        } else if (transitionMode == 8) {
            float depthScale = 0.72 + 0.28 * transitionDepth;
            outgoingUv = sourceTransitionAnchor
                       + (sampleUv - sourceTransitionAnchor)
                       * (1.0 - 0.13 * bridge * motionScale * depthScale);
            incomingUv = incomingTransitionAnchor
                       + (sampleUv - incomingTransitionAnchor)
                       * (1.0 + 0.18 * (1.0 - easedPresetMix)
                          * motionScale * depthScale);
        } else if (transitionMode == 9) {
            vec2 q = sampleUv - anchor;
            vec2 direction = normalize(vec2(
                sin((q.x + q.y * 0.72) * 17.0),
                cos((q.x * 0.61 - q.y) * 21.0)) + vec2(0.001));
            direction = normalize(mix(direction, transitionMotion, 0.32));
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
    vec2 transitionAnchor = mix(sourceTransitionAnchor,
                                incomingTransitionAnchor, easedPresetMix);
    vec2 transitionMotion = mix(sourceTransitionMotion,
                                incomingTransitionMotion, easedPresetMix);
    transitionMotion /= max(0.001, length(transitionMotion));
    vec2 transitionNormal = vec2(-transitionMotion.y, transitionMotion.x);
    vec2 transitionQ = sampleUv - transitionAnchor;
    vec2 transitionCoordinates = vec2(dot(transitionQ, transitionMotion),
                                      dot(transitionQ, transitionNormal));
    if (transitionMode == 6) {
        flow = 0.50 + 0.18 * sin((transitionCoordinates.y + 0.5) * 8.0
                               + sin((transitionCoordinates.x + 0.5) * 5.0)
                                 * 1.3)
                    + 0.10 * transitionCoordinates.x;
        // Keep the carry edge broad enough to feel fluid, but narrow enough
        // that two detailed scenes do not spend the middle of the transition
        // as one low-contrast double exposure.
        localMix = smoothstep(flow - 0.16, flow + 0.16, easedPresetMix);
    } else if (transitionMode == 7) {
        float radius = length(transitionQ);
        flow = clamp(radius * 1.18, 0.06, 0.88);
        localMix = smoothstep(flow - 0.11, flow + 0.11, easedPresetMix);
    } else if (transitionMode == 8) {
        float radius = length(transitionQ);
        float depthShape = mix(sourceTransitionDepth,
                               incomingTransitionDepth, easedPresetMix);
        flow = 0.28 + radius * mix(0.62, 0.78, depthShape)
             + 0.055 * sin(radius * 31.0);
        localMix = smoothstep(flow - 0.13, flow + 0.13, easedPresetMix);
    } else if (transitionMode == 9) {
        vec2 q = transitionCoordinates;
        flow = 0.50
             + 0.14 * sin((q.x + q.y * 0.68) * 18.0)
             + 0.13 * sin((q.x * 0.57 - q.y) * 23.0)
             + 0.065 * sin(q.x * 37.0 + q.y * 5.0);
        localMix = smoothstep(flow - 0.09, flow + 0.09, easedPresetMix);
    } else if (transitionMode == 10) {
        vec2 q = transitionCoordinates;
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
        float radius = length(sampleUv - transitionAnchor);
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
    if (transitionMode == 11) {
        // Untouched MilkDrop colors and geometry; both feedback engines remain
        // live throughout a smooth dissolve, including its exact endpoints.
        visual = mix(texture(sourceFrame, sampleUv).rgb,
                     texture(nextFrame, sampleUv).rgb, easedPresetMix);
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
    if (asciiEnabled == 0) visual *= fieldExposure;
    if (coverMix <= 0.0) return visual;
    vec2 p = coverSampleUv - 0.5;
    float screenAspect = resolution.x / resolution.y;
    if (screenAspect > coverAspect) p.x *= screenAspect / coverAspect;
    else p.y *= coverAspect / screenAspect;
    vec2 coverUv = p + 0.5;
    vec3 cover = vec3(0.0);
    float coverPresence = coverMix;
    if (all(greaterThanEqual(coverUv, vec2(0.0))) && all(lessThanEqual(coverUv, vec2(1.0)))) {
        cover = texture(coverFrame, vec2(coverUv.x, 1.0 - coverUv.y)).rgb;
    }
    return mix(visual, cover, coverPresence);
}

void main() {
    if (asciiEnabled == 0) {
        color = vec4(limitFlashBrightness(
            sceneSample(uv)) * visibility, 1.0);
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
}
