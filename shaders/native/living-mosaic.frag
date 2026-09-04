#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

vec2 cellPoint(vec2 cell) {
    vec2 random = hash22(cell + vec2(17.2, 41.7));
    return cell + 0.16 + random * 0.68;
}

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(0.0001, dot(ab, ab)), 0.0, 1.0);
    return length(p - a - ab * position);
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    vec2 fieldP = (p + vec2(0.86, 0.50)) * vec2(4.25, 4.8);
    vec2 baseCell = floor(fieldP);
    float nearest = 100.0;
    float secondNearest = 100.0;
    vec2 nearestPoint = vec2(0.0);
    vec2 nearestCell = vec2(0.0);
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 cell = baseCell + vec2(float(x), float(y));
            vec2 point = cellPoint(cell);
            float distanceToPoint = length(fieldP - point);
            if (distanceToPoint < nearest) {
                secondNearest = nearest;
                nearest = distanceToPoint;
                nearestPoint = point;
                nearestCell = cell;
            } else if (distanceToPoint < secondNearest) {
                secondNearest = distanceToPoint;
            }
        }
    }

    float frameWindow = smoothstep(-0.88, -0.78, p.x)
                      * smoothstep(0.88, 0.78, p.x)
                      * smoothstep(-0.50, -0.42, p.y)
                      * smoothstep(0.50, 0.42, p.y);
    float edgeDistance = secondNearest - nearest;
    float seam = 1.0 - smoothstep(0.018, 0.052, edgeDistance);
    float innerSeam = 1.0 - smoothstep(0.050, 0.145, edgeDistance);
    vec2 local = fieldP - nearestPoint;
    float cellIdentity = hash12(nearestCell + vec2(3.1, 8.7));
    float cellIdentityB = hash12(nearestCell + vec2(21.4, 5.2));

    // Every cell and seam is fixed. Audio changes selected material accents,
    // never the field coordinates or the scale of the composition.
    float relief = 1.0 - smoothstep(0.18, 0.80, nearest);
    vec2 reliefGradient = normalize(local + vec2(0.0001));
    float grazing = max(0.0, dot(reliefGradient,
        normalize(vec2(-0.62, 0.78))));
    float membrane = smoothstep(0.08, 0.22, edgeDistance)
                   * (0.72 + 0.28 * relief);
    float fineGrain = 0.5 + 0.5 * sin(
        dot(fieldP, vec2(7.3, 11.1)) + cellIdentity * 18.0);

    vec3 primary = mix(palettePrimary(5.75), vec3(0.04, 0.64, 0.88), 0.16);
    vec3 secondary = mix(paletteSecondary(5.75), vec3(0.94, 0.12, 0.66), 0.14);
    vec3 accent = mix(paletteAccent(5.75), vec3(1.0, 0.76, 0.18), 0.10);
    vec3 cellColor = mix(primary, secondary,
        clamp(0.10 + cellIdentity * 0.78, 0.0, 1.0));
    cellColor = mix(cellColor, accent,
        smoothstep(0.72, 0.96, cellIdentityB) * 0.34);

    int bandIndex = int(mod(abs(nearestCell.x * 2.0 + nearestCell.y * 3.0), 6.0));
    float level = 1.0 - exp(-0.18 * bandLevel[bandIndex]);
    float spectralInlay = smoothstep(0.79, 0.94, cellIdentityB)
                        * relief * level;

    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.66, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;

    // Beat timing selects one narrow column of cells at a time.
    float beatColumn = mix(-0.72, 0.72, beatPhase);
    float beatSelection = 1.0 - smoothstep(0.05, 0.13,
        abs((nearestPoint.x / 4.25 - 0.86) - beatColumn));
    float beatEdge = seam * beatSelection * clockConfidence
                   * (0.22 + 0.78 * beatPulse);

    // Kicks resonate in two fixed lower cells only.
    vec2 kickA = vec2(-0.50, -0.27);
    vec2 kickB = vec2(0.42, -0.31);
    float kickCells = (1.0 - smoothstep(0.16, 0.27, length(p - kickA)))
                    + (1.0 - smoothstep(0.13, 0.23, length(p - kickB)));
    kickCells *= sceneKick * membrane;
    // Snare draws a short broken seam through only the middle-right cells.
    vec2 snareA = vec2(-0.10, -0.22);
    vec2 snareB = vec2(0.43, 0.20);
    float alongSnare = dot(p - snareA, normalize(snareB - snareA));
    float snareDash = step(0.52, fract(alongSnare * 15.0));
    float snareLine = line(segmentDistance(p, snareA, snareB), 0.004)
                    * snareDash * sceneSnare * frameWindow;
    float snareCut = (1.0 - smoothstep(0.003,
        0.008 + 0.007 * sceneSnare,
        segmentDistance(p, snareA, snareB)))
        * snareDash * smoothstep(0.06, 0.27, sceneSnare) * frameWindow;

    // Hats flash a few nuclei in small upper cells.
    float nucleus = 1.0 - smoothstep(0.030,
        0.060 + 0.010 * sceneHat, nearest);
    float upperCells = smoothstep(0.08, 0.28, p.y);
    float hatSelection = smoothstep(0.72, 0.90, cellIdentity)
                       * smoothstep(0.54, 0.78, cellIdentityB);
    float hatNuclei = nucleus * upperCells * hatSelection * sceneHat;

    float anticipation = line(p.x + 0.78, 0.004)
                       * line(p.y - 0.31, 0.022)
                       * beatAnticipation * clockConfidence;
    float downbeatCell = line(length((p - vec2(-0.31, 0.11))
                              / vec2(1.0, 0.82)) - 0.092, 0.007)
                       * downbeat * membrane;
    float dormantSeams = seam * smoothstep(0.84, 0.96, cellIdentityB)
                       * section;

    float vignette = exp(-length(p * vec2(0.70, 1.05)) * 1.35);
    float restingNuclei = nucleus * smoothstep(0.82, 0.95, cellIdentityB)
                         * (0.35 + 0.65 * relief);
    vec3 result = mix(primary, secondary, 0.46) * vignette * 0.005;
    result += cellColor * membrane * frameWindow
              * (0.034 + 0.055 * grazing + 0.010 * fineGrain
                 + 0.018 * relief)
            + mix(cellColor, accent, 0.30) * innerSeam * frameWindow * 0.030
            + mix(primary, accent, 0.52) * seam * frameWindow * 0.030
            + accent * restingNuclei * frameWindow * 0.055
            + accent * spectralInlay * frameWindow * 0.026
            + accent * beatEdge * frameWindow * 0.145
            + mix(primary, accent, 0.28) * kickCells * 0.135
            + mix(accent, vec3(1.0), 0.28) * snareLine * 0.28
            + accent * hatNuclei * frameWindow * 0.34
            + secondary * anticipation * 0.11
            + accent * downbeatCell * frameWindow * 0.15
            + secondary * dormantSeams * frameWindow * 0.095;
    result *= 1.0 - seam * frameWindow * 0.42;
    result += mix(primary, accent, 0.58) * seam * frameWindow * 0.064;
    result *= 1.0 - clamp(snareCut * 0.52, 0.0, 0.62);
    result *= 1.35;
    result *= 1.0 - 0.62 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
