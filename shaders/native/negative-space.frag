#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float ellipseDistance(vec2 p, vec2 radii) {
    return (length(p / radii) - 1.0) * min(radii.x, radii.y);
}

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(0.0001, dot(ab, ab)), 0.0, 1.0);
    return length(p - a - ab * position);
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.66, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;
    float lowSustain = 1.0 - exp(
        -0.34 * (bandLevel[0] + bandLevel[1]));
    float midSustain = 1.0 - exp(
        -0.34 * (bandLevel[2] + bandLevel[3]));
    float highSustain = 1.0 - exp(
        -0.34 * (bandLevel[4] + bandLevel[5]));

    vec2 previousP = p;
    previousP.x -= 0.000025 * harmonic * motionScale;
    previousP.y += 0.000018 * energySlow * motionScale;
    vec2 previousUv = previousP / aspect + 0.5;
    float edge = smoothstep(0.0, 0.08, uv.x) * smoothstep(0.0, 0.08, uv.y)
               * smoothstep(0.0, 0.08, 1.0 - uv.x)
               * smoothstep(0.0, 0.08, 1.0 - uv.y);
    vec3 feedback = texture(previousFrame, clamp(previousUv, 0.001, 0.999)).rgb
                  * mix(0.845, 0.905, harmonic) * edge;

    float upperBoundary = 0.34 + 0.065 * sin(p.x * 2.25 + 0.4)
                        + 0.018 * tonalMotion;
    float lowerBoundary = -0.34 + 0.055 * sin(p.x * 2.85 - 0.8);
    float mainSlab = smoothstep(lowerBoundary - 0.035,
                                lowerBoundary + 0.020, p.y)
                   * smoothstep(upperBoundary + 0.035,
                                upperBoundary - 0.020, p.y);
    float upperRibbonCenter = 0.36 + 0.075 * sin(p.x * 1.70 - 0.9)
                            - 0.025 * harmonicChange;
    float upperSlab = 1.0 - smoothstep(
        0.065, 0.105, abs(p.y - upperRibbonCenter));
    upperSlab *= smoothstep(-0.52, -0.12, p.x);
    float body = max(mainSlab, upperSlab * (0.55 + 0.35 * harmonic));

    vec2 mainVoidP = p - vec2(0.31, -0.025);
    mainVoidP.x += 0.026 * sin(mainVoidP.y * 8.0 + 1.2);
    mainVoidP.y += 0.014 * sin(mainVoidP.x * 10.0 - 0.7);
    float mainVoidDistance = ellipseDistance(
        mainVoidP, vec2(0.225, 0.350));
    float mainVoid = 1.0 - smoothstep(-0.012, 0.025, mainVoidDistance);
    float mainRim = line(mainVoidDistance, 0.008 + 0.004 * downbeat);
    float voidAngle = atan(mainVoidP.y / 0.350, mainVoidP.x / 0.225);
    float beatArc = smoothstep(0.42, 0.70,
        cos(voidAngle + 1.15 + beatPhase * 0.45));
    float beatMarker = line(mainVoidDistance - 0.031, 0.0045)
                     * beatArc * beatPulse;
    float anticipationArc = smoothstep(0.70, 0.88,
        cos(voidAngle - 2.20));
    float anticipationMarker = line(mainVoidDistance - 0.052, 0.0035)
                             * anticipationArc * beatAnticipation;
    float contours = 0.0;
    for (int index = 0; index < 4; ++index) {
        float offset = 0.045 + float(index) * 0.052;
        float broken = smoothstep(-0.35, 0.30,
            sin(atan(mainVoidP.y, mainVoidP.x) * 3.0 + float(index) * 1.7));
        contours += line(mainVoidDistance - offset, 0.0035) * broken;
    }

    float kickPresence = smoothstep(0.08, 0.30, sceneKick);
    float kickRadius = 0.020 + 0.070 * sceneKick;
    float kickVoidDistance = length(p - vec2(-0.42, -0.15)) - kickRadius;
    float kickVoid = 1.0 - smoothstep(-0.008, 0.020, kickVoidDistance);
    kickVoid *= kickPresence;
    float kickRim = line(kickVoidDistance, 0.006) * sceneKick;

    vec2 snareA = vec2(0.31, -0.28);
    vec2 snareB = vec2(0.58, 0.22);
    float snareCutDistance = segmentDistance(p, snareA, snareB);
    float snareCut = 1.0 - smoothstep(
        0.006 + 0.015 * sceneSnare,
        0.014 + 0.027 * sceneSnare, snareCutDistance);
    snareCut *= smoothstep(0.04, 0.28, sceneSnare);

    float perforations = 0.0;
    float perforationRims = 0.0;
    float highEtching = 0.0;
    for (int index = 0; index < 11; ++index) {
        float fi = float(index);
        vec2 center = vec2(-0.48 + fi * 0.095,
                           0.195 + 0.045 * sin(fi * 1.78));
        float localHat = sceneHat * (0.58 + 0.42 * sin(fi * 2.27 + 1.1));
        float radius = 0.003 + 0.029 * max(0.0, localHat)
                     + 0.001 * spectrumLevel[index * 2 + 4];
        float distanceToHole = length(p - center) - radius;
        perforations = max(perforations,
            (1.0 - smoothstep(-0.003, 0.008, distanceToHole))
            * smoothstep(0.06, 0.24, sceneHat));
        perforationRims += line(distanceToHole, 0.0035) * sceneHat;
        highEtching += line(distanceToHole - 0.009, 0.0025)
                     * highSustain * (0.45 + 0.55 * step(5.0, fi));
    }

    float lowContour = line(
        length(p - vec2(-0.42, -0.15)) - (0.064 + 0.018 * lowSustain),
        0.005) * lowSustain;
    float midThread = line(segmentDistance(
        p, vec2(-0.34, -0.02), vec2(0.02, 0.24)), 0.0045)
        * midSustain * body;

    float sectionCut = 1.0 - smoothstep(
        0.004, 0.022 + 0.060 * section,
        abs(p.x + 0.16 + 0.12 * sin(p.y * 4.0)));
    sectionCut *= section;
    float carved = clamp(max(mainVoid, kickVoid) + snareCut
                       + perforations + sectionCut, 0.0, 1.0);

    float paperGrain = 0.94 + 0.06 * sin(p.x * 41.0 + p.y * 17.0)
                            * sin(p.y * 53.0 - p.x * 11.0);
    float horizon = smoothstep(-0.40, 0.38, p.y + 0.10 * p.x);
    vec3 primary = palettePrimary(0.48);
    vec3 secondary = paletteSecondary(0.48);
    vec3 accent = paletteAccent(0.48);
    vec3 paperColor = mix(primary, secondary, horizon * 0.54
                         + tonalMotion * 0.18);
    paperColor = mix(paperColor, vec3(1.0, 0.91, 0.68), 0.16);
    vec3 injection = paperColor * body * paperGrain
                   * (0.065 + 0.025 * harmonic + 0.015 * development)
                   + accent * mainRim * (0.030 + 0.10 * downbeat)
                   + accent * beatMarker * 0.15
                   + secondary * anticipationMarker * 0.11
                   + mix(secondary, accent, 0.35) * contours * body * 0.032
                   + secondary * kickRim * 0.16
                   + accent * perforationRims * 0.30
                   + primary * lowContour * 0.12
                   + secondary * midThread * 0.13
                   + mix(accent, vec3(1.0), 0.25) * highEtching * 0.08;
    injection *= 1.0 - 0.62 * release;

    vec3 result = feedback + injection;
    result *= 1.0 - carved * 0.94;
    result += accent * line(snareCutDistance,
        0.003 + 0.003 * sceneSnare) * sceneSnare * 0.10;
    result = max(result - vec3(0.0038), vec3(0.0));
    color = vec4(result, 1.0);
}
