#version 330 core

in vec2 uv;
out vec4 color;

#include "scene-uniforms.glsl"

void main() {
    vec2 aspect = vec2(resolution.x / max(1.0, resolution.y), 1.0);
    vec2 p = (uv - 0.5) * aspect;
    p.x -= stereoWidth * 0.035 * sin(p.y * 3.0 + flowTime * 0.15);

    vec2 previousP = p;
    previousP.y += 0.00012 + 0.00025 * energySlow;
    float floorRegion = smoothstep(-0.04, -0.50, p.y);
    previousP.x *= 1.0 - 0.013 * kick * floorRegion
                         + 0.0012 * beatAnticipation;
    vec2 previousUv = previousP / aspect + 0.5;
    float edge = smoothstep(0.0, 0.07, uv.x) * smoothstep(0.0, 0.07, uv.y)
               * smoothstep(0.0, 0.07, 1.0 - uv.x)
               * smoothstep(0.0, 0.07, 1.0 - uv.y);
    vec3 feedback = texture(previousFrame, clamp(previousUv, 0.001, 0.999)).rgb
                  * mix(0.91, 0.956, harmonic) * edge;

    float architecture = 0.0;
    float distantArchitecture = 0.0;
    float windows = 0.0;
    for (int index = 0; index < 5; ++index) {
        float fi = float(index);
        float depth = fi / 4.0;
        float kickReach = kick * smoothstep(0.22, 0.92, depth)
                        * (0.72 + 0.28 * sin(fi * 2.1 + barPhase * tau));
        float width = mix(0.18, 0.60, depth) + 0.060 * kickReach;
        float roof = mix(0.13, 0.43, depth) + 0.055 * development
                   + 0.012 * downbeat;
        vec2 q = p;
        q.x -= snare * (0.025 + 0.045 * depth)
             * sin(fi * 1.8 + barPhase * tau);
        float archRadius = length(vec2(q.x / width, (q.y - roof * 0.28) / roof));
        float arch = line(archRadius - 1.0,
                          0.017 + 0.007 * bandLevel[index]);
        arch *= smoothstep(-0.45, -0.12, q.y);
        float pillars = line(abs(q.x) - width, 0.009 + 0.005 * bandLevel[2]);
        pillars *= smoothstep(roof * 0.25, -0.52, q.y);
        float structure = max(arch, pillars);
        float layerWeight = mix(1.0, 0.18, depth);
        architecture = max(architecture, structure * layerWeight);
        distantArchitecture = max(distantArchitecture,
                                  structure * depth * 0.38);
        windows += arch * line(sin(atan(q.y, q.x) * 18.0 + flowTime), 0.08) * hat;
    }
    float aisle = line(abs(p.x) - (0.05 + 0.34 * (p.y + 0.5)), 0.009)
                * smoothstep(0.52, -0.22, p.y);
    float floorBars = line(sin((0.15 / max(0.04, p.y + 0.57)
                              + flowTime * 0.25 + 1.20 * beatPulse
                              + kick) * tau), 0.10)
                    * smoothstep(-0.08, -0.48, p.y);
    float downbeatArch = line(length(p / vec2(0.38, 0.28))
                            - mix(0.35, 1.0, beatPhase), 0.014)
                       * downbeat * clockConfidence;
    float beatArch = line(length(p / vec2(0.52, 0.38))
                          - mix(0.34, 1.0,
                            1.0 - clamp(beatPulse, 0.0, 1.0)), 0.018)
                   * beatPulse * clockConfidence;
    vec2 roseP = p - vec2(0.0, 0.18);
    float roseRadius = length(roseP);
    float roseWindow = line(roseRadius - 0.13, 0.010)
                     + line(sin(atan(roseP.y, roseP.x) * 8.0
                                + tonalMotion * 1.8), 0.045)
                       * smoothstep(0.125, 0.03, roseRadius);
    roseWindow *= 0.22 + 0.34 * harmonic;
    float sectionRose = line(roseRadius - 0.13, 0.018) * section;
    float ambience = line(sin(p.y * 10.0 + p.x * 4.0 - flowTime * 0.18), 0.24)
                   * harmonic * 0.16;

    vec3 primary = palettePrimary(2.82);
    vec3 secondary = paletteSecondary(2.82);
    vec3 accent = paletteAccent(2.82);
    vec3 injection = primary * architecture * (0.14 + 0.08 * harmonic)
                   + secondary * distantArchitecture * 0.040
                   + secondary * (aisle + floorBars) * (0.09 + 0.06 * energySlow)
                   + accent * windows * (0.34 + 0.28 * hat)
                   + mix(primary, accent, 0.38) * roseWindow * 0.15
                   + accent * (beatArch + downbeatArch + sectionRose) * 0.24
                   + mix(primary, secondary, 0.5) * ambience * 0.08;
    injection *= 1.0 - 0.56 * release;
    float compositionMask = mix(1.0, 0.72,
        smoothstep(0.34, 0.82, length(p)));
    vec3 result = (feedback + injection) * compositionMask;
    result = max(result - vec3(0.0043), vec3(0.0));
    color = vec4(result, 1.0);
}
