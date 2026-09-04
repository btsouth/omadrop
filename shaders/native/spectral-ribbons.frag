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
    float roleGesture = sceneKick * lowZone + sceneSnare * middleZone
                      + sceneHat * highZone;

    vec2 previousP = p;
    previousP.x += (0.00003 + 0.00005 * energySlow) * motionScale;
    previousP.y *= 1.0 - 0.003 * beatPulse - 0.026 * sceneKick * lowZone
                         + 0.0012 * beatAnticipation;
    previousP.y += sceneHat * highZone * 0.0026 * sin(previousP.x * 52.0);
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
        float band = spectrumLevel[index * 4 + 1];
        float frequency = 2.2 + fi * 0.72;
        float amplitude = 0.013 + 0.0015 * band + 0.008 * development;
        float roleWidth = 0.0;
        if (index < 3) {
            // Low ribbons open vertically and deepen their large curve.
            amplitude += sceneKick * (0.105 + 0.012 * fi);
            y += sign(y) * sceneKick * 0.062;
            roleWidth = 0.0045 * sceneKick;
        } else if (index < 6) {
            // Snares make the middle voices fold through one another.
            amplitude += sceneSnare * 0.180;
            y += sceneSnare * 0.063 * sin(fi * 2.3 + barPhase * tau);
            roleWidth = 0.0040 * sceneSnare;
        } else {
            // Hats reveal short, high-frequency ripples on the upper voices.
            amplitude += sceneHat * 0.150;
            roleWidth = 0.0032 * sceneHat;
        }
        float curve = y + amplitude * sin(p.x * frequency * tau
                    - flowTime * (0.015 + 0.004 * fi)
                    + phrasePhase * tau * 0.18);
        curve += (0.018 + 0.020 * development)
               * sin(p.x * tau * 0.72 + fi * 0.66
                     + phrasePhase * tau * 0.24);
        if (index < 3) {
            curve += sceneKick * 0.044
                   * sin(p.x * (frequency + 1.4) * tau + fi * 0.7);
        } else if (index < 6) {
            curve += sceneSnare * 0.170
                   * sin(p.x * frequency * 1.75 * tau + fi * 0.9);
        } else {
            curve += sceneHat * 0.130
                   * sin(p.x * frequency * 3.2 * tau - flowTime * 3.0 + fi);
        }
        float roleDepth = index < 3 ? 1.0 : index < 6 ? 0.78 : 0.62;
        float ribbon = line(p.y - curve,
                            0.005 + 0.0035 * band + roleWidth);
        ribbon *= smoothstep(0.88, 0.68, abs(p.x)) * roleDepth;
        if (index < 3) lowRibbon = max(lowRibbon, ribbon);
        else if (index < 6) midRibbon = max(midRibbon, ribbon);
        else highRibbon = max(highRibbon, ribbon);
        intersections += ribbon * line(sin(p.x * 32.0 + fi), 0.06)
                       * sceneHat * (0.42 + 0.88 * weaveFocus);
        travelers += ribbon * line(p.x - travelerX, 0.026)
                   * clockConfidence * (0.16 + 0.84 * downbeat);
    }
    float sectionBand = line(abs(p.y) - mix(0.06, 0.46, section), 0.012) * section;
    float snareFractures = line(
        sin((p.x + p.y * 0.72) * 24.0 + barPhase * tau), 0.062);
    snareFractures *= middleZone * sceneSnare
                    * smoothstep(0.78, 0.62, abs(p.x));
    float harmonicField = line(sin(p.x * 7.0 + p.y * 13.0 - flowTime * 0.07), 0.24)
                        * harmonic * 0.09;

    vec3 primary = palettePrimary(3.76);
    vec3 secondary = paletteSecondary(3.76);
    vec3 accent = paletteAccent(3.76);
    vec3 injection = primary * lowRibbon
                     * (0.14 + 0.08 * bandLevel[0]
                        + 0.88 * sceneKick)
                   + secondary * midRibbon
                     * (0.11 + 0.06 * bandLevel[3]
                        + 0.20 * sceneSnare)
                   + accent * highRibbon
                     * (0.10 + 0.07 * bandLevel[5]
                        + 1.65 * sceneHat)
                   + mix(secondary, vec3(1.0), 0.46)
                     * snareFractures * 2.20
                   + accent * (intersections + travelers + sectionBand) * 0.20
                   + mix(primary, secondary, 0.5) * harmonicField * 0.07;
    injection *= 1.0 - 0.57 * release;
    vec3 result = feedback + injection;
    result = max(result - vec3(0.0045), vec3(0.0));
    color = vec4(result, 1.0);
}
