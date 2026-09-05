// Musical response interface v3. No clock, inferred beat, or scene automation.
// Geometry and material accents share continuous impacts. Raw attack uniforms
// are deliberately excluded: they caused full-frame flashes between smooth poses.
uniform vec2 resolution;
uniform vec3 albumColor;
uniform float motionScale;
uniform vec3 impactMotion;
uniform float harmonicShape[32];
uniform float bassBody;
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

// Independent bass headroom: middle and high activity cannot consume this range.
float bassGesture() {
    // A low-note transient is not necessarily a kick. Require measured low
    // body for the largest expansion; retain a small cue on quieter masters.
    float bodyWeight=0.20+0.80*smoothstep(0.015,0.12,bassBody);
    return 0.72*(1.0-exp(-5.0*impactMotion.x))*bodyWeight+0.28*bassBody;
}
// Proven Molten excursion. The scene supplies anchors and support radii in
// its own coordinates; the engine supplies already-settled measured drive.
// Keep this mapping independent of the other musical roles.
float coneExcursion() {
    return motionScale*(0.85*(1.0-exp(-4.0*bassBody))
        +0.45*(1.0-exp(-4.0*impactMotion.x)));
}
vec2 coneOffset(vec2 p, vec2 anchor, float interior, float support,
                float excursion) {
    vec2 delta=p-anchor;
    float surround=1.0-smoothstep(interior,support,length(delta));
    return delta*(excursion/(1.0+excursion))*surround;
}
// Bounded sustained spectral windows share the analyzer's amplitude reference.
// These are registers of the mixture, not identified instruments. Scenes can
// assign each window to its own material region without merging their motion.
float sustainedRegister(int first) {
    float strength=0.0;
    for(int i=0;i<4;++i) strength+=harmonicShape[clamp(first+i,0,31)];
    return 1.0-exp(-strength*0.9);
}
// Smooth spatial curve built from the measured harmonic spectrum. Different
// pitched content changes its form, without an oscillator running on a clock.
float melodicCurve(float x) {
    float curve=0.0;
    for(int i=0;i<8;++i) {
        float strength=(harmonicShape[10+i*2]+harmonicShape[11+i*2])*0.5;
        curve+=strength*sin(x*(1.5+float(i)*0.65)+float(i)*1.7);
    }
    return curve*0.22;
}
float melodicPresence() {
    float strength=0.0;
    for(int i=10;i<26;++i) strength+=harmonicShape[i];
    return 1.0-exp(-strength*0.45);
}
