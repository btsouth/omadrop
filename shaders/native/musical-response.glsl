// Musical response interface v1. No clock, inferred beat, or scene automation.
// Geometry uses damped impacts; material accents can use immediate attacks.
uniform vec2 resolution;
uniform vec3 albumColor;
uniform float motionScale;
uniform float kick;
uniform float snare;
uniform float hat;
uniform vec3 impactMotion;
uniform float bandMotion[6];
uniform float spectrumMotion[32];
uniform sampler2D previousFrame;
uniform float renderSeconds;
#include "common.glsl"
float musicalBand(int role) {
    int i = clamp(role, 0, 2) * 2;
    return 1.0 - exp(-0.55 * (bandMotion[i] + bandMotion[i + 1]));
}
float musicalDetail(float position) {
    float bin = clamp(position, 0.0, 1.0) * 31.0;
    int i = int(floor(bin));
    return 1.0 - exp(-0.65 * mix(spectrumMotion[i], spectrumMotion[min(i+1,31)],
                                smoothstep(0.0, 1.0, fract(bin))));
}
vec3 musicalFinish(vec3 light) {
    return 1.0 - exp(-max(light, vec3(0.0)));
}
