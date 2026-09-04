#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(0.0001, dot(ab, ab)), 0.0, 1.0);
    return length(p - a - ab * position);
}

float curveY(float x, float lane) {
    return lane * 0.070
         + 0.118 * sin(x * 4.5 + lane * 1.30)
         + 0.022 * sin(x * 10.0 - lane * 0.72);
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    vec2 q = rotate2d(-0.16) * (p - vec2(-0.02, -0.01));
    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.66, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;

    float horizontalWindow = smoothstep(-0.82, -0.68, q.x)
                           * smoothstep(0.82, 0.68, q.x);
    float body = 0.0;
    float beadHalo = 0.0;
    vec3 wovenColor = vec3(0.0);
    vec3 primary = mix(palettePrimary(5.72), vec3(0.02, 0.70, 1.0), 0.20);
    vec3 secondary = mix(paletteSecondary(5.72), vec3(0.92, 0.08, 0.82), 0.18);
    vec3 accent = mix(paletteAccent(5.72), vec3(1.0, 0.72, 0.16), 0.12);

    // Five fixed warp threads form the subject. Audio changes the material of
    // individual lanes, never the position or scale of the complete weave.
    for (int index = 0; index < 5; ++index) {
        float fi = float(index) - 2.0;
        float localY = curveY(q.x, fi);
        float distanceToThread = abs(q.y - localY);
        float depth = 0.5 + 0.5 * sin(q.x * 8.0 + fi * 1.72);
        float laneSpectrum = 1.0 - exp(
            -0.20 * spectrumLevel[index * 6 + 2]);
        float baseThread = line(distanceToThread,
            0.0022 + 0.0008 * depth) * horizontalWindow;
        float spacing = 0.055 + 0.0035 * float(index);
        float beadCell = floor((q.x + 0.84) / spacing);
        float beadX = -0.84 + (beadCell + 0.5) * spacing;
        vec2 beadPoint = vec2(beadX, curveY(beadX, fi));
        float distanceToBead = length(q - beadPoint);
        float bead = (1.0 - smoothstep(
            0.008 + 0.002 * depth, 0.014 + 0.004 * depth,
            distanceToBead)) * horizontalWindow;
        float halo = (1.0 - smoothstep(0.014, 0.032, distanceToBead))
                   * horizontalWindow;
        float over = smoothstep(0.38, 0.72, depth);
        vec3 laneColor = mix(primary, secondary,
            0.18 + 0.16 * float(index) + 0.16 * tonalMotion);
        wovenColor += laneColor * baseThread
                    * (0.022 + 0.018 * over + 0.012 * laneSpectrum);
        wovenColor += mix(laneColor, accent, 0.10 + 0.10 * depth) * bead
                    * (0.25 + 0.23 * over + 0.10 * laneSpectrum);
        beadHalo += halo * (0.25 + 0.30 * over);
        body = max(body, max(baseThread, bead));
    }

    // Short weft stitches cross only the width of the fabric. Their alternating
    // depth is fixed, so the pattern reads as a textile instead of a grid.
    float crossStitches = 0.0;
    float crossShadow = 0.0;
    float crossKnots = 0.0;
    for (int index = 0; index < 9; ++index) {
        float fi = float(index);
        float x = -0.68 + fi * 0.17;
        float lane = mod(fi, 4.0) - 1.5;
        float tilt = 0.012 * sin(fi * 1.91);
        vec2 a = vec2(x - tilt, curveY(x, lane - 0.48));
        vec2 b = vec2(x + tilt, curveY(x, lane + 0.48));
        float stitch = line(segmentDistance(q, a, b), 0.0032)
                     * (1.0 - smoothstep(0.018, 0.055, abs(q.x - x)));
        float over = mod(fi, 2.0) < 0.5 ? 1.0 : 0.48;
        crossStitches += stitch * over;
        crossShadow += stitch * (1.0 - over);
        vec2 knot = vec2(x, curveY(x, lane));
        crossKnots += (1.0 - smoothstep(0.006, 0.012,
            length(q - knot))) * (0.55 + 0.45 * over);
    }

    // Beat timing is one shuttle traveling through the established weave.
    float shuttleX = mix(-0.68, 0.68, beatPhase);
    vec2 shuttlePoint = vec2(shuttleX, curveY(shuttleX, 0.0));
    float shuttle = 1.0 - smoothstep(0.010, 0.023,
        length(q - shuttlePoint));
    shuttle *= clockConfidence * (0.28 + 0.72 * beatPulse);
    float shuttleTail = line(segmentDistance(q,
        shuttlePoint - vec2(0.060, 0.0), shuttlePoint), 0.003)
        * beatPulse * clockConfidence;

    // Kicks tighten two low knots, confined to their existing crossings.
    float kickKnots = 0.0;
    for (int index = 0; index < 2; ++index) {
        float x = index == 0 ? -0.43 : 0.29;
        vec2 knot = vec2(x, curveY(x, -1.45));
        float ringDistance = abs(length(q - knot)
            - (0.018 + 0.012 * sceneKick));
        kickKnots += line(ringDistance, 0.005) * sceneKick;
    }

    // A snare adds one brief diagonal cross-stitch through the middle lanes.
    vec2 snareA = vec2(-0.18, curveY(-0.18, -2.2));
    vec2 snareB = vec2(0.15, curveY(0.15, 2.2));
    float snareStitch = line(segmentDistance(q, snareA, snareB), 0.0025)
                      * sceneSnare * 0.20;
    for (int index = 0; index < 6; ++index) {
        vec2 stitchPoint = mix(snareA, snareB,
            (float(index) + 0.5) / 6.0);
        snareStitch += (1.0 - smoothstep(0.007,
            0.015 + 0.003 * sceneSnare, length(q - stitchPoint)))
            * sceneSnare;
    }
    float snareCut = 1.0 - smoothstep(0.003,
        0.008 + 0.010 * sceneSnare,
        segmentDistance(q, snareA, snareB));
    snareCut *= smoothstep(0.06, 0.26, sceneSnare);

    // Hats light a few beads on the upper lane. They do not shimmer across
    // the entire cloth.
    float hatBeads = 0.0;
    for (int index = 0; index < 6; ++index) {
        float fi = float(index);
        float x = -0.57 + fi * 0.22;
        vec2 point = vec2(x, curveY(x, 2.0));
        float gate = 0.45 + 0.55 * sin(fi * 2.18 + 0.6);
        hatBeads += (1.0 - smoothstep(0.006,
            0.017 + 0.006 * sceneHat, length(q - point)))
            * sceneHat * max(0.0, gate);
    }

    float anticipation = line(q.x + 0.73, 0.004)
                       * line(q.y - curveY(-0.73, 0.0), 0.020)
                       * beatAnticipation * clockConfidence;
    float downbeatKnot = line(length(q - vec2(0.0, curveY(0.0, 0.0)))
                              - 0.036, 0.005) * downbeat;
    float sectionThread = line(abs(q.y - curveY(q.x, 2.75)), 0.004)
                        * horizontalWindow * section;

    float clothTop = max(curveY(q.x, 2.25), curveY(q.x, -2.25));
    float clothBottom = min(curveY(q.x, 2.25), curveY(q.x, -2.25));
    float cloth = smoothstep(clothBottom - 0.025, clothBottom + 0.020, q.y)
                * smoothstep(clothTop + 0.025, clothTop - 0.020, q.y)
                * horizontalWindow;
    float clothGrain = 0.5 + 0.5
        * sin(q.x * 44.0 + q.y * 13.0)
        * sin(q.y * 38.0 - q.x * 9.0);
    float field = exp(-length(q * vec2(0.82, 1.70)) * 2.3);
    vec3 result = mix(primary, secondary, 0.54) * field * 0.0035;
    result += mix(primary, secondary, 0.50 + 0.22 * q.x)
              * cloth * (0.0035 + 0.0030 * clothGrain)
            + wovenColor
            + mix(primary, secondary, 0.50) * beadHalo * 0.017
            + mix(primary, secondary, 0.52) * crossStitches * 0.065
            + primary * crossShadow * 0.015
            + accent * crossKnots * 0.095
            + accent * shuttle * 0.24
            + accent * shuttleTail * 0.075
            + primary * kickKnots * 0.27
            + mix(accent, vec3(1.0), 0.28) * snareStitch * 0.29
            + accent * hatBeads * 0.40
            + secondary * anticipation * 0.11
            + accent * downbeatKnot * 0.13
            + secondary * sectionThread * 0.12;
    result *= 1.0 - clamp(snareCut * 0.32 * body, 0.0, 0.32);
    result *= 1.0 - 0.62 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
