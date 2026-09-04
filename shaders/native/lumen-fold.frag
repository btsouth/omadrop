#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(0.0001, dot(ab, ab)), 0.0, 1.0);
    return length(p - a - ab * position);
}

float foldX(float y, float center, float identity) {
    return center
         + 0.050 * sin(y * 5.4 + identity * 1.37)
         + 0.014 * sin(y * 13.0 - identity * 0.82);
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.66, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;

    vec3 primary = mix(palettePrimary(5.78), vec3(0.02, 0.62, 0.96), 0.14);
    vec3 secondary = mix(paletteSecondary(5.78), vec3(0.94, 0.12, 0.70), 0.12);
    vec3 accent = mix(paletteAccent(5.78), vec3(1.0, 0.74, 0.18), 0.10);

    float verticalWindow = smoothstep(-0.38, -0.33, p.y)
                         * smoothstep(0.40, 0.34, p.y);
    float sheets = 0.0;
    float sheetEdges = 0.0;
    float creases = 0.0;
    float internalLight = 0.0;
    float floorReflection = 0.0;
    vec3 sheetColor = vec3(0.0);
    vec3 sheetGlow = vec3(0.0);
    vec3 reflectionColor = vec3(0.0);
    for (int index = 0; index < 5; ++index) {
        float fi = float(index);
        float center = -0.56 + fi * 0.28 + 0.018 * sin(fi * 2.2);
        float x = foldX(p.y, center, fi);
        float halfWidth = 0.086 + 0.014 * mod(fi, 2.0);
        float bottom = -0.36 + 0.035 * sin(fi * 1.7 + 0.4);
        float top = 0.39 - 0.055 * mod(fi + 1.0, 3.0);
        float sheetWindow = smoothstep(bottom, bottom + 0.045, p.y)
                          * smoothstep(top, top - 0.055, p.y);
        float across = p.x - x;
        float body = (1.0 - smoothstep(halfWidth * 0.72,
                                      halfWidth, abs(across)))
                   * sheetWindow;
        float halo = (1.0 - smoothstep(halfWidth * 0.74,
                                      halfWidth + 0.055, abs(across)))
                   * sheetWindow;
        float edge = line(abs(across) - halfWidth * 0.82, 0.008)
                   * sheetWindow;
        float crease = line(across + halfWidth * 0.20
                           * sin(p.y * 8.0 + fi), 0.007)
                     * sheetWindow;
        float level = 1.0 - exp(-0.18 * spectrumLevel[index * 6 + 2]);
        float lightBandY = -0.25 + 0.12 * float(index % 4);
        float lightBand = exp(-pow((p.y - lightBandY) / 0.075, 2.0))
                        * body * level;
        float depth = 0.26 + 0.15 * fi;
        vec3 material = mix(primary, secondary,
            clamp(0.10 + fi * 0.18 + tonalMotion * 0.10, 0.0, 1.0));
        float face = smoothstep(-halfWidth * 0.60,
                                halfWidth * 0.60, across);
        vec3 faceColor = mix(material, mix(material, accent, 0.28), face);
        sheetColor += faceColor * body * (0.038 + 0.014 * depth)
                    + mix(material, accent, 0.28) * edge * 0.052
                    + primary * crease * 0.030;
        sheetGlow += material * halo * (1.0 - body) * 0.010;
        sheets = max(sheets, body);
        sheetEdges += edge;
        creases += crease;
        internalLight += lightBand;

        float reflectedY = -0.72 - p.y;
        float reflectedX = foldX(reflectedY, center, fi);
        float reflectedAcross = p.x - reflectedX;
        float reflection = (1.0 - smoothstep(halfWidth * 0.64,
            halfWidth * 1.10, abs(reflectedAcross)))
            * smoothstep(-0.50, -0.43, p.y)
            * smoothstep(-0.31, -0.38, p.y);
        float reflectionFade = smoothstep(-0.50, -0.36, p.y);
        reflectionColor += material * reflection * reflectionFade
                         * (0.012 + 0.006 * depth);
        floorReflection += reflection * reflectionFade;
    }

    // Beat timing is one packet descending the central sheet.
    float beatY = mix(0.32, -0.31, beatPhase);
    vec2 beatPoint = vec2(foldX(beatY, 0.0, 2.0), beatY);
    float beatPacket = 1.0 - smoothstep(0.014,
        0.034 + 0.006 * beatPulse, length(p - beatPoint));
    beatPacket *= clockConfidence * (0.25 + 0.75 * beatPulse);
    float beatTail = line(segmentDistance(p,
        beatPoint + vec2(0.0, 0.060), beatPoint), 0.004)
        * beatPulse * clockConfidence;

    // Kick lights two small pools where separate sheets meet the floor.
    float kickPools = 0.0;
    vec2 kickA = vec2(-0.40, -0.37);
    vec2 kickB = vec2(0.40, -0.37);
    kickPools += 1.0 - smoothstep(0.06,
        0.12 + 0.02 * sceneKick,
        length((p - kickA) / vec2(1.0, 0.42)));
    kickPools += 1.0 - smoothstep(0.05,
        0.10 + 0.018 * sceneKick,
        length((p - kickB) / vec2(1.0, 0.42)));
    kickPools *= sceneKick;

    // Snare inserts one broken diagonal plane across only the center sheets.
    vec2 snareA = vec2(-0.27, -0.16);
    vec2 snareB = vec2(0.31, 0.21);
    float snareAlong = dot(p - snareA, normalize(snareB - snareA));
    float snareDash = step(0.48, fract(snareAlong * 17.0));
    float snarePlane = line(segmentDistance(p, snareA, snareB), 0.005)
                     * snareDash * sceneSnare;
    float snareCut = (1.0 - smoothstep(0.003,
        0.009 + 0.006 * sceneSnare,
        segmentDistance(p, snareA, snareB)))
        * snareDash * smoothstep(0.06, 0.28, sceneSnare);

    // Hats light fixed suspension pins, not the whole upper edge.
    float hatPins = 0.0;
    for (int index = 0; index < 5; ++index) {
        float fi = float(index);
        float center = -0.54 + fi * 0.27;
        vec2 pin = vec2(foldX(0.36, center, fi), 0.36);
        float gate = index == 1 || index == 3 ? 0.50 : 1.0;
        hatPins += (1.0 - smoothstep(0.006,
            0.016 + 0.005 * sceneHat, length(p - pin)))
            * sceneHat * gate;
    }

    float anticipation = line(p.x + 0.69, 0.004)
                       * line(p.y - 0.31, 0.020)
                       * beatAnticipation * clockConfidence;
    float downbeatBase = line(p.y + 0.365, 0.006)
                       * line(p.x, 0.23) * downbeat;
    float rearFold = line(abs(p.x - foldX(p.y, 0.68, 5.0)) - 0.065, 0.008)
                   * verticalWindow * section;

    float floor = smoothstep(-0.31, -0.39, p.y)
                * exp(-abs(p.y + 0.40) * 5.2);
    float horizon = line(p.y + 0.38, 0.005);
    float room = exp(-length(p * vec2(0.72, 1.10)) * 1.45);
    vec3 result = mix(primary, secondary, 0.48) * room * 0.0035;
    result += mix(primary, secondary, 0.38) * floor * 0.005
            + primary * horizon * 0.020
            + reflectionColor
            + sheetGlow
            + sheetColor
            + accent * internalLight * 0.028
            + accent * beatPacket * 0.24
            + accent * beatTail * 0.080
            + primary * kickPools * 0.19
            + mix(accent, vec3(1.0), 0.30) * snarePlane * 0.29
            + accent * hatPins * 0.32
            + secondary * anticipation * 0.11
            + accent * downbeatBase * 0.14
            + secondary * rearFold * 0.12;
    result *= 1.0 - clamp(snareCut * sheets * 0.46, 0.0, 0.58);
    result *= 1.0 - 0.62 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
