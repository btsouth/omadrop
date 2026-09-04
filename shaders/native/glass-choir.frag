#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(dot(ab, ab), 0.0001),
                           0.0, 1.0);
    return length(p - a - ab * position);
}

float curvedDistance(vec2 p, vec2 a, vec2 b, float bend) {
    vec2 direction = b - a;
    vec2 normal = normalize(vec2(-direction.y, direction.x));
    vec2 control = (a + b) * 0.5 + normal * bend;
    float closest = 10.0;
    vec2 previous = a;
    for (int index = 1; index <= 7; ++index) {
        float t = float(index) / 7.0;
        vec2 point = mix(mix(a, control, t), mix(control, b, t), t);
        closest = min(closest, segmentDistance(p, previous, point));
        previous = point;
    }
    return closest;
}

float disc(vec2 p, vec2 center, float radius) {
    return 1.0 - smoothstep(radius, radius * 2.15, length(p - center));
}

float voiceX(int index) {
    if (index == 0) return -0.59;
    if (index == 1) return -0.42;
    if (index == 2) return -0.225;
    if (index == 3) return -0.010;
    if (index == 4) return 0.225;
    if (index == 5) return 0.445;
    return 0.62;
}

float voiceY(int index) {
    if (index == 0) return -0.085;
    if (index == 1) return 0.055;
    if (index == 2) return -0.025;
    if (index == 3) return 0.105;
    if (index == 4) return -0.045;
    if (index == 5) return 0.045;
    return -0.095;
}

float voiceHeight(int index) {
    if (index == 0) return 0.225;
    if (index == 1) return 0.305;
    if (index == 2) return 0.255;
    if (index == 3) return 0.345;
    if (index == 4) return 0.275;
    if (index == 5) return 0.300;
    return 0.215;
}

float voiceWidth(int index) {
    if (index == 0) return 0.060;
    if (index == 1) return 0.074;
    if (index == 2) return 0.064;
    if (index == 3) return 0.080;
    if (index == 4) return 0.069;
    if (index == 5) return 0.073;
    return 0.058;
}

float voiceTilt(int index) {
    if (index == 0) return -0.17;
    if (index == 1) return 0.09;
    if (index == 2) return -0.08;
    if (index == 3) return 0.035;
    if (index == 4) return 0.15;
    if (index == 5) return -0.11;
    return 0.18;
}

vec2 voiceCenter(int index) {
    return vec2(voiceX(index), voiceY(index));
}

// A narrow, asymmetric crystal: broad at its shoulder and pointed at both
// ends. The distance is deliberately stable; music changes what is visible
// inside each voice rather than making the complete silhouette bounce.
float shardDistance(vec2 q, float halfWidth, float halfHeight) {
    float normalizedY = clamp(q.y / max(halfHeight, 0.001), -1.0, 1.0);
    float upperWidth = mix(halfWidth, halfWidth * 0.16,
        smoothstep(0.03, 1.0, normalizedY));
    float lowerWidth = mix(halfWidth * 0.24, halfWidth,
        smoothstep(-1.0, 0.03, normalizedY));
    float width = normalizedY >= 0.03 ? upperWidth : lowerWidth;
    float skewedX = q.x + q.y * 0.075;
    return max(abs(q.y) - halfHeight, abs(skewedX) - width);
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.68, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;

    float lowSustain = 1.0 - exp(-0.34 * (bandLevel[0] + bandLevel[1]));
    float midSustain = 1.0 - exp(-0.34 * (bandLevel[2] + bandLevel[3]));
    float highSustain = 1.0 - exp(-0.34 * (bandLevel[4] + bandLevel[5]));

    vec3 primary = mix(palettePrimary(5.70), vec3(0.10, 0.72, 0.92), 0.12);
    vec3 secondary = mix(paletteSecondary(5.70), vec3(0.78, 0.24, 0.92), 0.14);
    vec3 accent = mix(paletteAccent(5.70), vec3(1.0, 0.78, 0.32), 0.10);

    float bodies = 0.0;
    float facetLines = 0.0;
    float caustics = 0.0;
    float lowResonance = 0.0;
    float snareFractures = 0.0;
    float fractureCuts = 0.0;
    float hatGlints = 0.0;
    float suspension = 0.0;
    float reflectedEdges = 0.0;
    float bevels = 0.0;
    vec3 glassLight = vec3(0.0);

    for (int index = 0; index < 7; ++index) {
        float fi = float(index);
        vec2 center = voiceCenter(index);
        float halfHeight = voiceHeight(index);
        float halfWidth = voiceWidth(index);
        float tilt = voiceTilt(index);
        vec2 q = rotate2d(tilt) * (p - center);
        float shard = shardDistance(q, halfWidth, halfHeight);
        float body = smoothstep(0.010, -0.012, shard);
        float edge = line(shard, 0.0032);
        float bevel = line(shard + 0.013, 0.0026) * body;
        bodies = max(bodies, body);
        bevels = max(bevels, bevel);

        float shoulderY = halfHeight * 0.03;
        vec2 bottom = vec2(-halfHeight * 0.075, -halfHeight * 0.91);
        vec2 leftShoulder = vec2(-halfWidth * 0.86, shoulderY);
        vec2 rightShoulder = vec2(halfWidth * 0.86, shoulderY);
        vec2 top = vec2(-halfHeight * 0.075, halfHeight * 0.92);
        float facets = line(segmentDistance(q, bottom, leftShoulder), 0.0018)
                     + line(segmentDistance(q, bottom, rightShoulder), 0.0018)
                     + line(segmentDistance(q, leftShoulder, top), 0.0015);
        facets *= body;
        facetLines += facets;

        float voiceLevel = 1.0 - exp(-0.24 * spectrumLevel[index * 4 + 2]);
        float harmonicOffset = 0.34 * harmonicChange
                             + 0.18 * tonalMotion
                             + 0.12 * voiceLevel;
        float causticA = line(sin(q.y * (23.0 + fi * 1.7)
                                  + q.x * (12.0 - fi * 0.6)
                                  + harmonicOffset * tau), 0.12);
        float causticB = line(sin(q.x * (30.0 - fi)
                                  - q.y * (8.0 + fi)
                                  - harmonicOffset * tau * 0.73), 0.10);
        float caustic = max(causticA, causticB * 0.68)
                      * body * (0.18 + 0.82 * voiceLevel);
        caustics += caustic;

        float colorPosition = clamp(0.08 + fi * 0.14
            + 0.10 * tonalMotion, 0.0, 1.0);
        vec3 voiceColor = mix(primary, secondary, colorPosition);
        glassLight += voiceColor * body
                    * (0.014 + 0.013 * harmonic + 0.010 * voiceLevel)
                    + mix(voiceColor, accent, 0.18) * edge * 0.13
                    + voiceColor * bevel * 0.045;

        float kickRole = (index == 0 || index == 3 || index == 6) ? 1.0 : 0.0;
        vec2 resonanceCenter = vec2(-halfHeight * 0.075,
                                    -halfHeight * 0.56);
        float resonance = line(length(q - resonanceCenter)
            - (0.015 + 0.020 * sceneKick), 0.0038) * body;
        lowResonance += resonance * sceneKick * kickRole;

        float snareRole = (index == 2 || index == 4) ? 1.0 : 0.0;
        vec2 fractureA = vec2(-halfWidth * 0.82, -halfHeight * 0.26);
        vec2 fractureB = vec2(halfWidth * 0.68, halfHeight * 0.32);
        float fractureDistance = segmentDistance(q, fractureA, fractureB);
        snareFractures += line(fractureDistance,
            0.0025 + 0.0020 * sceneSnare) * body * sceneSnare * snareRole;
        fractureCuts += (1.0 - smoothstep(0.0015,
            0.004 + 0.008 * sceneSnare, fractureDistance))
            * body * smoothstep(0.08, 0.30, sceneSnare) * snareRole;

        float hatRole = mod(fi, 2.0) < 0.5 ? 1.0 : 0.42;
        vec2 worldTop = center + rotate2d(-tilt) * top;
        hatGlints += disc(p, worldTop,
            0.004 + 0.007 * sceneHat) * sceneHat * hatRole;

        vec2 hangingTop = center + rotate2d(-tilt)
                        * vec2(-halfHeight * 0.075, halfHeight);
        float hangingDistance = segmentDistance(
            p, hangingTop, hangingTop + vec2(0.0, 0.10 + 0.02 * mod(fi, 3.0)));
        suspension = max(suspension, line(hangingDistance, 0.0014));

        vec2 reflectedP = vec2(p.x, -0.50 - p.y);
        vec2 reflectedQ = rotate2d(-tilt) * (reflectedP - center);
        float reflectedShard = shardDistance(
            reflectedQ, halfWidth, halfHeight);
        reflectedEdges += line(reflectedShard, 0.0030)
                        * smoothstep(-0.46, -0.28, p.y) * 0.10;
    }

    // A single conductor traces the choir from left to right once per beat.
    float route = clamp(beatPhase, 0.0, 0.999) * 6.0;
    int routeIndex = int(floor(route));
    float routeT = fract(route);
    vec2 routeA = voiceCenter(routeIndex)
                + vec2(0.0, voiceHeight(routeIndex) * 0.20);
    vec2 routeB = voiceCenter(min(routeIndex + 1, 6))
                + vec2(0.0, voiceHeight(min(routeIndex + 1, 6)) * 0.20);
    vec2 conductorPosition = mix(routeA, routeB, routeT)
                           + vec2(0.0, 0.035 * sin(routeT * 3.14159265));
    float conductor = disc(p, conductorPosition,
        0.006 + 0.007 * beatPulse) * clockConfidence
        * (0.12 * energySlow + 0.88 * beatPulse);

    float anticipation = line(length(p - voiceCenter(0))
        - (0.016 + 0.013 * beatAnticipation), 0.0032)
        * beatAnticipation * clockConfidence;
    float downbeatBell = line(length(p - voiceCenter(3)) - 0.043, 0.0042)
                       * downbeat * clockConfidence;
    float sectionArc = line(curvedDistance(
        p, voiceCenter(0), voiceCenter(6), -0.19), 0.0028) * section;

    float lowBody = bodies * (1.0 - smoothstep(-0.34, -0.02, p.y))
                  * lowSustain;
    float midFacets = facetLines * midSustain;
    float highBevels = bevels * highSustain;
    float harmonicMist = exp(-pow((p.y + 0.04) / 0.34, 2.0))
                       * smoothstep(0.80, 0.56, abs(p.x))
                       * harmonic * 0.012;

    vec3 result = primary * harmonicMist
                + glassLight
                + mix(primary, secondary, 0.35) * bodies * 0.006
                + primary * facetLines * 0.055
                + mix(primary, accent, 0.30) * caustics * 0.060
                + primary * lowBody * 0.030
                + secondary * midFacets * 0.065
                + accent * highBevels * 0.045
                + primary * lowResonance * 0.36
                + mix(accent, vec3(1.0), 0.32) * snareFractures * 0.30
                + mix(accent, vec3(1.0), 0.52) * hatGlints * 0.22
                + accent * conductor * 0.34
                + secondary * anticipation * 0.19
                + accent * downbeatBell * 0.26
                + mix(primary, secondary, 0.48) * sectionArc * 0.16
                + secondary * suspension * 0.052
                + primary * reflectedEdges * 0.075;
    result *= 1.0 - clamp(fractureCuts, 0.0, 1.0) * 0.62;
    result *= 1.0 - 0.60 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
