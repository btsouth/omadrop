// Shared full-frame field construction. Spatial phases are fixed: the field
// changes only when measured bands, spectra or damped impacts change.
// This is analytic domain warping, not a time-stepped fluid simulation.
vec2 fieldWarp(vec2 p, float low, float mid, float detail) {
    vec2 q=p;
    q+=0.38*vec2(sin(p.y*1.7+p.x*0.4),cos(p.x*1.5-p.y*0.5));
    q+=motionScale*vec2(
        low*0.36*sin(p.y*1.35+0.6)+impactMotion.x*0.24*cos(p.y*1.8-p.x)*(0.55+0.45*sin(p.x+0.8)),
        mid*0.22*sin(p.x*2.2-0.4)+impactMotion.y*0.08*sin(p.x*3.1+p.y)*(0.55-0.45*sin(p.x+0.8)));
    q+=0.19*vec2(sin(q.y*3.4+q.x),cos(q.x*3.0-q.y));
    q+=motionScale*detail*0.05*vec2(sin(q.y*7.0),cos(q.x*6.0));
    return q;
}
float fieldRidge(float phase, float sharpness) {
    // Suppress features smaller than a pixel rather than allowing shimmer.
    float footprint=fwidth(phase);
    return pow(0.5+0.5*cos(phase),sharpness)
        *(1.0-smoothstep(0.6,2.5,footprint));
}
vec3 fieldPalette(float t) {
    // Deep indigo, petrol, warm copper and icy highlights.
    return 0.48+0.46*cos(t+vec3(0.25,1.75,3.45));
}
