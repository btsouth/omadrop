#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

vec2 starPosition(float index) {
    return vec2(sin(index * 12.9898 + 1.7), sin(index * 78.233 + 0.4))
         * vec2(0.58, 0.39);
}

float segmentDistance(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    float t = clamp(dot(p - a, ab) / max(dot(ab, ab), 0.0001), 0.0, 1.0);
    return length(p - a - ab * t);
}

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;

    vec2 previousP = p;
    previousP = rotate2d(0.000020 * motionScale) * previousP;
    vec2 previousUv = previousP / aspect + 0.5;
    float edge = smoothstep(0.0, 0.06, uv.x) * smoothstep(0.0, 0.06, uv.y)
               * smoothstep(0.0, 0.06, 1.0 - uv.x)
               * smoothstep(0.0, 0.06, 1.0 - uv.y);
    vec3 feedback = texture(previousFrame, clamp(previousUv, 0.001, 0.999)).rgb
                  * mix(0.825, 0.915, harmonic) * edge;

    float stars = 0.0;
    float kickStars = 0.0;
    float links = 0.0;
    float snareLinks = 0.0;
    float travelers = 0.0;
    float hatSparks = 0.0;
    for (int index = 0; index < 12; ++index) {
        float fi = float(index);
        vec2 a = starPosition(fi);
        float bi = mod(fi + 3.0 + floor(fi / 4.0), 12.0);
        vec2 b = starPosition(bi);
        float aTension = 0.025 + 0.075 * sin(fi * 2.39996);
        float bTension = 0.025 + 0.075 * sin(bi * 2.39996);
        float kickRole = 1.0 - step(4.0, fi);
        float snareRole = step(4.0, fi) * (1.0 - step(8.0, fi));
        float hatRole = step(8.0, fi);
        float anticipation = beatAnticipation * clockConfidence;
        a *= 1.0 + kick * aTension * kickRole
           + anticipation * aTension * 0.16;
        b *= 1.0 + kick * bTension * kickRole
           + anticipation * bTension * 0.16;
        a.x += stereoWidth * 0.035 * sign(a.x);
        b.x += stereoWidth * 0.035 * sign(b.x);
        a += snare * snareRole * 0.100 * vec2(-a.y, a.x);
        b += snare * snareRole * 0.100 * vec2(b.y, -b.x);
        float starRadius = 0.009 + 0.0025 * spectrumLevel[index * 2]
                         + 0.0015 * beatPulse
                         + 0.052 * kick * kickRole
                           * (0.5 + 0.5 * sin(fi))
                         + 0.018 * hat * hatRole
                           * (0.5 + 0.5 * sin(fi * 2.1));
        float star = 1.0 - smoothstep(
            starRadius, starRadius * 2.2, length(p - a));
        stars += star;
        kickStars += star * kickRole;
        float connection = line(segmentDistance(p, a, b),
                                0.0035 + 0.001 * harmonic
                                + 0.0015 * beatPulse);
        links = max(links, connection);
        snareLinks = max(snareLinks, connection * snareRole);
        float travel = fract(flowTime * (0.18 + 0.22 * drive) + fi * 0.137 + beatPhase);
        vec2 node = mix(a, b, travel);
        travelers += (1.0 - smoothstep(0.006, 0.019, length(p - node)))
                   * hat * hatRole;
        float sparkAngle = fi * 2.39996 + flowTime * 0.45;
        vec2 sparkPosition = a + 0.022 * vec2(cos(sparkAngle), sin(sparkAngle));
        float spark = 1.0 - smoothstep(0.003, 0.012, length(p - sparkPosition));
        float dashedLink = connection
            * smoothstep(0.42, 0.80, sin(dot(p, normalize(b - a)) * 95.0
                                       - flowTime * 5.0 + fi));
        hatSparks += hat * hatRole * (spark + 0.34 * dashedLink);
    }
    float anchor = line(length(p) - (0.055 + 0.012 * beatPulse
                           + 0.095 * kick), 0.008 + 0.003 * beatPulse);
    float beatWave = line(length(p) - mix(0.12, 0.58,
                          1.0 - clamp(beatPulse, 0.0, 1.0)), 0.010)
                   * beatPulse * clockConfidence;
    float downbeatPulse = line(length(p) - mix(0.07, 0.70, beatPhase), 0.009)
                        * downbeat * clockConfidence;
    float sectionLink = line(abs(p.x + p.y) - 0.22 * section, 0.010) * section;
    float nebula = line(sin(p.x * 8.0 + p.y * 11.0 + flowTime * 0.045), 0.25)
                 * harmonic * 0.13;

    vec3 primary = palettePrimary(3.28);
    vec3 secondary = paletteSecondary(3.28);
    vec3 accent = paletteAccent(3.28);
    vec3 injection = primary * stars
                     * (0.13 + 0.07 * spectralCentroid)
                   + primary * kickStars * 0.34 * kick
                   + secondary * links * (0.055 + 0.075 * harmonic)
                   + secondary * snareLinks * 0.60 * snare
                   + accent * (6.2 * travelers + 8.4 * hatSparks
                               + anchor + beatWave
                               + downbeatPulse + sectionLink) * 0.30
                   + mix(primary, secondary, 0.5) * nebula * 0.07;
    injection *= 1.0 - 0.62 * release;
    vec3 result = feedback + injection;
    result = max(result - vec3(0.0048), vec3(0.0));
    color = vec4(result, 1.0);
}
