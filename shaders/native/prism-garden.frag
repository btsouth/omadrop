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
    for (int index = 1; index <= 6; ++index) {
        float t = float(index) / 6.0;
        vec2 point = mix(mix(a, control, t), mix(control, b, t), t);
        closest = min(closest, segmentDistance(p, previous, point));
        previous = point;
    }
    return closest;
}

float disc(vec2 p, vec2 center, float radius) {
    return 1.0 - smoothstep(radius, radius * 2.2, length(p - center));
}

float plantX(int index) {
    if (index == 0) return -0.67;
    if (index == 1) return -0.48;
    if (index == 2) return -0.28;
    if (index == 3) return -0.055;
    if (index == 4) return 0.19;
    if (index == 5) return 0.41;
    return 0.64;
}

float plantHeight(int index) {
    if (index == 0) return 0.34;
    if (index == 1) return 0.57;
    if (index == 2) return 0.43;
    if (index == 3) return 0.72;
    if (index == 4) return 0.48;
    if (index == 5) return 0.61;
    return 0.37;
}

float plantLean(int index) {
    if (index == 0) return -0.035;
    if (index == 1) return 0.040;
    if (index == 2) return -0.022;
    if (index == 3) return 0.018;
    if (index == 4) return 0.045;
    if (index == 5) return -0.038;
    return 0.025;
}

vec2 plantBase(int index) {
    return vec2(plantX(index), -0.36);
}

vec2 plantTip(int index) {
    return vec2(plantX(index) + plantLean(index),
                -0.36 + plantHeight(index));
}

float diamondDistance(vec2 p, vec2 center, vec2 radius, float rotation) {
    vec2 q = rotate2d(rotation) * (p - center);
    return (abs(q.x) / radius.x + abs(q.y) / radius.y - 1.0)
         * min(radius.x, radius.y);
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

    vec3 primary = mix(palettePrimary(1.34), vec3(0.12, 0.72, 0.94), 0.16);
    vec3 secondary = mix(paletteSecondary(1.34), vec3(0.90, 0.18, 0.72), 0.18);
    vec3 accent = mix(paletteAccent(1.34), vec3(1.0, 0.76, 0.20), 0.12);

    float stems = 0.0;
    float stemHalos = 0.0;
    float leaves = 0.0;
    float leafHalos = 0.0;
    float leafBodies = 0.0;
    float leafVeins = 0.0;
    float crowns = 0.0;
    float crownOutlines = 0.0;
    float crownHalos = 0.0;
    float facets = 0.0;
    float snareFacets = 0.0;
    float hatDew = 0.0;
    vec3 plantColor = vec3(0.0);

    for (int index = 0; index < 7; ++index) {
        float fi = float(index);
        vec2 base = plantBase(index);
        vec2 tip = plantTip(index);
        float bend = 0.020 * sin(fi * 1.73 - 0.4);
        float distanceToStem = curvedDistance(p, base, tip, bend);
        float stem = line(distanceToStem, 0.0032 + 0.0005 * mod(fi, 2.0));
        float stemHalo = line(distanceToStem, 0.013);
        stems = max(stems, stem);
        stemHalos += stemHalo * 0.14;

        float leafT = 0.34 + 0.08 * mod(fi, 3.0);
        vec2 leafAnchor = mix(base, tip, leafT);
        float side = mod(fi, 2.0) < 0.5 ? -1.0 : 1.0;
        vec2 leafTip = leafAnchor + vec2(side * (0.095 + 0.010 * sin(fi)),
                                         0.040 + 0.018 * sin(fi * 2.0));
        float distanceToLeaf = curvedDistance(
            p, leafAnchor, leafTip, -side * 0.021);
        leaves = max(leaves, line(distanceToLeaf, 0.0030));
        leafHalos += line(distanceToLeaf, 0.011) * 0.12;

        vec2 leafDirection = normalize(leafTip - leafAnchor);
        vec2 leafCenter = mix(leafAnchor, leafTip, 0.76);
        float leafAngle = atan(leafDirection.y, leafDirection.x);
        float leafDistance = diamondDistance(
            p, leafCenter, vec2(0.040, 0.013), -leafAngle);
        leafBodies = max(leafBodies,
            smoothstep(0.006, -0.006, leafDistance));
        leafVeins = max(leafVeins,
            line(segmentDistance(p, leafAnchor, leafTip), 0.0018)
            * smoothstep(0.009, -0.004, leafDistance));

        // A smaller counter-leaf breaks the row-of-lamps silhouette without
        // making the plants sway or scale with every transient.
        float upperT = 0.57 + 0.045 * mod(fi + 1.0, 3.0);
        vec2 upperAnchor = mix(base, tip, upperT);
        vec2 upperTip = upperAnchor
                      + vec2(-side * (0.060 + 0.008 * cos(fi * 1.4)),
                             0.026 + 0.009 * cos(fi * 2.1));
        float upperDistance = curvedDistance(
            p, upperAnchor, upperTip, side * 0.014);
        leaves = max(leaves, line(upperDistance, 0.0026));
        leafHalos += line(upperDistance, 0.009) * 0.09;
        vec2 upperDirection = normalize(upperTip - upperAnchor);
        vec2 upperCenter = mix(upperAnchor, upperTip, 0.78);
        float upperAngle = atan(upperDirection.y, upperDirection.x);
        float upperLeafDistance = diamondDistance(
            p, upperCenter, vec2(0.029, 0.0095), -upperAngle);
        leafBodies = max(leafBodies,
            smoothstep(0.005, -0.005, upperLeafDistance));
        leafVeins = max(leafVeins,
            line(segmentDistance(p, upperAnchor, upperTip), 0.0015)
            * smoothstep(0.007, -0.003, upperLeafDistance));

        vec2 crownRadius = vec2(0.048 + 0.006 * mod(fi, 3.0),
                                0.072 + 0.008 * mod(fi + 1.0, 3.0));
        float rotation = -0.18 + 0.07 * fi + plantLean(index) * 2.0;
        float crownDistance = diamondDistance(
            p, tip, crownRadius, rotation);
        float crown = smoothstep(0.009, -0.012, crownDistance);
        float outline = line(crownDistance, 0.0042);
        vec2 crownAxis = rotate2d(-rotation) * vec2(1.0, 0.0);
        vec2 leftCenter = tip - crownAxis * crownRadius.x * 0.54
                              - vec2(0.0, crownRadius.y * 0.07);
        vec2 rightCenter = tip + crownAxis * crownRadius.x * 0.54
                               - vec2(0.0, crownRadius.y * 0.07);
        float leftDistance = diamondDistance(
            p, leftCenter, crownRadius * vec2(0.57, 0.72), rotation - 0.43);
        float rightDistance = diamondDistance(
            p, rightCenter, crownRadius * vec2(0.57, 0.72), rotation + 0.43);
        float leftPetal = smoothstep(0.007, -0.008, leftDistance);
        float rightPetal = smoothstep(0.007, -0.008, rightDistance);
        float petalOutline = max(line(leftDistance, 0.0032),
                                 line(rightDistance, 0.0032));
        crowns = max(crowns, max(crown, max(leftPetal, rightPetal)));
        crownOutlines = max(crownOutlines, max(outline, petalOutline));
        crownHalos += smoothstep(0.060, 0.0, abs(crownDistance)) * 0.10;

        vec2 facetA = tip + rotate2d(-rotation)
                    * vec2(-crownRadius.x * 0.72, 0.0);
        vec2 facetB = tip + rotate2d(-rotation)
                    * vec2(0.0, crownRadius.y * 0.72);
        vec2 facetC = tip + rotate2d(-rotation)
                    * vec2(crownRadius.x * 0.72, -crownRadius.y * 0.18);
        float facetLines = line(segmentDistance(p, facetA, facetB), 0.0022)
                         + line(segmentDistance(p, facetB, facetC), 0.0022);
        facetLines *= smoothstep(0.012, -0.006, crownDistance);
        facets += facetLines;

        float snareRole = mod(fi, 2.0);
        float snareDash = smoothstep(0.30, 0.82,
            sin(dot(p - facetA, normalize(facetC - facetA)) * 72.0 + fi));
        snareFacets += facetLines * snareDash * snareRole * sceneSnare;

        float dewRole = 0.38 + 0.62 * step(2.0, fi);
        vec2 dew = tip + vec2(0.0, crownRadius.y * 0.78);
        hatDew += disc(p, dew,
            0.004 + 0.010 * sceneHat * (0.55 + 0.45 * sin(fi * 2.2)))
            * dewRole * sceneHat;

        vec3 localColor = mix(primary, secondary,
            0.12 + 0.12 * fi + 0.10 * tonalMotion);
        vec3 petalColor = mix(localColor, accent, 0.18 + 0.05 * mod(fi, 3.0));
        plantColor += localColor * crown
                    * (0.035 + 0.013 * harmonic + 0.008 * sin(fi * 1.7))
                    + petalColor * (leftPetal + rightPetal) * 0.026
                    + mix(localColor, accent, 0.20)
                    * max(outline, petalOutline) * 0.17;
    }

    float rootNetwork = 0.0;
    float rootGlow = 0.0;
    float rootFilaments = 0.0;
    for (int index = 0; index < 6; ++index) {
        vec2 a = plantBase(index);
        vec2 b = plantBase(index + 1);
        float rootDistance = curvedDistance(
            p, a, b, 0.012 * sin(float(index) * 1.8));
        rootNetwork = max(rootNetwork, line(rootDistance, 0.0028));
        rootGlow += line(rootDistance, 0.012) * 0.12;
    }
    for (int index = 0; index < 7; ++index) {
        float fi = float(index);
        vec2 base = plantBase(index);
        float rootSide = mod(fi, 2.0) < 0.5 ? -1.0 : 1.0;
        vec2 rootTip = base + vec2(rootSide * (0.070 + 0.010 * mod(fi, 3.0)),
                                   -0.065 - 0.012 * mod(fi + 1.0, 3.0));
        float rootDistance = curvedDistance(
            p, base, rootTip, rootSide * 0.018);
        rootFilaments = max(rootFilaments, line(rootDistance, 0.0022));
        rootGlow += line(rootDistance, 0.010) * 0.08;
    }

    float kickBulbs = 0.0;
    for (int index = 0; index < 3; ++index) {
        int plantIndex = index * 3;
        vec2 bulb = plantBase(plantIndex);
        kickBulbs += line(length(p - bulb)
            - (0.014 + 0.020 * sceneKick), 0.0043) * sceneKick;
    }

    // One pollinator crosses the fixed crown sequence on each beat.
    float route = clamp(beatPhase, 0.0, 0.999) * 6.0;
    int routeIndex = int(floor(route));
    float routeT = fract(route);
    vec2 pollinatorPosition = mix(
        plantTip(routeIndex), plantTip(min(routeIndex + 1, 6)), routeT);
    float pollinator = disc(p, pollinatorPosition,
        0.008 + 0.008 * beatPulse) * clockConfidence
        * (0.10 * energySlow + 0.90 * beatPulse);
    float anticipationSeed = line(length(p - plantBase(0))
        - (0.019 + 0.014 * beatAnticipation), 0.0035)
        * beatAnticipation * clockConfidence;
    float downbeatRoot = line(length(p - plantBase(3)) - 0.027, 0.004)
                       * downbeat * clockConfidence;

    float lowRoots = disc(p, plantBase(3), 0.072) * lowSustain;
    float midLeaves = leaves * midSustain;
    float highCrowns = crownHalos * highSustain;
    float chordWeight = clamp(chroma[1] + 0.76 * chroma[5]
                              + 0.90 * chroma[8], 0.0, 1.0);
    float chordFacet = facets * chordWeight;
    float sectionCanopy = line(curvedDistance(
        p, plantTip(1), plantTip(5), -0.10), 0.0032) * section;

    float ground = 1.0 - smoothstep(-0.47, -0.34, p.y);
    float groundLine = line(p.y + 0.36, 0.0045);
    float groundGrain = 0.5 + 0.5 * sin(p.x * 34.0)
                              * sin(p.y * 47.0 - p.x * 8.0);
    float mist = exp(-pow((p.y + 0.34) / 0.13, 2.0))
               * smoothstep(0.84, 0.62, abs(p.x));

    vec3 result = mix(primary, secondary, 0.52) * ground
                * (0.005 + 0.004 * groundGrain)
                + primary * mist * 0.008
                + primary * rootNetwork * (0.13 + 0.045 * lowSustain)
                + mix(primary, accent, 0.26) * rootFilaments
                  * (0.075 + 0.026 * lowSustain)
                + primary * rootGlow * 0.018
                + mix(primary, secondary, 0.38) * stems
                  * (0.16 + 0.045 * midSustain)
                + primary * stemHalos * 0.022
                + secondary * leaves * 0.13
                + secondary * leafHalos * 0.018
                + mix(secondary, accent, 0.30) * leafBodies
                  * (0.028 + 0.024 * midSustain)
                + accent * leafVeins * 0.11
                + secondary * midLeaves * 0.070
                + plantColor
                + mix(primary, secondary, 0.50) * crowns * 0.018
                + secondary * facets * 0.070
                + accent * chordFacet * 0.065
                + accent * highCrowns * 0.070
                + mix(secondary, vec3(1.0), 0.34) * snareFacets * 0.60
                + accent * kickBulbs * 0.30
                + mix(accent, vec3(1.0), 0.42) * hatDew * 0.34
                + accent * pollinator * 0.40
                + secondary * anticipationSeed * 0.20
                + accent * downbeatRoot * 0.26
                + primary * lowRoots * 0.090
                + accent * sectionCanopy * 0.20
                + primary * groundLine * 0.12;
    result *= 1.0 - 0.62 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
