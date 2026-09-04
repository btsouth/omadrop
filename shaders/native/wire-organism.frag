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
    return 1.0 - smoothstep(radius, radius * 2.2, length(p - center));
}

float spineX(float y) {
    return -0.045
         + 0.052 * sin(y * 5.7 + 0.35)
         + 0.019 * sin(y * 13.0 - 0.8)
         + 0.020 * tonalMotion * sin(y * 3.2 + 1.1);
}

float bodyWidth(float y) {
    float normalized = (y + 0.01) / 0.42;
    float envelope = sqrt(max(0.0, 1.0 - normalized * normalized));
    return 0.055 + 0.205 * envelope
         + 0.014 * sin(y * 9.0 - 0.4)
         + 0.012 * development;
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    p.x -= 0.035;

    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.68, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;
    float lowSustain = 1.0 - exp(-0.34 * (bandLevel[0] + bandLevel[1]));
    float midSustain = 1.0 - exp(-0.34 * (bandLevel[2] + bandLevel[3]));
    float highSustain = 1.0 - exp(-0.34 * (bandLevel[4] + bandLevel[5]));

    vec3 primary = mix(palettePrimary(0.88), vec3(0.18, 0.76, 0.78), 0.18);
    vec3 secondary = mix(paletteSecondary(0.88), vec3(0.80, 0.20, 0.76), 0.18);
    vec3 accent = mix(paletteAccent(0.88), vec3(1.0, 0.76, 0.24), 0.12);

    float centerX = spineX(p.y);
    float width = bodyWidth(p.y);
    float verticalWindow = smoothstep(-0.47, -0.42, p.y)
                         * smoothstep(0.47, 0.42, p.y);
    float signedBody = abs(p.x - centerX) - width;
    float body = smoothstep(0.018, -0.018, signedBody) * verticalWindow;
    float bodyOutline = line(signedBody, 0.0045) * verticalWindow;
    float innerOutline = line(signedBody + 0.033, 0.0027) * verticalWindow;
    float spine = line(p.x - centerX, 0.0055) * verticalWindow;
    float spineHalo = line(p.x - centerX, 0.019) * verticalWindow;

    float grain = 0.5 + 0.5
        * sin((p.x - centerX) * 44.0 + p.y * 8.0)
        * sin(p.y * 51.0 - (p.x - centerX) * 12.0);
    float innerCurrent = line(
        sin((p.x - centerX) * 18.0 + p.y * 13.0), 0.20)
        * body * (0.18 + 0.82 * midSustain);

    float branches = 0.0;
    float branchHalos = 0.0;
    float branchTips = 0.0;
    float snareBranches = 0.0;
    for (int index = 0; index < 8; ++index) {
        float fi = float(index);
        float anchorY = -0.31 + fi * 0.088;
        float side = mod(fi, 2.0) < 0.5 ? -1.0 : 1.0;
        vec2 anchor = vec2(spineX(anchorY), anchorY);
        float reach = 0.24 + 0.055 * sin(fi * 1.73 + 0.5);
        vec2 tip = vec2(anchor.x + side * reach,
                        anchorY + 0.055 * sin(fi * 2.11 - 0.7));
        float distanceToBranch = curvedDistance(
            p, anchor, tip, side * (0.025 + 0.010 * sin(fi)));
        float branch = line(distanceToBranch, 0.0032);
        branches = max(branches, branch);
        branchHalos += line(distanceToBranch, 0.012) * 0.20;
        branchTips += disc(p, tip, 0.007 + 0.002 * sin(fi * 1.4));

        float snareRole = step(2.0, fi) * (1.0 - step(6.0, fi));
        float dash = smoothstep(0.30, 0.82,
            sin(dot(p - anchor, normalize(tip - anchor)) * 72.0
                + fi * 1.7));
        snareBranches += branch * dash * snareRole * sceneSnare;
    }

    float rootFilaments = 0.0;
    float rootHalos = 0.0;
    vec2 root = vec2(spineX(-0.42), -0.42);
    for (int index = 0; index < 3; ++index) {
        float fi = float(index);
        vec2 rootTip = vec2(-0.31 + fi * 0.30,
                            -0.455 + 0.024 * sin(fi * 2.2));
        float rootDistance = curvedDistance(
            p, root, rootTip, (fi - 1.0) * 0.035);
        rootFilaments = max(rootFilaments, line(rootDistance, 0.0035));
        rootHalos += line(rootDistance, 0.014) * 0.16;
    }
    float lowRootGlow = disc(p, root, 0.052) * lowSustain;

    float crownCilia = 0.0;
    float crownTips = 0.0;
    vec2 crown = vec2(spineX(0.42), 0.42);
    for (int index = 0; index < 5; ++index) {
        float fi = float(index);
        vec2 crownTip = vec2(-0.30 + fi * 0.15,
                             0.455 - 0.018 * abs(fi - 2.0));
        float ciliumDistance = curvedDistance(
            p, crown, crownTip, (fi - 2.0) * 0.018);
        crownCilia = max(crownCilia, line(ciliumDistance, 0.0024));
        crownTips += disc(p, crownTip,
            0.0045 + 0.010 * sceneHat * (0.55 + 0.45 * sin(fi * 2.3)))
            * sceneHat;
    }

    // Beat position travels along the fixed spine, so rhythm is readable
    // without scaling or bending the whole organism.
    float beatY = mix(-0.39, 0.39, beatPhase);
    vec2 beatCenter = vec2(spineX(beatY), beatY);
    float beatNode = disc(p, beatCenter,
        0.010 + 0.009 * beatPulse) * clockConfidence
        * (0.10 * energySlow + 0.90 * beatPulse);
    float beatTrail = line(segmentDistance(
        p, vec2(spineX(beatY - 0.055), beatY - 0.055), beatCenter),
        0.0032) * beatPulse * clockConfidence;
    float anticipationRoot = line(length(p - root)
        - (0.020 + 0.015 * beatAnticipation), 0.0035)
        * beatAnticipation * clockConfidence;

    // Kicks energize only the three root bulbs.
    float kickBulbs = 0.0;
    for (int index = 0; index < 3; ++index) {
        float fi = float(index);
        vec2 bulb = vec2(-0.31 + fi * 0.30,
                         -0.455 + 0.024 * sin(fi * 2.2));
        kickBulbs += line(length(p - bulb)
            - (0.012 + 0.020 * sceneKick), 0.0045) * sceneKick;
    }

    float heart = line(length(p - vec2(spineX(0.03), 0.03)) - 0.034,
                       0.0045);
    float downbeatHeart = heart * downbeat * clockConfidence;
    float chordWeight = clamp(chroma[2] + 0.72 * chroma[5]
                              + 0.88 * chroma[9], 0.0, 1.0);
    float nucleus = disc(p, vec2(spineX(0.03), 0.03), 0.021)
                  * chordWeight;
    float sectionShell = line(signedBody - 0.028, 0.0045)
                       * verticalWindow * section;

    float aura = exp(-pow((p.x + 0.01) / 0.52, 2.0)
                     -pow((p.y + 0.01) / 0.62, 2.0));
    vec3 bodyColor = mix(primary, secondary,
        clamp(0.46 + 0.28 * p.y + 0.14 * tonalMotion, 0.0, 1.0));
    vec3 result = mix(primary, secondary, 0.48) * aura * 0.012
                + bodyColor * body * (0.027 + 0.010 * grain
                                      + 0.012 * harmonic)
                + primary * bodyOutline * 0.22
                + secondary * innerOutline * 0.075
                + mix(primary, secondary, 0.38) * innerCurrent * 0.050
                + mix(primary, vec3(1.0), 0.20) * spine * 0.30
                + primary * spineHalo * 0.025
                + secondary * branches * (0.14 + 0.035 * midSustain)
                + secondary * branchHalos * 0.020
                + mix(secondary, accent, 0.25) * branchTips * 0.22
                + primary * rootFilaments * (0.16 + 0.055 * lowSustain)
                + primary * rootHalos * 0.022
                + primary * lowRootGlow * 0.085
                + secondary * crownCilia * (0.13 + 0.055 * highSustain)
                + mix(secondary, vec3(1.0), 0.34) * snareBranches * 0.34
                + accent * kickBulbs * 0.36
                + mix(accent, vec3(1.0), 0.40) * crownTips * 0.38
                + accent * beatNode * 0.34
                + accent * beatTrail * 0.13
                + secondary * anticipationRoot * 0.22
                + accent * downbeatHeart * 0.28
                + mix(primary, secondary, 0.62) * nucleus * 0.085
                + accent * sectionShell * 0.18;
    result *= 1.0 - 0.62 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
