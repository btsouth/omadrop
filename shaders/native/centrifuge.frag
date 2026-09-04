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
    for (int index = 1; index <= 8; ++index) {
        float t = float(index) / 8.0;
        vec2 point = mix(mix(a, control, t), mix(control, b, t), t);
        closest = min(closest, segmentDistance(p, previous, point));
        previous = point;
    }
    return closest;
}

vec2 curvedPoint(vec2 a, vec2 b, float bend, float t) {
    vec2 direction = b - a;
    vec2 normal = normalize(vec2(-direction.y, direction.x));
    vec2 control = (a + b) * 0.5 + normal * bend;
    return mix(mix(a, control, t), mix(control, b, t), t);
}

float disc(vec2 p, vec2 center, float radius) {
    return 1.0 - smoothstep(radius, radius * 2.15, length(p - center));
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    vec2 center = vec2(-0.13, -0.015);
    vec2 rotorP = p - center;
    float radius = length(rotorP);
    float angle = atan(rotorP.y, rotorP.x);

    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.68, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;
    float lowSustain = 1.0 - exp(-0.34 * (bandLevel[0] + bandLevel[1]));
    float midSustain = 1.0 - exp(-0.34 * (bandLevel[2] + bandLevel[3]));
    float highSustain = 1.0 - exp(-0.34 * (bandLevel[4] + bandLevel[5]));

    vec3 primary = mix(palettePrimary(0.42), vec3(0.02, 0.74, 0.88), 0.18);
    vec3 secondary = mix(paletteSecondary(0.42), vec3(0.92, 0.16, 0.64), 0.18);
    vec3 accent = mix(paletteAccent(0.42), vec3(1.0, 0.75, 0.20), 0.13);

    float plate = 1.0 - smoothstep(0.225, 0.235, radius);
    float plateEdge = line(radius - 0.235, 0.0032);
    float brokenRim = line(radius - 0.302, 0.0040)
                    * smoothstep(0.20, 0.78,
                        0.5 + 0.5 * sin(angle * 3.0 + 0.62));
    float innerRim = line(radius - 0.092, 0.0032);
    float hub = 1.0 - smoothstep(0.046, 0.052, radius);
    float hubEdge = line(radius - 0.052, 0.0032);

    float arms = 0.0;
    float armHalos = 0.0;
    float chambers = 0.0;
    float chamberEdges = 0.0;
    float chamberCores = 0.0;
    float chamberBolts = 0.0;
    float kickWeights = 0.0;
    float snareClamps = 0.0;
    float highTicks = 0.0;
    vec3 chamberColor = vec3(0.0);

    for (int index = 0; index < 6; ++index) {
        float fi = float(index);
        float armAngle = 0.18 + fi * tau / 6.0;
        vec2 direction = vec2(cos(armAngle), sin(armAngle));
        vec2 tangent = vec2(-direction.y, direction.x);
        vec2 armStart = center + direction * 0.078;
        vec2 armEnd = center + direction * 0.253;
        float armDistance = segmentDistance(p, armStart, armEnd);
        arms = max(arms, line(armDistance, 0.0032));
        armHalos += line(armDistance, 0.011) * 0.10;

        vec2 chamberCenter = center + direction * 0.274;
        float chamberDistance = segmentDistance(
            p, chamberCenter - tangent * 0.030,
            chamberCenter + tangent * 0.030);
        float chamber = 1.0 - smoothstep(0.012, 0.022, chamberDistance);
        float chamberEdge = line(chamberDistance - 0.018, 0.0030);
        float core = disc(p, chamberCenter, 0.008 + 0.0015 * mod(fi, 2.0));
        chambers = max(chambers, chamber);
        chamberEdges = max(chamberEdges, chamberEdge);
        chamberCores += core;

        vec2 boltA = chamberCenter - tangent * 0.024;
        vec2 boltB = chamberCenter + tangent * 0.024;
        chamberBolts += disc(p, boltA, 0.0026) + disc(p, boltB, 0.0026);

        float spectrum = 1.0 - exp(-0.26 * spectrumLevel[index * 5 + 2]);
        vec3 localColor = mix(primary, secondary,
            clamp(0.08 + fi * 0.16 + 0.10 * tonalMotion, 0.0, 1.0));
        chamberColor += localColor * chamber
                      * (0.018 + 0.020 * spectrum)
                      + mix(localColor, accent, 0.22) * chamberEdge * 0.13;

        float kickRole = (index == 1 || index == 4) ? 1.0 : 0.0;
        kickWeights += line(length(p - chamberCenter)
            - (0.012 + 0.018 * sceneKick), 0.0038)
            * sceneKick * kickRole;

        float snareRole = (index == 0 || index == 3) ? 1.0 : 0.0;
        float clampDistance = segmentDistance(
            p, chamberCenter - tangent * (0.038 + 0.018 * sceneSnare),
            chamberCenter + tangent * (0.038 + 0.018 * sceneSnare));
        snareClamps += line(clampDistance, 0.0033)
                     * sceneSnare * snareRole;

        float tickRole = mod(fi, 2.0) < 0.5 ? 1.0 : 0.42;
        vec2 tickStart = center + direction * 0.312;
        vec2 tickEnd = center + direction * (0.322 + 0.015 * sceneHat);
        highTicks += line(segmentDistance(p, tickStart, tickEnd), 0.0026)
                   * sceneHat * tickRole;
    }

    float plateGrain = 0.5 + 0.5
        * sin(rotorP.x * 39.0 + rotorP.y * 13.0)
        * sin(rotorP.y * 33.0 - rotorP.x * 9.0);
    float plateFacet = line(sin(angle * 6.0 + radius * 11.0), 0.10)
                     * plate * (0.18 + 0.82 * midSustain);

    // One local counterweight carries beat position around the fixed rotor.
    float beatAngle = beatPhase * tau + 0.18;
    vec2 beatDirection = vec2(cos(beatAngle), sin(beatAngle));
    vec2 beatPosition = center + beatDirection * 0.302;
    float counterweight = disc(p, beatPosition,
        0.006 + 0.008 * beatPulse) * clockConfidence
        * (0.12 * energySlow + 0.88 * beatPulse);
    float anticipation = line(length(p - center)
        - (0.064 + 0.020 * beatAnticipation), 0.0030)
        * beatAnticipation * clockConfidence;
    float downbeatHub = line(length(p - center) - 0.040, 0.0040)
                      * downbeat * clockConfidence;

    vec2 streamStart = center + vec2(0.275, 0.075);
    vec2 streamEnd = vec2(0.73, 0.19);
    float streamPath = line(curvedDistance(
        p, streamStart, streamEnd, 0.082), 0.0024);
    float streamHalo = line(curvedDistance(
        p, streamStart, streamEnd, 0.082), 0.010);
    float sampleParticles = 0.0;
    float hatParticles = 0.0;
    for (int index = 0; index < 9; ++index) {
        float fi = float(index);
        float t = (fi + 0.45) / 9.0;
        vec2 particle = curvedPoint(streamStart, streamEnd, 0.082, t);
        particle += vec2(0.006 * sin(fi * 2.7),
                         0.012 * sin(fi * 1.9 + 0.6));
        float spectrum = 1.0 - exp(-0.28 * spectrumLevel[index * 3 + 1]);
        sampleParticles += disc(p, particle,
            0.0035 + 0.0035 * spectrum) * (0.12 + 0.88 * spectrum);
        float hatRole = mod(fi, 3.0) < 0.5 ? 1.0 : 0.0;
        hatParticles += disc(p, particle,
            0.004 + 0.007 * sceneHat) * sceneHat * hatRole;
    }

    float sectionJet = streamPath * section;
    float sectionGate = line(curvedDistance(
        p, center + vec2(-0.28, -0.12), streamEnd, -0.25), 0.0030)
        * section;
    float lowPlate = plate * (1.0 - smoothstep(-0.02, 0.20, rotorP.y))
                   * lowSustain;
    float middleArms = arms * midSustain;
    float upperRim = brokenRim * smoothstep(-0.05, 0.20, rotorP.y)
                   * highSustain;

    float background = exp(-pow(radius / 0.52, 2.0)) * 0.010
                     + exp(-pow((p.y + 0.32) / 0.16, 2.0))
                     * smoothstep(0.82, 0.55, abs(p.x)) * 0.006;
    vec3 result = mix(primary, secondary, 0.48) * background
                + mix(primary, secondary, 0.32) * plate
                  * (0.010 + 0.006 * plateGrain)
                + primary * plateEdge * 0.11
                + secondary * brokenRim * 0.14
                + primary * innerRim * 0.080
                + mix(primary, secondary, 0.42) * hub * 0.035
                + accent * hubEdge * 0.16
                + primary * arms * 0.13
                + primary * armHalos * 0.018
                + chamberColor
                + accent * chamberCores * 0.080
                + mix(accent, vec3(1.0), 0.35) * chamberBolts * 0.13
                + secondary * plateFacet * 0.030
                + primary * lowPlate * 0.025
                + secondary * middleArms * 0.080
                + accent * upperRim * 0.060
                + accent * kickWeights * 0.27
                + mix(secondary, vec3(1.0), 0.36) * snareClamps * 0.32
                + mix(accent, vec3(1.0), 0.52) * highTicks * 0.22
                + accent * counterweight * 0.30
                + secondary * anticipation * 0.19
                + accent * downbeatHub * 0.28
                + primary * streamPath * (0.045 + 0.030 * harmonic)
                + primary * streamHalo * 0.012
                + mix(primary, accent, 0.34) * sampleParticles * 0.080
                + mix(accent, vec3(1.0), 0.45) * hatParticles * 0.18
                + accent * sectionJet * 0.13
                + secondary * sectionGate * 0.12;
    result *= 1.0 - 0.60 * release;
    color = vec4(max(result, vec3(0.0)), 1.0);
}
