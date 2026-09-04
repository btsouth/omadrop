#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

float diamondDistance(vec2 p, vec2 halfSize) {
    return abs(p.x) / max(halfSize.x, 0.001)
         + abs(p.y) / max(halfSize.y, 0.001) - 1.0;
}

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float position = clamp(dot(p - a, ab) / max(0.0001, dot(ab, ab)), 0.0, 1.0);
    return length(p - a - ab * position);
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    float overload = smoothstep(2.55, 3.0, kick + snare + hat);
    float gestureBudget = mix(1.0, 0.68, overload);
    float sceneKick = kick * gestureBudget;
    float sceneSnare = snare * gestureBudget;
    float sceneHat = hat * gestureBudget;

    vec2 previousP = p;
    previousP.x -= 0.000018 * harmonic * motionScale;
    previousP.y += 0.000014 * energySlow * motionScale;
    vec2 previousUv = previousP / aspect + 0.5;
    float screenEdge = smoothstep(0.0, 0.075, uv.x)
                     * smoothstep(0.0, 0.075, uv.y)
                     * smoothstep(0.0, 0.075, 1.0 - uv.x)
                     * smoothstep(0.0, 0.075, 1.0 - uv.y);
    vec3 feedback = texture(previousFrame, clamp(previousUv, 0.001, 0.999)).rgb
                  * mix(0.845, 0.910, harmonic) * screenEdge;

    float glassBody = 0.0;
    float glassEdge = 0.0;
    float innerEdge = 0.0;
    float caustics = 0.0;
    float lightFacet = 0.0;
    float darkFacet = 0.0;
    float lowResonance = 0.0;
    float fractures = 0.0;
    float fractureCuts = 0.0;
    float glints = 0.0;
    float timingLight = 0.0;
    float downbeatCap = 0.0;
    for (int index = 0; index < 5; ++index) {
        float fi = float(index);
        float centerX = -0.42;
        float centerY = -0.02;
        float halfHeight = 0.30;
        float halfWidth = 0.15;
        float angle = -0.20;
        if (index == 1) {
            centerX = -0.16; centerY = 0.06;
            halfHeight = 0.38; halfWidth = 0.18; angle = 0.12;
        } else if (index == 2) {
            centerX = 0.08; centerY = -0.05;
            halfHeight = 0.24; halfWidth = 0.12; angle = -0.12;
        } else if (index == 3) {
            centerX = 0.30; centerY = 0.10;
            halfHeight = 0.19; halfWidth = 0.105; angle = 0.72;
        } else if (index == 4) {
            centerX = 0.49; centerY = -0.02;
            halfHeight = 0.29; halfWidth = 0.145; angle = -0.08;
        }
        vec2 q = p - vec2(centerX, centerY);
        q = rotate2d(angle) * q;
        q.x += 0.030 * sin(q.y * 5.2 + fi);
        float distanceToGlass = diamondDistance(
            q, vec2(halfWidth, halfHeight));
        float pane = 1.0 - smoothstep(-0.025, 0.040, distanceToGlass);
        float paneEdge = line(distanceToGlass, 0.026);
        float inset = line(distanceToGlass + 0.16, 0.020);
        float level = 1.0 - exp(-0.20 * spectrumLevel[index * 6 + 2]);
        float harmonicOffset = 0.25 * (harmonic - 0.5)
                             + 0.18 * harmonicChange
                             + 0.16 * level;
        float caustic = line(sin(q.y * (15.0 + fi)
                               + q.x * (8.0 - fi)
                               + harmonicOffset * tau), 0.18);
        float crossCaustic = line(sin(q.x * (20.0 - fi)
                                    - q.y * (6.0 + fi)
                                    - harmonicOffset * tau * 0.7), 0.14);
        caustic = max(caustic, crossCaustic * 0.62);
        caustic *= pane * (0.36 + 0.64 * level);
        float facetDivider = smoothstep(-0.035, 0.035,
                                        q.x + q.y * (0.22 - 0.07 * fi));
        lightFacet = max(lightFacet, pane * facetDivider
                         * (0.35 + 0.65 * level));
        darkFacet = max(darkFacet, pane * (1.0 - facetDivider));

        float lowZone = 1.0 - smoothstep(-0.10, 0.10, q.y);
        float kickGate = index < 2 ? 1.0 : 0.0;
        float kickBand = line(q.y + halfHeight * 0.46,
            0.007 + 0.013 * sceneKick) * pane;
        lowResonance += kickBand * sceneKick * kickGate;

        vec2 fractureA = vec2(-halfWidth * 0.85, -halfHeight * 0.32);
        vec2 fractureB = vec2(halfWidth * 0.75, halfHeight * 0.38);
        float fracture = line(segmentDistance(q, fractureA, fractureB),
                              0.003 + 0.003 * sceneSnare);
        float snareGate = index == 2 || index == 3 ? 1.0 : 0.0;
        fractures += fracture * pane * sceneSnare * snareGate;
        float fractureCut = 1.0 - smoothstep(
            0.002, 0.005 + 0.010 * sceneSnare,
            segmentDistance(q, fractureA, fractureB));
        fractureCuts += fractureCut * pane
                      * smoothstep(0.05, 0.26, sceneSnare) * snareGate;

        vec2 top = vec2(0.0, halfHeight - 0.018);
        float glint = 1.0 - smoothstep(
            0.008, 0.022 + 0.008 * sceneHat,
            length(q - top - vec2(0.018 * sin(fi * 2.4), 0.0)));
        float hatGate = mod(fi, 2.0) < 0.5 ? 1.0 : 0.45;
        glints += glint * sceneHat * hatGate;

        float timingY = mix(-halfHeight * 0.78, halfHeight * 0.78,
                            1.0 - beatPhase);
        float timingX = -halfWidth * (1.0 - clamp(
            abs(timingY) / max(halfHeight, 0.001), 0.0, 1.0));
        float timing = 1.0 - smoothstep(
            0.006, 0.017 + 0.006 * beatPulse,
            length(q - vec2(timingX, timingY)));
        timing *= pane * beatPulse * clockConfidence;
        timingLight += timing * (index == 1 ? 1.0 : 0.0);
        float cap = line(q.y - halfHeight, 0.008)
                  * line(q.x, halfWidth * 0.58)
                  * pane * downbeat * (index == 2 ? 1.0 : 0.0);
        downbeatCap += cap;

        glassBody = max(glassBody, pane * (0.54 + 0.46 * lowZone));
        glassEdge = max(glassEdge, paneEdge);
        innerEdge = max(innerEdge, inset);
        caustics += caustic;
    }

    float anticipationTick = line(p.x + 0.25, 0.004)
                           * line(p.y - 0.36, 0.018)
                           * beatAnticipation * clockConfidence;
    vec2 sectionP = rotate2d(-0.10) * (p - vec2(0.0, 0.02));
    float sectionPaneDistance = diamondDistance(
        sectionP, vec2(0.22 + 0.04 * section, 0.40));
    float sectionPane = line(sectionPaneDistance, 0.024) * section;

    vec3 primary = palettePrimary(5.70);
    vec3 secondary = paletteSecondary(5.70);
    vec3 accent = paletteAccent(5.70);
    vec3 glassColor = mix(primary, secondary, 0.24);
    vec3 injection = glassColor * glassBody * (0.018 + 0.020 * harmonic)
                   + mix(glassColor, accent, 0.22) * lightFacet * 0.010
                   + primary * darkFacet * 0.006
                   + primary * glassEdge * 0.082
                   + secondary * innerEdge * 0.040
                   + mix(primary, accent, 0.30) * caustics * 0.052
                   + primary * lowResonance * 0.18
                   + mix(accent, vec3(1.0), 0.30) * fractures * 0.22
                   + accent * glints * 0.24
                   + accent * timingLight * 0.20
                   + secondary * anticipationTick * 0.11
                   + accent * downbeatCap * 0.13
                   + secondary * sectionPane * 0.10;
    injection *= 1.0 - 0.60 * release;

    vec2 refractOffset = vec2(0.0025 * harmonicChange,
                              0.0018 * tonalMotion);
    vec3 refraction = texture(previousFrame,
        clamp(previousUv + refractOffset, 0.001, 0.999)).gbr;
    vec3 result = feedback + injection + refraction * glassBody * 0.035;
    result *= 1.0 - clamp(fractureCuts, 0.0, 1.0) * 0.68;
    result = max(result - vec3(0.0039), vec3(0.0));
    color = vec4(result, 1.0);
}
