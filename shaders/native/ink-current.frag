#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(0.0001, dot(ab, ab)), 0.0, 1.0);
    return length(p - a - ab * position);
}

float strokeCenter(float x, float offset, float phase) {
    return offset + 0.105 * sin(x * 2.25 + phase)
         + 0.030 * sin(x * 5.70 - phase * 0.70);
}

float hash21(vec2 value) {
    return fract(sin(dot(value, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.68, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;

    vec2 previousP = p;
    previousP.x -= 0.000020 * energySlow * motionScale;
    previousP.y += 0.000016 * harmonic * motionScale;
    vec2 previousUv = previousP / aspect + 0.5;
    float edge = smoothstep(0.0, 0.075, uv.x)
               * smoothstep(0.0, 0.075, uv.y)
               * smoothstep(0.0, 0.075, 1.0 - uv.x)
               * smoothstep(0.0, 0.075, 1.0 - uv.y);
    vec3 feedback = texture(previousFrame, clamp(previousUv, 0.001, 0.999)).rgb
                  * mix(0.855, 0.915, harmonic) * edge;

    float bassFlow = smoothstep(0.18, 0.90, bandLevel[0]);
    float mainCenter = -0.09 + 0.17 * sin(p.x * 2.05 + 0.45)
                     + 0.055 * sin(p.x * 5.10 - 0.70)
                     + 0.065 * p.x
                     + 0.025 * tonalMotion * sin(p.x * 3.4 - 0.7);
    float localSpectrum = 0.0;
    float localLift = 0.0;
    for (int index = 0; index < 6; ++index) {
        float fi = float(index);
        float zoneX = -0.55 + fi * 0.22;
        float influence = exp(-75.0 * (p.x - zoneX) * (p.x - zoneX));
        float level = 1.0 - exp(-0.22 * spectrumLevel[index * 5 + 2]);
        localSpectrum += influence * level;
        localLift += influence * level * (mod(fi, 2.0) * 2.0 - 1.0);
    }
    mainCenter += 0.009 * localLift;
    float mainTaper = exp(-1.35 * (p.x + 0.18) * (p.x + 0.18));
    float mainWidth = (0.045 + 0.032 * mainTaper)
                    * (0.88 + 0.24 * bassFlow)
                    + 0.009 * localSpectrum
                    + 0.005 * sin(p.x * 9.2 + 0.6);
    float fiberNoise = sin(p.x * 71.0 + p.y * 37.0)
                     * sin(p.x * 29.0 - p.y * 83.0);
    float mainDistance = abs(p.y - mainCenter)
                       + 0.0045 * fiberNoise;
    float mainStroke = 1.0 - smoothstep(mainWidth, mainWidth + 0.012,
                                        mainDistance);

    float strokeMask = smoothstep(0.83, 0.64, abs(p.x));
    mainStroke *= strokeMask;
    float edgeContour = line(abs(p.y - mainCenter) - mainWidth, 0.004);
    float tributaryGate = (1.0 - smoothstep(-0.42, 0.22, p.x)) * strokeMask;
    float tributaryCenter = mainCenter + 0.13 - 0.11 * (p.x + 0.42);
    float tributary = line(p.y - tributaryCenter,
        0.004 + 0.003 * spectrumLevel[18]) * tributaryGate;
    float eddyCenterY = -0.09 + 0.17 * sin(0.30 * 2.05 + 0.45)
                       + 0.055 * sin(0.30 * 5.10 - 0.70)
                       + 0.065 * 0.30;
    vec2 eddyCenter = vec2(0.30, eddyCenterY + 0.01);
    vec2 eddyP = p - eddyCenter;
    float eddyAngle = atan(eddyP.y, eddyP.x);
    float harmonicEddy = 0.0;
    for (int index = 0; index < 4; ++index) {
        float fi = float(index);
        float eddyRadius = 0.045 + fi * 0.027
                         + 0.009 * sin(eddyAngle * 2.0 - fi * 0.8);
        float gate = smoothstep(-0.72, 0.18,
            sin(eddyAngle + 0.45 + fi * 0.38));
        harmonicEddy += line(length(eddyP) - eddyRadius, 0.0028)
                      * gate * (0.42 + 0.58 * harmonic);
    }

    float bristles = 0.0;
    for (int index = 0; index < 5; ++index) {
        float fi = float(index) - 2.0;
        float offset = fi * (0.007 + 0.002 * sin(p.x * 13.0 + fi));
        float broken = smoothstep(-0.42, 0.34,
            sin(p.x * (19.0 + fi) + fi * 2.1));
        bristles += line(p.y - mainCenter - offset, 0.0018) * broken;
    }
    bristles *= strokeMask;

    float beatX = mix(-0.62, 0.62, beatPhase);
    float beatY = strokeCenter(beatX, -0.03, 0.35);
    float beatBead = 1.0 - smoothstep(
        0.009, 0.021 + 0.010 * beatPulse,
        length(p - vec2(beatX, beatY)));
    beatBead *= beatPulse * clockConfidence;
    float beatWake = line(p.y - beatY, 0.0045)
                   * smoothstep(0.18, 0.0, abs(p.x - beatX + 0.075))
                   * beatPulse * clockConfidence;
    float anticipationMark = line(p.x - beatX - 0.045, 0.004)
                           * line(p.y - beatY, 0.016)
                           * beatAnticipation * clockConfidence;
    vec2 downbeatCenter = vec2(-0.60,
        -0.09 + 0.17 * sin(-0.60 * 2.05 + 0.45)
        + 0.055 * sin(-0.60 * 5.10 - 0.70) - 0.039);
    float downbeatSeal = line(length(p - downbeatCenter) - 0.026,
                              0.0045)
                       * downbeat * clockConfidence;

    float vortices = 0.0;
    float vortexCores = 0.0;
    for (int index = 0; index < 3; ++index) {
        float fi = float(index);
        float x = -0.46 + fi * 0.43;
        float y = strokeCenter(x, -0.03, 0.35);
        float radius = 0.012 + 0.022 * sceneKick
                     + 0.002 * spectrumLevel[index * 4 + 1];
        float vortexDistance = length(p - vec2(x, y)) - radius;
        float gate = 0.62 + 0.38 * smoothstep(
            -0.15, 0.65, sin(fi * 2.4 + barPhase * tau));
        vortices += line(vortexDistance, 0.0065) * sceneKick * gate;
        vortexCores += (1.0 - smoothstep(
            radius * 0.38, radius * 0.72,
            length(p - vec2(x, y)))) * sceneKick * gate;
    }

    vec2 snareA = vec2(-0.18, -0.34);
    vec2 snareB = vec2(0.18, 0.31);
    float snareDistance = segmentDistance(p, snareA, snareB);
    float snarePresence = smoothstep(0.06, 0.28, sceneSnare);
    float snareCut = (1.0 - smoothstep(
        0.004, 0.010 + 0.013 * sceneSnare, snareDistance)) * snarePresence;
    float snareEdge = line(snareDistance,
        0.003 + 0.003 * sceneSnare) * sceneSnare;

    float droplets = 0.0;
    for (int index = 0; index < 9; ++index) {
        float fi = float(index);
        float angle = -0.15 + fi * 0.34;
        vec2 center = eddyCenter
                    + vec2(cos(angle), sin(angle))
                    * (0.15 + 0.014 * sin(fi * 2.05));
        float localHat = sceneHat * (0.68 + 0.32 * sin(fi * 2.73));
        float radius = 0.004 + 0.013 * max(0.0, localHat)
                     + 0.0015 * spectrumLevel[index * 3 + 4];
        droplets += (1.0 - smoothstep(radius, radius + 0.006,
                                      length(p - center)))
                  * smoothstep(0.05, 0.22, sceneHat);
    }

    float sectionBranchCenter = mainCenter + 0.085
                              + 0.18 * max(0.0, p.x + 0.08);
    float sectionBranch = line(p.y - sectionBranchCenter,
        0.004 + 0.009 * section) * section;
    sectionBranch *= smoothstep(-0.12, 0.12, p.x)
                   * (1.0 - smoothstep(0.58, 0.72, p.x));

    vec3 primary = palettePrimary(5.05);
    vec3 secondary = paletteSecondary(5.05);
    vec3 accent = paletteAccent(5.05);
    float paper = 0.92 + 0.08 * sin(p.x * 57.0 + p.y * 31.0)
                             * sin(p.y * 43.0 - p.x * 17.0);
    float dryBrush = 0.78 + 0.22 * smoothstep(-0.30, 0.48,
        sin(p.x * 93.0 + p.y * 41.0)
        * sin(p.y * 77.0 - p.x * 33.0));
    vec3 edgeInk = mix(primary, secondary, 0.28);
    vec3 filamentInk = mix(primary, accent, 0.24);
    vec3 injection = primary * mainStroke * paper
                   * dryBrush
                   * (0.040 + 0.018 * bassFlow + 0.008 * harmonic)
                   + edgeInk * edgeContour * strokeMask * 0.070
                   + filamentInk * bristles * 0.047
                   + edgeInk * tributary * 0.064
                   + filamentInk * harmonicEddy * 0.070
                   + accent * beatBead * 0.29
                   + mix(accent, secondary, 0.42) * beatWake * 0.14
                   + secondary * anticipationMark * 0.12
                   + accent * downbeatSeal * 0.15
                   + edgeInk * vortices * 0.19
                   + mix(edgeInk, accent, 0.45) * vortexCores * 0.10
                   + accent * droplets * 0.17
                   + mix(accent, vec3(1.0), 0.32) * snareEdge * 0.14
                   + edgeInk * sectionBranch * 0.10;
    injection *= 1.0 - 0.58 * release;

    vec3 result = feedback + injection;
    result *= 1.0 - snareCut * 0.86;
    vec2 grainCoordinate = (p + vec2(0.9, 0.5)) * vec2(75.0, 48.0);
    vec2 grainCell = floor(grainCoordinate);
    vec2 grainPoint = fract(grainCoordinate) - 0.5;
    float grainHole = (1.0 - smoothstep(0.10, 0.31, length(grainPoint)))
                    * smoothstep(0.82, 0.96, hash21(grainCell))
                    * mainStroke * 0.42;
    result *= 1.0 - grainHole;
    result = max(result - vec3(0.0041), vec3(0.0));
    color = vec4(result, 1.0);
}
