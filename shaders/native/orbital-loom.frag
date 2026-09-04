#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 loomCenter = vec2(-0.035 + 0.012 * sin(phrasePhase * tau), 0.015);
    vec2 p = (uv - 0.5) * aspect - loomCenter;
    float radius = max(0.002, length(p));
    float angle = atan(p.y, p.x);

    vec2 previousP = p;
    previousP.x -= (0.00016 + 0.00046 * drive) * motionScale;
    previousP.y += sin(p.x * 8.0 + flowTime * 0.24)
                 * (0.00028 * motionScale + 0.0012 * hat);
    vec2 previousUv = (previousP + loomCenter) / aspect + 0.5;
    float edge = smoothstep(0.0, 0.07, uv.x) * smoothstep(0.0, 0.07, uv.y)
               * smoothstep(0.0, 0.07, 1.0 - uv.x)
               * smoothstep(0.0, 0.07, 1.0 - uv.y);
    vec3 feedback = texture(previousFrame, clamp(previousUv, 0.001, 0.999)).rgb
                  * mix(0.875, 0.945, harmonic) * edge;

    float threads = 0.0;
    float kickThreads = 0.0;
    float snareThreads = 0.0;
    float airThreads = 0.0;
    float crossings = 0.0;
    for (int index = 0; index < 6; ++index) {
        float fi = float(index);
        float threadGesture = 0.30 + 0.70
                            * (0.5 + 0.5 * sin(fi * 2.3 + barPhase * tau));
        float kickThread = index < 2 ? kick : 0.0;
        float snareThread = index >= 2 && index < 4 ? snare : 0.0;
        float gestureDirection = mod(fi, 2.0) < 0.5 ? -1.0 : 1.0;
        float tilt = -0.48 + fi * 0.19 + phrasePhase * 0.12
                   + gestureDirection * (0.16 * kickThread
                     + 0.42 * snareThread
                     + (index < 2 ? 0.12 * beatPulse : 0.0));
        vec2 threadCenter = vec2(0.026 * sin(fi * 1.7),
                                 (fi - 2.5) * 0.035);
        vec2 q = rotate2d(tilt) * (p - threadCenter);
        q.y /= 0.135 + 0.022 * sin(fi * 2.2 + flowTime * 0.13);
        q.x /= 0.53 + 0.055 * development
              + 0.012 * threadGesture * kickThread;
        float ellipse = abs(length(q) - 1.0);
        float strand = line(ellipse, 0.009 + 0.0012 * bandLevel[index]);
        threads = max(threads, strand);
        if (index < 2) kickThreads = max(kickThreads, strand);
        else if (index < 4) snareThreads = max(snareThreads, strand);
        else airThreads = max(airThreads, strand);
        crossings += strand * line(
            sin(p.x * 19.0 + fi + flowTime * 0.18), 0.08);
    }
    float rails = line(abs(p.y) - (0.052 + 0.016 * kick), 0.008)
                * smoothstep(0.62, 0.14, abs(p.x));
    float shuttlePhase = fract(angle / tau + flowTime * (0.42 + 0.3 * drive)
                             + beatPhase);
    float shuttles = line(shuttlePhase - 0.5, 0.035) * threads * hat;
    float kickKnot = (line(length(p - vec2(-0.24, 0.015))
                           - (0.028 + 0.018 * kick), 0.009)
                     + line(length(p - vec2(0.28, -0.035))
                           - (0.021 + 0.012 * kick), 0.008)) * kick;
    float snareHeddles = line(
        sin(p.x * 22.0 + phrasePhase * tau + snare * 2.0), 0.034)
        * smoothstep(0.42, 0.10, abs(p.x))
        * smoothstep(0.31, 0.07, abs(p.y)) * snare;
    float hatGlints = line(sin(angle * 38.0 - beatPhase * tau), 0.026)
        * threads * hat;
    float onsetCut = line(abs(p.x + 0.34 - 0.11 * onsetPulse), 0.008)
                   * smoothstep(0.28, 0.04, abs(p.y)) * onsetPulse;
    float beatShuttle = line(fract(angle / tau - beatPhase + 0.5) - 0.5,
                                 0.055)
                      * threads * beatPulse * clockConfidence;
    float anticipationShuttle = line(
        abs(p.x - mix(-0.42, 0.42, beatAnticipation)), 0.008)
        * smoothstep(0.18, 0.025, abs(p.y + 0.20))
        * beatAnticipation;
    float downbeatPin = line(length(p - vec2(0.34, 0.12))
                             - 0.025, 0.010)
                      * downbeat * clockConfidence;
    float sectionKnot = line(abs(p.x * p.y) - 0.12 * section, 0.012) * section;
    float medium = line(sin(p.x * 13.0 + p.y * 5.0
                            + flowTime * 0.30), 0.22)
                 * harmonic * smoothstep(0.72, 0.16, radius) * 0.18;

    vec3 primary = palettePrimary(1.86);
    vec3 secondary = paletteSecondary(1.86);
    vec3 accent = paletteAccent(1.86);
    float threadLight = 0.12 + 0.055 * energySlow;
    vec3 injection = primary * kickThreads * threadLight
                   + secondary * snareThreads
                         * (threadLight + 0.08 * snare)
                   + mix(primary, secondary, 0.58) * airThreads * threadLight
                   + secondary * crossings * 0.10
                   + primary * rails * 0.12
                   + accent * (shuttles + downbeatPin + sectionKnot) * 0.22
                   + accent * beatShuttle * 0.24
                   + mix(accent, vec3(1.0), 0.24)
                         * anticipationShuttle * 0.18
                   + mix(accent, vec3(1.0), 0.32) * kickKnot * 0.18
                   + mix(secondary, vec3(1.0), 0.40) * snareHeddles * 0.27
                   + mix(primary, vec3(1.0), 0.52) * hatGlints * 0.34
                   + mix(accent, vec3(1.0), 0.46) * onsetCut * 0.36
                   + primary * medium * 0.09;
    injection *= 1.0 - 0.54 * release;
    vec3 result = feedback + injection;
    result = max(result - vec3(0.0043), vec3(0.0));
    color = vec4(result, 1.0);
}
