#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float boxDistance(vec3 p, vec3 halfSize) {
    vec3 q = abs(p) - halfSize;
    return length(max(q, vec3(0.0))) + min(max(q.x, max(q.y, q.z)), 0.0)
         - 0.025;
}

vec2 nearer(vec2 a, vec2 b) {
    return a.x < b.x ? a : b;
}

vec2 portalFrame(vec3 p, vec3 center, vec2 opening, float thickness,
                 float depth, float angle, float sideMaterial,
                 float beamMaterial) {
    vec3 local = p - center;
    local.xy = rotate2d(angle) * local.xy;
    vec3 sideSize = vec3(thickness * 0.5,
                         opening.y + thickness * 0.5, depth);
    vec2 hit = vec2(boxDistance(
        local - vec3(-opening.x - thickness * 0.5, 0.0, 0.0), sideSize),
        sideMaterial);
    hit = nearer(hit, vec2(boxDistance(
        local - vec3(opening.x + thickness * 0.5, 0.0, 0.0), sideSize),
        sideMaterial));
    hit = nearer(hit, vec2(boxDistance(
        local - vec3(0.0, opening.y + thickness * 0.5, 0.0),
        vec3(opening.x + thickness, thickness * 0.5, depth)),
        beamMaterial));
    return hit;
}

vec2 architecture(vec3 p) {
    vec2 hit = vec2(p.y + 1.02, 1.0);
    hit = nearer(hit, portalFrame(p, vec3(-0.26, -0.06, -0.22),
        vec2(0.92, 0.83), 0.27, 0.30, 0.055, 2.0, 3.0));
    hit = nearer(hit, portalFrame(p, vec3(0.16, -0.12, -1.62),
        vec2(0.67, 0.61), 0.17, 0.22, -0.075, 4.0, 5.0));
    hit = nearer(hit, portalFrame(p, vec3(-0.04, -0.16, -2.78),
        vec2(0.43, 0.40), 0.11, 0.16, 0.105, 6.0, 7.0));
    return hit;
}

vec3 sceneNormal(vec3 p) {
    const float epsilon = 0.0025;
    const vec2 offset = vec2(epsilon, 0.0);
    float center = architecture(p).x;
    return normalize(vec3(
        architecture(p + offset.xyy).x - center,
        architecture(p + offset.yxy).x - center,
        architecture(p + offset.yyx).x - center));
}

float softShadow(vec3 origin, vec3 direction) {
    float visibility = 1.0;
    float travel = 0.025;
    for (int index = 0; index < 22; ++index) {
        float distanceToScene = architecture(origin + direction * travel).x;
        visibility = min(visibility, 11.0 * distanceToScene / travel);
        travel += clamp(distanceToScene, 0.018, 0.30);
        if (distanceToScene < 0.001 || travel > 7.0) break;
    }
    return clamp(visibility, 0.0, 1.0);
}

float ambientOcclusion(vec3 position, vec3 normal) {
    float occlusion = 0.0;
    float scale = 1.0;
    for (int index = 0; index < 4; ++index) {
        float horizon = 0.055 + 0.085 * float(index);
        float distanceToScene = architecture(position + normal * horizon).x;
        occlusion += (horizon - distanceToScene) * scale;
        scale *= 0.58;
    }
    return clamp(1.0 - 1.45 * occlusion, 0.25, 1.0);
}

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(0.0001, dot(ab, ab)), 0.0, 1.0);
    return length(p - a - ab * position);
}

void main() {
    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.64, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;
    float lowSustain = 1.0 - exp(
        -0.34 * (bandLevel[0] + bandLevel[1]));
    float midSustain = 1.0 - exp(
        -0.34 * (bandLevel[2] + bandLevel[3]));
    float highSustain = 1.0 - exp(
        -0.34 * (bandLevel[4] + bandLevel[5]));

    vec2 p = (uv * 2.0 - 1.0)
           * vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec3 camera = vec3(0.12, 0.18, 3.88);
    vec3 target = vec3(0.06, 0.42, -0.92);
    vec3 forward = normalize(target - camera);
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    vec3 up = cross(right, forward);
    vec3 ray = normalize(forward + p.x * right * 0.50 + p.y * up * 0.50);

    float travel = 0.0;
    float material = 0.0;
    bool found = false;
    for (int index = 0; index < 72; ++index) {
        vec2 samplePoint = architecture(camera + ray * travel);
        if (samplePoint.x < 0.0015) {
            material = samplePoint.y;
            found = true;
            break;
        }
        travel += samplePoint.x * 0.82;
        if (travel > 16.0) break;
    }

    vec3 primary = mix(palettePrimary(0.10), vec3(0.04, 0.46, 1.0), 0.14);
    vec3 secondary = mix(paletteSecondary(0.10), vec3(0.72, 0.06, 1.0), 0.13);
    vec3 accent = mix(paletteAccent(0.10), vec3(1.0, 0.48, 0.10), 0.12);
    float voidShaft = exp(-abs(p.x - 0.16 - p.y * 0.18) * 4.2)
                    * smoothstep(-0.72, 0.38, p.y);
    float portalHaze = exp(-length((p - vec2(0.08, 0.00))
                           * vec2(1.0, 1.45)) * 2.8);
    vec3 result = vec3(0.0015, 0.0020, 0.0060)
                + mix(primary, secondary, 0.58) * voidShaft * 0.009
                + secondary * portalHaze * 0.006;
    if (found) {
        vec3 position = camera + ray * travel;
        vec3 normal = sceneNormal(position);

        vec3 keyDirection = normalize(vec3(-0.62, 0.82, 0.48));
        float key = max(0.0, dot(normal, keyDirection));
        float shadow = softShadow(position + normal * 0.009, keyDirection);
        float rim = pow(max(0.0, 1.0 - abs(dot(normal, -ray))), 2.8);
        float occlusion = ambientOcclusion(position, normal);
        vec3 reflectedKey = reflect(-keyDirection, normal);
        float specular = pow(max(0.0, dot(reflectedKey, -ray)), 28.0)
                       * shadow;
        float floorMaterial = 1.0 - step(1.5, material);
        float sideMaterial = max(
            1.0 - step(0.45, abs(material - 2.0)),
            max(1.0 - step(0.45, abs(material - 4.0)),
                1.0 - step(0.45, abs(material - 6.0))));
        float beamMaterial = max(
            1.0 - step(0.45, abs(material - 3.0)),
            max(1.0 - step(0.45, abs(material - 5.0)),
                1.0 - step(0.45, abs(material - 7.0))));
        float farMaterial = step(3.5, material) * (1.0 - step(7.5, material));
        float nearLayer = 1.0 - step(3.5, material);
        float middleLayer = step(3.5, material) * (1.0 - step(5.5, material));
        float deepLayer = step(5.5, material);

        vec3 concrete = primary * 0.68 * nearLayer
                      + secondary * 0.58 * middleLayer
                      + mix(primary, accent, 0.58) * 0.52 * deepLayer;
        concrete = mix(concrete, mix(primary, secondary, 0.28) * 0.055,
                       floorMaterial * 0.78);
        float grain = 0.94 + 0.06
            * sin(position.x * 7.0 + position.z * 3.0)
            * sin(position.y * 9.0 - position.z * 4.0);
        float baseLight = 0.110 + 0.52 * key * shadow + 0.38 * rim;
        result = concrete * baseLight * grain * occlusion;
        result += mix(accent, vec3(1.0), 0.44) * specular * 0.24;

        // The floor carries a dim, stable reflection of the portals. It adds
        // depth without giving the music another surface to pulse globally.
        if (floorMaterial > 0.5) {
            vec3 reflectionRay = reflect(ray, normal);
            float reflectionTravel = 0.035;
            float reflectionMaterial = 0.0;
            bool reflectionFound = false;
            for (int reflectionStep = 0; reflectionStep < 38; ++reflectionStep) {
                vec2 reflectionSample = architecture(
                    position + normal * 0.018
                    + reflectionRay * reflectionTravel);
                if (reflectionSample.x < 0.0020) {
                    if (reflectionSample.y > 1.5) {
                        reflectionMaterial = reflectionSample.y;
                        reflectionFound = true;
                    }
                    break;
                }
                reflectionTravel += reflectionSample.x * 0.84;
                if (reflectionTravel > 9.0) break;
            }
            if (reflectionFound) {
                float reflectedNear = 1.0 - step(3.5, reflectionMaterial);
                float reflectedMiddle = step(3.5, reflectionMaterial)
                                      * (1.0 - step(5.5, reflectionMaterial));
                float reflectedDeep = step(5.5, reflectionMaterial);
                vec3 reflectedColor = primary * reflectedNear
                                    + secondary * reflectedMiddle
                                    + accent * reflectedDeep;
                float fresnel = pow(1.0 - max(0.0, dot(normal, -ray)), 2.0);
                float reflectionFade = exp(-0.12 * reflectionTravel);
                result += reflectedColor * reflectionFade
                        * (0.050 + 0.15 * fresnel);
            }
            float floorSeams = line(sin(position.x * 3.1), 0.055)
                             + line(sin(position.z * 1.65), 0.045);
            result += mix(primary, secondary, 0.42) * floorSeams * 0.006;
        }

        float inwardFace = sideMaterial * smoothstep(0.72, 0.96, abs(normal.x));
        float embeddedLine = line(position.y + 0.47, 0.018) * inwardFace;
        float beamUnderside = beamMaterial
                            * smoothstep(0.76, 0.96, -normal.y);
        float bevelLight = pow(clamp(1.0 - max(abs(normal.x),
            max(abs(normal.y), abs(normal.z))), 0.0, 1.0), 1.8);
        float normalEdge = smoothstep(0.16, 0.62,
            length(vec3(fwidth(normal.x), fwidth(normal.y), fwidth(normal.z))));
        float depthEdge = smoothstep(0.035, 0.24, fwidth(travel));
        float silhouetteEdge = max(normalEdge, depthEdge) * (1.0 - floorMaterial);
        float insetLight = line(position.y + 0.46 + 0.055 * material, 0.012)
                         * sideMaterial;
        float lowFloorCourse = floorMaterial
            * line(sin(position.z * 2.4 + position.x * 1.2), 0.080)
            * lowSustain;
        float midWallCourse = sideMaterial
            * line(sin(position.y * 5.5 + position.z * 1.8), 0.11)
            * midSustain * smoothstep(3.4, 0.3, abs(position.z));
        float highBeamJoints = beamMaterial
            * line(sin(position.x * 8.0 + material * 0.7), 0.095)
            * highSustain;
        result += mix(primary, secondary, 0.45) * embeddedLine * 0.055
                + secondary * beamUnderside * 0.060
                + accent * bevelLight * (0.034 + 0.018 * deepLayer)
                + mix(primary, accent, 0.28) * silhouetteEdge * 0.17
                + mix(secondary, accent, 0.40) * insetLight * 0.075
                + primary * lowFloorCourse * 0.075
                + secondary * midWallCourse * 0.12
                + mix(accent, vec3(1.0), 0.24) * highBeamJoints * 0.095;

        vec3 kickSource = vec3(-0.54, -0.82, 1.06);
        vec3 toKick = kickSource - position;
        float kickDistance = dot(toKick, toKick);
        float kickLambert = max(0.0, dot(normal, normalize(toKick)));
        float kickRegion = floorMaterial
                         + sideMaterial * smoothstep(-0.05, -0.80, position.y);
        float kickLight = sceneKick * kickLambert
                        * exp(-0.52 * kickDistance)
                        * exp(-0.85 * abs(position.x + 0.34))
                        * clamp(kickRegion, 0.0, 1.0);
        result += primary * kickLight * 0.72;

        float incisionCoordinate = position.y + 0.72 * position.x + 0.24;
        float snareStrip = exp(-abs(incisionCoordinate) * 42.0)
                         * max(sideMaterial, beamMaterial) * sceneSnare;
        result *= 1.0 - snareStrip * 0.66;
        result += mix(accent, vec3(1.0), 0.28) * snareStrip * 0.82;

        float jointCell = abs(fract((position.x + 1.60) * 2.5) - 0.5);
        float joint = smoothstep(0.10, 0.02, jointCell)
                    * beamMaterial * sceneHat;
        result += accent * joint * (0.62 + 0.30 * spectralCentroid);

        float farVein = line(sin(position.y * 10.0 + position.x * 5.0
                                 + harmonicChange * 1.4), 0.15);
        result += secondary * farVein * farMaterial
                * (0.012 + 0.030 * harmonic);
        result += secondary * farMaterial * section * 0.12;

        float fog = 1.0 - exp(-0.040 * travel * travel);
        vec3 fogColor = mix(primary, secondary, 0.62) * 0.026;
        result = mix(result, fogColor, fog * 0.76);
    }

    vec2 pathA = vec2(0.055, -0.02);
    vec2 pathB = vec2(-0.015, -0.57);
    vec2 beatPoint = mix(pathA, pathB, beatPhase * beatPhase);
    float beatGuide = line(segmentDistance(p, pathA, pathB), 0.0025) * 0.20;
    float beatMarker = 1.0 - smoothstep(0.008, 0.020,
        length(p - beatPoint));
    beatMarker *= clockConfidence * (0.30 + 0.70 * beatPulse);
    float anticipationMark = line(p.x - 0.58, 0.003)
                           * line(p.y + 0.46, 0.020)
                           * beatAnticipation * clockConfidence;
    float downbeatLintel = line(p.y - 0.27, 0.005)
                         * line(p.x - 0.15, 0.115) * downbeat;

    vec2 chamberP = p - vec2(0.15, 0.02);
    float chamberDistance = max(abs(chamberP.x) / 0.22,
                                abs(chamberP.y) / 0.17) - 1.0;
    float sectionFrame = line(chamberDistance, 0.025) * section;
    result += accent * beatMarker * 0.24
            + primary * beatGuide * 0.025
            + secondary * anticipationMark * 0.11
            + accent * downbeatLintel * 0.14
            + secondary * sectionFrame * 0.13;
    result *= 1.0 - 0.62 * release;
    result = max(result, vec3(0.0));
    color = vec4(result, 1.0);
}
