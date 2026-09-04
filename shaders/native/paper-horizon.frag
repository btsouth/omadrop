#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(0.0001, dot(ab, ab)), 0.0, 1.0);
    return length(p - a - ab * position);
}

float ridgeY(float x, float layer) {
    if (layer < 0.5) {
        return 0.00 + 0.080 * sin(x * 2.5 + 0.7)
                    + 0.030 * sin(x * 6.0 - 0.4);
    }
    if (layer < 1.5) {
        return -0.11 + 0.095 * sin(x * 3.1 - 1.0)
                     + 0.024 * sin(x * 8.0 + 0.8);
    }
    if (layer < 2.5) {
        return -0.22 + 0.075 * sin(x * 2.7 + 1.8)
                     + 0.034 * sin(x * 5.2 - 0.6);
    }
    return -0.34 + 0.060 * sin(x * 3.6 - 0.2)
                 + 0.020 * sin(x * 9.0 + 0.5);
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

    vec3 primary = mix(palettePrimary(5.81), vec3(0.05, 0.52, 0.88), 0.13);
    vec3 secondary = mix(paletteSecondary(5.81), vec3(0.92, 0.18, 0.62), 0.12);
    vec3 accent = mix(paletteAccent(5.81), vec3(1.0, 0.72, 0.16), 0.10);

    float skyWindow = smoothstep(-0.90, -0.82, p.x)
                    * smoothstep(0.90, 0.82, p.x)
                    * smoothstep(-0.50, -0.44, p.y)
                    * smoothstep(0.50, 0.44, p.y);
    float skyGradient = smoothstep(-0.38, 0.46, p.y);
    float paperGrain = 0.5 + 0.5 * sin(p.x * 87.0 + p.y * 31.0)
                                 * sin(p.y * 73.0 - p.x * 19.0);
    vec3 skyColor = mix(mix(primary, secondary, 0.38) * 0.32,
                        mix(primary, secondary, 0.62) * 0.62,
                        skyGradient);
    vec3 result = skyColor * skyWindow * (0.026 + 0.005 * paperGrain);

    vec2 moonCenter = vec2(0.37, 0.22);
    float moonDistance = length((p - moonCenter) / vec2(1.0, 0.96));
    float moonDisc = 1.0 - smoothstep(0.115, 0.128, moonDistance);
    float moonCut = 1.0 - smoothstep(0.078, 0.108,
        length((p - moonCenter - vec2(0.047, 0.018)) / vec2(1.0, 0.96)));
    float moonCrescent = moonDisc * (1.0 - moonCut);
    float moonEdge = line(moonDistance - 0.120, 0.006) * (1.0 - moonCut);
    result += mix(primary, accent, 0.56) * moonCrescent * 0.086
            + accent * moonEdge * 0.042;

    float layerMasks[4];
    float layerEdges[4];
    vec3 layerContribution = vec3(0.0);
    for (int index = 0; index < 4; ++index) {
        float fi = float(index);
        float ridge = ridgeY(p.x, fi);
        float mask = smoothstep(ridge + 0.012, ridge - 0.010, p.y)
                   * skyWindow;
        float edge = line(p.y - ridge, 0.008) * skyWindow;
        layerMasks[index] = mask;
        layerEdges[index] = edge;
        float depth = fi / 3.0;
        vec3 layerColor = mix(primary, secondary,
            clamp(0.16 + depth * 0.68, 0.0, 1.0));
        layerColor *= mix(0.78, 0.40, depth);
        float emboss = 0.5 + 0.5 * sin(p.x * (9.0 + fi * 2.0)
                                     + p.y * 7.0 + fi * 1.9);
        float level = 1.0 - exp(-0.18 * bandLevel[index + 1]);
        float inlay = smoothstep(0.82, 0.95, emboss) * level;
        layerContribution = mix(layerContribution,
            layerColor * (0.070 + 0.008 * paperGrain)
            + mix(layerColor, accent, 0.22) * inlay * 0.016,
            mask);
        layerContribution += mix(layerColor, accent, 0.18)
                           * edge * (0.036 + 0.010 * (1.0 - depth));
    }
    result += layerContribution;

    // Beat timing is one small lantern moving along the distant ridge.
    float lanternX = mix(-0.68, 0.68, beatPhase);
    float lanternY = ridgeY(lanternX, 0.0) + 0.035;
    vec2 lanternPoint = vec2(lanternX, lanternY);
    float lantern = 1.0 - smoothstep(0.010,
        0.027 + 0.005 * beatPulse, length(p - lanternPoint));
    lantern *= clockConfidence * (0.25 + 0.75 * beatPulse);
    float lanternGlow = 1.0 - smoothstep(0.030, 0.070,
        length(p - lanternPoint));
    lanternGlow *= beatPulse * clockConfidence;

    // Kick illuminates separate pieces of the foreground edge.
    float frontRidge = ridgeY(p.x, 3.0);
    float leftKick = exp(-pow((p.x + 0.48) / 0.13, 2.0));
    float rightKick = exp(-pow((p.x - 0.43) / 0.11, 2.0));
    float kickEdge = line(p.y - frontRidge,
        0.008 + 0.005 * sceneKick) * (leftKick + rightKick) * sceneKick;
    float kickPocket = (1.0 - smoothstep(0.035, 0.095,
        length((p - vec2(-0.48, frontRidge)) / vec2(1.0, 0.52))))
        * sceneKick * layerMasks[3];

    // Snare tears three short openings in the second paper layer.
    float snareTears = 0.0;
    float snareCuts = 0.0;
    for (int index = 0; index < 3; ++index) {
        float offset = (float(index) - 1.0) * 0.060;
        vec2 a = vec2(-0.08 + offset, -0.29);
        vec2 b = vec2(0.10 + offset, -0.18);
        float distanceToTear = segmentDistance(p, a, b);
        snareTears += line(distanceToTear, 0.0035)
                    * sceneSnare * layerMasks[1];
        snareCuts += (1.0 - smoothstep(0.003,
            0.008 + 0.006 * sceneSnare, distanceToTear))
            * smoothstep(0.06, 0.28, sceneSnare) * layerMasks[1];
    }

    // Hats light a sparse set of fixed star perforations.
    float restingStars = 0.0;
    float stars = 0.0;
    for (int index = 0; index < 6; ++index) {
        float fi = float(index);
        vec2 point = vec2(-0.63 + fi * 0.21,
            0.27 + 0.095 * sin(fi * 2.15 + 0.4));
        float star = 1.0 - smoothstep(0.004,
            0.011 + 0.004 * sceneHat, length(p - point));
        float gate = index == 1 || index == 4 ? 0.45 : 1.0;
        restingStars += star * (0.28 + 0.16 * gate);
        stars += star * sceneHat * gate;
    }

    float anticipation = line(p.x + 0.76, 0.004)
                       * line(p.y - 0.35, 0.020)
                       * beatAnticipation * clockConfidence;
    float downbeatMoon = line(moonDistance - 0.145, 0.008)
                       * downbeat * (1.0 - moonCut);
    float sectionHalo = line(moonDistance - 0.190, 0.010) * section;
    float sectionPeak = line(p.y - ridgeY(p.x, 0.0) - 0.045, 0.005)
                      * smoothstep(-0.15, 0.32, p.x)
                      * smoothstep(0.72, 0.32, p.x) * section;

    result += mix(primary, accent, 0.55) * restingStars * 0.045
            + primary * layerEdges[3]
              * exp(-5.0 * (p.x + 0.38) * (p.x + 0.38))
              * lowSustain * 0.095
            + secondary * layerEdges[1]
              * exp(-6.0 * (p.x - 0.02) * (p.x - 0.02))
              * midSustain * 0.085
            + mix(accent, vec3(1.0), 0.24) * restingStars
              * highSustain * 0.34
            + accent * lantern * 0.24
            + accent * lanternGlow * 0.045
            + primary * kickEdge * 0.24
            + mix(primary, accent, 0.25) * kickPocket * 0.12
            + mix(accent, vec3(1.0), 0.28) * snareTears * 0.29
            + accent * stars * 0.34
            + secondary * anticipation * 0.11
            + accent * downbeatMoon * 0.14
            + secondary * (sectionHalo + sectionPeak) * 0.11;
    result *= 1.0 - clamp(snareCuts * 0.52, 0.0, 0.60);
    result *= 1.0 - 0.62 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
