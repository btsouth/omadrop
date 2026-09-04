#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    float radius = length(p);
    float angle = atan(p.y, p.x);

    vec2 previousP = p;
    previousP = rotate2d(0.0002 * motionScale) * previousP;
    vec2 previousUv = previousP / aspect + 0.5;
    vec3 feedback = texture(previousFrame,
                            clamp(previousUv, 0.001, 0.999)).rgb * 0.91;

    float kickWindow = exp(-42.0 * (angle + 1.8) * (angle + 1.8));
    float snareWindow = exp(-38.0 * (angle - 0.1) * (angle - 0.1));
    float orbitRadius = 0.28 + 0.035 * kick * kickWindow;
    float orbit = line(radius - orbitRadius, 0.008 + 0.003 * kickWindow * kick);
    float crossCut = line(p.y - 0.04, 0.004) * snare * snareWindow;
    float glintAngle = beatPhase * tau - 3.14159265;
    vec2 glintPoint = vec2(cos(glintAngle), sin(glintAngle)) * orbitRadius;
    float glint = (1.0 - smoothstep(0.008, 0.025, length(p - glintPoint)))
                * hat;
    float sectionRing = line(radius - mix(0.05, 0.62, section), 0.006)
                      * section;

    vec3 primary = palettePrimary(0.7);
    vec3 secondary = paletteSecondary(0.7);
    vec3 accent = paletteAccent(0.7);
    vec3 result = feedback
                + primary * orbit * (0.16 + 0.10 * harmonic)
                + secondary * crossCut * 0.65
                + accent * (glint * 1.2 + sectionRing * 0.35);
    color = vec4(max(result - vec3(0.004), vec3(0.0)), 1.0);
}
