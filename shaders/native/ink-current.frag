#version 330 core
in vec2 uv;
out vec4 color;
#include "scene-uniforms.glsl"
uniform vec3 impactMotion;
uniform float bandMotion[6];
uniform float spectrumMotion[32];
uniform float grooveMotion;
uniform float musicalExpansion;
uniform float renderSeconds;

// Twisting silk sheets: large folds carry bass, the body carries harmony,
// traveling creases carry snare, and fine ridges carry treble.
float spectral(float x) {
    float bin = clamp(x, 0.0, 1.0) * 31.0;
    int lo = int(floor(bin));
    int hi = min(lo + 1, 31);
    // A continuous tangent between bins prevents polygonal kinks in the cloth.
    return 1.0 - exp(-0.65 * mix(spectrumMotion[lo], spectrumMotion[hi],
                                 smoothstep(0.0, 1.0, fract(bin))));
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    float t = flowTime * 2.5;
    float movement = clamp(motionScale, 0.0, 1.0);
    float low = 1.0 - exp(-0.70 * (bandMotion[0] + bandMotion[1]));
    float mid = 1.0 - exp(-0.55 * (bandMotion[2] + bandMotion[3]));
    float high = 1.0 - exp(-0.55 * (bandMotion[4] + bandMotion[5]));
    float bassHit = min(kick, 1.25);
    float snareHit = min(snare, 1.25);
    float hatHit = min(hat, 1.25);
    float breath = 0.17 * musicalExpansion;
    vec3 cool = mix(vec3(0.008, 0.38, 0.90), palettePrimary(0.2), 0.22);
    vec3 warm = mix(vec3(1.0, 0.16, 0.018), paletteAccent(0.2), 0.12);
    vec3 violet = mix(vec3(0.26, 0.018, 0.65), paletteSecondary(0.2), 0.16);
    vec3 result = vec3(0.0006, 0.0012, 0.003);
    result += cool * 0.009 * exp(-3.8 * dot(p - vec2(-0.22, 0.04),
                                                   p - vec2(-0.22, 0.04)));
    for (int layer = 0; layer < 9; ++layer) {
        float f = float(layer);
        float depth = f / 8.0;
        float phase = f * 0.53;
        float x = p.x * (0.91 + 0.11 * depth);
        float sweep = x * 3.15 - t * 0.63 + phase;
        float twist = x * 4.0 + t * 0.44 - phase * 0.72;
        float lowZone = exp(-2.6 * (x + 0.30) * (x + 0.30));
        float highZone = exp(-3.8 * (x - 0.38) * (x - 0.38));
        float localBand = spectral(x * 0.48 + 0.5 + 0.018 * f);
        float bassWeight = 1.0 - 0.65 * depth;
        float middleWeight = 0.25 + 0.75 * exp(-18.0 * (depth - 0.5) * (depth - 0.5));
        float trebleWeight = 0.15 + 0.85 * depth;
        // Opposing displacements open a fold instead of scaling the whole image.
        float center = 0.105 * x + (0.14 + breath) * sin(sweep)
                     + 0.09 * sin(x * 1.7 + t * 0.28 - phase)
                     + (depth - 0.5) * 0.25;
        center += movement * lowZone * impactMotion.x * bassWeight * 0.24 * sin(sweep + 0.9);
        center += movement * mid * middleWeight * 0.06 * sin(x * 6.0 - t + phase);
        center += movement * localBand * 0.038 * sin(x * 9.0 + phase);
        center += movement * impactMotion.y * middleWeight * 0.12
                * sin(x * 17.0 - t * 2.1 + phase) * exp(-1.8 * x * x);
        center += movement * highZone * impactMotion.z * trebleWeight * 0.024
                * sin(x * 67.0 + phase * 3.0);
        // Groove opens a diagonal fold; phrase and harmonic changes alter its
        // breadth. Confidence gates the inferred beat, never the actual attacks.
        center += movement * 0.027 * grooveMotion
                * sin(sweep - 0.6) * exp(-3.0 * x * x);
        center += movement * 0.033 * (section + harmonicChange)
                * cos(x * 2.0 + phase);
        float facing = sin(twist);
        float halfWidth = (0.028 + 0.067 * abs(facing))
                        * (0.85 + 0.42 * low + 0.20 * localBand);
        float crossSection = (p.y - center) / halfWidth;
        float edge = abs(crossSection);
        float pixel = 1.5 / max(1.0, resolution.y);
        float mask = 1.0 - smoothstep(halfWidth - pixel, halfWidth + pixel,
                                      abs(p.y - center));
        float taper = 1.0 - smoothstep(0.62, 1.04, abs(x));
        mask *= taper;
        float arc = sqrt(max(0.0, 1.0 - crossSection * crossSection));
        vec3 normal = normalize(vec3(0.32 * cos(sweep),
                                     crossSection * 0.85, 0.25 + arc));
        vec3 light = normalize(vec3(-0.35, 0.55, 0.8));
        float diffuse = max(0.0, dot(normal, light));
        float specular = pow(max(0.0, dot(reflect(-light, normal),
                                         vec3(0.0, 0.0, 1.0))), 24.0);
        float rim = exp(-36.0 * (edge - 0.89) * (edge - 0.89));
        float fiberPhase = crossSection * 85.0 + 5.0 * sin(x * 6.0 - t);
        float fibers = 0.88 + 0.12 * sin(fiberPhase)
                     * (1.0 - smoothstep(0.8, 2.8, fwidth(fiberPhase)));
        float groovePhase = crossSection * 210.0 + x * 22.0 - t;
        float grooves = 0.95 + 0.05 * sin(groovePhase)
                      * (1.0 - smoothstep(0.8, 2.8, fwidth(groovePhase)));
        vec3 dye = mix(cool, violet, 0.5 + 0.5 * sin(phase + x * 1.5 + t * 0.17));
        dye = mix(dye, warm, smoothstep(0.10, 0.95,
                   sin(x * 2.4 - phase * 0.36 + t * 0.12)));
        float illumination = (0.20 + 0.90 * diffuse) * (0.48 + 0.52 * depth);
        vec3 sheet = dye * illumination * fibers * grooves;
        sheet += mix(dye, vec3(0.82, 0.94, 1.0), 0.38)
               * (0.32 * specular + 0.15 * rim);
        float crease = pow(0.5 + 0.5 * sin(x * 17.0 - t * 2.1 + phase), 8.0);
        sheet += warm * snareHit * crease * 0.48 * (0.35 + 0.65 * arc);
        float glints = pow(0.5 + 0.5 * sin(x * 112.0 + crossSection * 23.0), 16.0);
        sheet += vec3(0.55, 0.86, 1.0) * highZone * glints
               * (0.07 * high + 0.62 * hatHit) * rim;
        sheet += cool * bassHit * lowZone * 0.24 * arc;
        sheet *= 0.86 + 0.14 * harmonic;
        float shadow = exp(-90.0 * pow(p.y - center + halfWidth + 0.016, 2.0))
                     * taper * 0.16;
        result *= 1.0 - shadow;
        result = mix(result, sheet, mask * 0.95);
        result += dye * exp(-32.0 * abs(p.y - center)) * taper * 0.008;
    }
    float vignette = 1.0 - 0.32 * smoothstep(0.25, 1.1, length(p));
    result = (vec3(1.0) - exp(-result * 1.55)) * vignette;
    // Short persistence softens edges without obscuring the attack frame.
    vec3 previous = texture(previousFrame, uv).rgb;
    float persistence = pow(0.12, max(renderSeconds, 1.0 / 240.0) * 60.0);
    color = vec4(mix(result, previous, persistence), 1.0);
}
