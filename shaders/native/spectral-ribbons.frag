#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;

    float lowZone = 1.0 - smoothstep(-0.20, 0.02, p.y);
    float highZone = smoothstep(0.08, 0.25, p.y);
    float middleZone = clamp(1.0 - lowZone - highZone, 0.0, 1.0);
    float rawSceneSnare = 1.0 - exp(-3.0 * snare);
    float sceneKick = kick / (1.0 + 0.42 * hat + 0.15 * rawSceneSnare);
    float sceneHat = hat / (1.0 + 0.42 * kick + 0.15 * rawSceneSnare);
    float sceneSnare = rawSceneSnare
                     / (1.0 + 0.75 * min(kick, hat));
    // Each percussion role owns a moving part of the weave. Keeping these
    // windows narrow prevents ordinary program energy from bending or
    // brightening every ribbon at once.
    float kickFocus = exp(-80.0 * (p.x + 0.28) * (p.x + 0.28));
    float snareFocus = exp(-96.0 * (p.x - 0.06) * (p.x - 0.06));
    float hatCenter = mix(-0.62, 0.62, fract(beatPhase + 0.18));
    float hatFocus = exp(-180.0 * (p.x - hatCenter) * (p.x - hatCenter));
    float localKick = sceneKick * kickFocus;
    float localSnare = sceneSnare * snareFocus;
    float localHat = sceneHat * hatFocus;
    float roleGesture = localKick * lowZone + localSnare * middleZone
                      + localHat * highZone;
    // Sustained frequency groups shape separate portions of the lines. This
    // makes melody and instrumentation readable between percussion hits
    // without applying one shared scale or brightness pulse to the field.
    float lowSustain = clamp(0.58 * bandLevel[0] + 0.42 * bandLevel[1],
                             0.0, 1.5);
    float midSustain = clamp(0.46 * bandLevel[2] + 0.54 * bandLevel[3],
                             0.0, 1.5);
    float highSustain = clamp(0.55 * bandLevel[4] + 0.45 * bandLevel[5],
                              0.0, 1.5);

    vec2 previousP = p;
    previousP.x += (0.00003 + 0.00005 * energySlow) * motionScale;
    previousP.y *= 1.0 - 0.014 * localKick * lowZone
                         + 0.0012 * beatAnticipation;
    previousP.y += localHat * highZone * 0.0034 * sin(previousP.x * 52.0);
    vec2 previousUv = previousP / aspect + 0.5;
    float edge = smoothstep(0.0, 0.07, uv.x) * smoothstep(0.0, 0.07, uv.y)
               * smoothstep(0.0, 0.07, 1.0 - uv.x)
               * smoothstep(0.0, 0.07, 1.0 - uv.y);
    vec3 feedback = texture(previousFrame, clamp(previousUv, 0.001, 0.999)).rgb
                  * mix(0.835, 0.915, harmonic)
                  * (1.0 - 0.400 * roleGesture) * edge;

    float lowRibbon = 0.0;
    float midRibbon = 0.0;
    float highRibbon = 0.0;
    float intersections = 0.0;
    float travelers = 0.0;
    float weaveFocus = exp(-5.2 * p.x * p.x);
    float travelerX = mix(-0.72, 0.72, beatPhase);
    for (int index = 0; index < 8; ++index) {
        float fi = float(index);
        float lane = fi - 3.5;
        float y = lane * 0.105 * (1.0 - 0.42 * weaveFocus)
                + weaveFocus * 0.026
                  * sin(fi * 2.1 + phrasePhase * tau);
        float frequency = 2.2 + fi * 0.72;
        float amplitude = 0.013 + 0.008 * development;
        float roleWidth = 0.0;
        float sustain = 0.0;
        float sustainCenter = 0.0;
        float sustainDepth = 0.0;
        if (index < 3) {
            // Low ribbons open vertically and deepen their large curve.
            amplitude += localKick * (0.120 + 0.014 * fi);
            y += sign(y) * localKick * 0.070;
            roleWidth = 0.0052 * localKick;
            sustain = lowSustain;
            sustainCenter = -0.42 + 0.05 * fi;
            sustainDepth = 0.042;
        } else if (index < 6) {
            // Snares make the middle voices fold through one another.
            amplitude += localSnare * 0.210;
            y += localSnare * 0.074 * sin(fi * 2.3 + barPhase * tau);
            roleWidth = 0.0048 * localSnare;
            sustain = midSustain;
            sustainCenter = -0.03 + 0.05 * (fi - 4.0);
            sustainDepth = 0.034;
        } else {
            // Hats reveal short, high-frequency ripples on the upper voices.
            amplitude += localHat * 0.180;
            roleWidth = 0.0040 * localHat;
            sustain = highSustain;
            sustainCenter = 0.39 + 0.06 * (fi - 6.5);
            sustainDepth = 0.026;
        }
        float curve = y + amplitude * sin(p.x * frequency * tau
                    - flowTime * (0.015 + 0.004 * fi)
                    + phrasePhase * tau * 0.18);
        curve += (0.018 + 0.020 * development)
               * sin(p.x * tau * 0.72 + fi * 0.66
                     + phrasePhase * tau * 0.24);
        float sustainWindow = exp(
            -18.0 * (p.x - sustainCenter) * (p.x - sustainCenter));
        curve += sustainWindow * sustain * sustainDepth
               * sin(p.x * tau * (1.4 + 0.18 * fi)
                     + fi * 0.71 + tonalMotion * 1.8);
        if (index < 3) {
            curve += localKick * 0.052
                   * sin(p.x * (frequency + 1.4) * tau + fi * 0.7);
        } else if (index < 6) {
            curve += localSnare * 0.205
                   * sin(p.x * frequency * 1.75 * tau + fi * 0.9);
        } else {
            curve += localHat * 0.160
                   * sin(p.x * frequency * 3.2 * tau - flowTime * 3.0 + fi);
        }
        float roleDepth = index < 3 ? 1.0 : index < 6 ? 0.78 : 0.62;
        float ribbon = line(p.y - curve,
                            0.005 + roleWidth);
        ribbon *= smoothstep(0.88, 0.68, abs(p.x)) * roleDepth;
        if (index < 3) lowRibbon = max(lowRibbon, ribbon);
        else if (index < 6) midRibbon = max(midRibbon, ribbon);
        else highRibbon = max(highRibbon, ribbon);
        intersections += ribbon * line(sin(p.x * 32.0 + fi), 0.06)
                       * localHat * (0.42 + 0.88 * weaveFocus);
        travelers += ribbon * line(p.x - travelerX, 0.026)
                   * clockConfidence
                   * (0.12 + 0.62 * beatPulse + 0.26 * downbeat);
    }
    float sectionBand = line(abs(p.y) - mix(0.06, 0.46, section), 0.012) * section;
    float snareFractures = line(
        sin((p.x + p.y * 0.72) * 24.0 + barPhase * tau), 0.062);
    snareFractures *= middleZone * localSnare
                    * smoothstep(0.78, 0.62, abs(p.x));
    float harmonicField = line(sin(p.x * 7.0 + p.y * 13.0 - flowTime * 0.07), 0.24)
                        * harmonic * 0.09;

    vec3 primary = palettePrimary(3.76);
    vec3 secondary = paletteSecondary(3.76);
    vec3 accent = paletteAccent(3.76);
    vec3 injection = primary * lowRibbon
                     * (0.14 + 1.00 * localKick)
                   + secondary * midRibbon
                     * (0.11 + 0.24 * localSnare)
                   + accent * highRibbon
                     * (0.10 + 1.90 * localHat)
                   + mix(secondary, vec3(1.0), 0.46)
                     * snareFractures * 2.20
                   + accent * (intersections + travelers + sectionBand) * 0.20
                   + mix(primary, secondary, 0.5) * harmonicField * 0.07;
    injection *= 1.0 - 0.57 * release;
    vec3 result = feedback + injection;
    result = max(result - vec3(0.0045), vec3(0.0));
    color = vec4(result, 1.0);
}
