#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float t = clamp(dot(p - a, ab) / max(dot(ab, ab), 0.0001), 0.0, 1.0);
    return length(p - a - ab * t);
}

vec2 starPosition(int index) {
    if (index == 0) return vec2(-0.73, -0.20);
    if (index == 1) return vec2(-0.59,  0.035);
    if (index == 2) return vec2(-0.44, -0.085);
    if (index == 3) return vec2(-0.30,  0.205);
    if (index == 4) return vec2(-0.16,  0.045);
    if (index == 5) return vec2(-0.035, -0.175);
    if (index == 6) return vec2( 0.105,  0.105);
    if (index == 7) return vec2( 0.235,  0.285);
    if (index == 8) return vec2( 0.355,  0.015);
    if (index == 9) return vec2( 0.485, -0.155);
    if (index == 10) return vec2(0.625,  0.125);
    if (index == 11) return vec2(0.755, -0.025);
    if (index == 12) return vec2(-0.515, -0.325);
    if (index == 13) return vec2(-0.105,  0.335);
    if (index == 14) return vec2(0.295, -0.315);
    return vec2(0.575, 0.335);
}

float curvedLinkDistance(vec2 p, vec2 a, vec2 b, float bend) {
    vec2 direction = b - a;
    vec2 normal = normalize(vec2(-direction.y, direction.x));
    vec2 control = (a + b) * 0.5 + normal * bend;
    float closest = 10.0;
    vec2 previous = a;
    for (int stepIndex = 1; stepIndex <= 6; ++stepIndex) {
        float t = float(stepIndex) / 6.0;
        vec2 point = mix(mix(a, control, t), mix(control, b, t), t);
        closest = min(closest, segmentDistance(p, previous, point));
        previous = point;
    }
    return closest;
}

float starDisc(vec2 p, vec2 center, float radius) {
    return 1.0 - smoothstep(radius, radius * 2.25, length(p - center));
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

    vec3 primary = mix(palettePrimary(3.28), vec3(0.96, 0.10, 0.56), 0.24);
    vec3 secondary = mix(paletteSecondary(3.28), vec3(0.08, 0.72, 1.0), 0.18);
    vec3 accent = mix(paletteAccent(3.28), vec3(1.0, 0.76, 0.30), 0.12);

    float mainLinks = 0.0;
    float linkHalo = 0.0;
    float snareBridge = 0.0;
    float harmonicThreads = 0.0;
    for (int index = 0; index < 11; ++index) {
        vec2 a = starPosition(index);
        vec2 b = starPosition(index + 1);
        float bend = 0.018 * sin(float(index) * 1.83 + 0.4);
        float distanceToLink = curvedLinkDistance(p, a, b, bend);
        float thread = line(distanceToLink, 0.0016);
        mainLinks = max(mainLinks, thread);
        linkHalo += line(distanceToLink, 0.0065) * 0.18;
        float midRole = 1.0 - step(4.0, abs(float(index) - 5.0));
        harmonicThreads += line(distanceToLink - 0.005, 0.0014)
                         * midRole * midSustain;
        float snareRole = step(3.0, float(index))
                        * (1.0 - step(8.0, float(index)));
        float dash = smoothstep(0.25, 0.85,
            sin(dot(p - a, normalize(b - a)) * 84.0
                - sceneSnare * 2.4 + float(index)));
        snareBridge += thread * dash * snareRole * sceneSnare;
    }

    float branchLinks = 0.0;
    for (int index = 0; index < 4; ++index) {
        int satellite = index + 12;
        int anchorIndex = index == 0 ? 2 : (index == 1 ? 4 : (index == 2 ? 8 : 10));
        float distanceToBranch = curvedLinkDistance(
            p, starPosition(anchorIndex), starPosition(satellite),
            (index < 2 ? -0.020 : 0.020));
        branchLinks = max(branchLinks, line(distanceToBranch, 0.0013));
        linkHalo += line(distanceToBranch, 0.0055) * 0.12;
    }

    float stars = 0.0;
    float starHalos = 0.0;
    float kickNodes = 0.0;
    float hatNodes = 0.0;
    float highSatellites = 0.0;
    for (int index = 0; index < 16; ++index) {
        vec2 center = starPosition(index);
        float fi = float(index);
        float baseRadius = 0.0052 + 0.0018 * (0.5 + 0.5 * sin(fi * 2.17));
        float star = starDisc(p, center, baseRadius);
        float halo = starDisc(p, center, baseRadius * 3.8);
        stars += star;
        starHalos += halo * (0.12 + 0.05 * sin(fi * 1.31));

        float kickRole = (index == 0 || index == 5 || index == 9) ? 1.0 : 0.0;
        float kickRing = line(length(p - center)
            - (0.014 + 0.017 * sceneKick), 0.0033);
        kickNodes += kickRing * kickRole * sceneKick;

        float hatRole = index >= 10 ? 1.0 : 0.0;
        float hatRadius = baseRadius + 0.010 * sceneHat
                        * (0.55 + 0.45 * sin(fi * 2.31 + 0.7));
        hatNodes += starDisc(p, center, hatRadius) * hatRole * sceneHat;
        highSatellites += halo * hatRole * highSustain;
    }

    float route = clamp(beatPhase, 0.0, 0.999) * 11.0;
    int routeIndex = int(floor(route));
    float routeT = fract(route);
    vec2 routeA = starPosition(routeIndex);
    vec2 routeB = starPosition(min(routeIndex + 1, 11));
    vec2 travelerPosition = mix(routeA, routeB, routeT);
    float traveler = starDisc(p, travelerPosition,
        0.007 + 0.006 * beatPulse) * clockConfidence
        * (0.08 * energySlow + 0.92 * beatPulse);
    float anticipationMarker = line(
        length(p - starPosition(0)) - (0.018 + 0.012 * beatAnticipation),
        0.003) * beatAnticipation * clockConfidence;
    float downbeatBeacon = line(
        length(p - starPosition(6)) - 0.024, 0.004)
        * downbeat * clockConfidence;

    float chordWeight = clamp(chroma[0] + 0.72 * chroma[4]
                              + 0.88 * chroma[7], 0.0, 1.0);
    float chordArc = line(curvedLinkDistance(
        p, starPosition(3), starPosition(7), -0.055) - 0.010,
        0.0020) * chordWeight;
    float sectionThread = line(curvedLinkDistance(
        p, starPosition(12), starPosition(15), 0.12), 0.0024) * section;
    float lowAnchor = line(length(p - starPosition(12))
                           - (0.026 + 0.018 * lowSustain), 0.004)
                    * lowSustain;

    float dust = 0.0;
    for (int index = 0; index < 9; ++index) {
        float fi = float(index);
        vec2 dustPoint = vec2(
            -0.74 + fract(fi * 0.6180339) * 1.48,
            -0.38 + fract(fi * 0.381966 + 0.23) * 0.76);
        dust += starDisc(p, dustPoint, 0.0018) * (0.45 + 0.55 * harmonic);
    }

    float diagonalVeil = exp(-pow(p.y - 0.11 * sin(p.x * 3.3) + 0.01,
                                 2.0) * 22.0)
                       * smoothstep(0.92, 0.12, abs(p.x));
    vec3 result = mix(primary, secondary, 0.46) * diagonalVeil
                * (0.035 + 0.0060 * harmonic)
                + primary * mainLinks * (0.28 + 0.070 * midSustain)
                + secondary * branchLinks * 0.20
                + mix(primary, secondary, 0.5) * linkHalo * 0.050
                + mix(primary, vec3(1.0), 0.24) * stars * 0.68
                + primary * starHalos * 0.075
                + secondary * harmonicThreads * 0.16
                + mix(secondary, vec3(1.0), 0.26) * snareBridge * 0.42
                + accent * kickNodes * 0.50
                + mix(accent, vec3(1.0), 0.34) * hatNodes * 0.58
                + accent * highSatellites * 0.060
                + accent * traveler * 0.58
                + secondary * anticipationMarker * 0.28
                + accent * downbeatBeacon * 0.30
                + mix(primary, secondary, 0.62) * chordArc * 0.14
                + accent * sectionThread * 0.26
                + primary * lowAnchor * 0.18
                + mix(primary, secondary, 0.52) * dust * 0.085;
    result *= 1.0 - 0.64 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
