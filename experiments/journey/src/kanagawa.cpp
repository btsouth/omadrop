// Kanagawa: a dawn woodblock print with a red sun. The swell that carries
// the six lines heaves up into the great wave, grows claws on onsets, flings
// spray that falls like snow across Fuji, and breaks on the surge.
#include "parts.h"
#include "rig.h"

#include <cmath>
#include <vector>

namespace Journey {
namespace {
const Col KINK = hex(0x14141c), DEEP = hex(0x1b2740), WAVE = hex(0x223249), MID = hex(0x39558a);
const Col BLUE = hex(0x7e9cd8), SPRING = hex(0x7fb4ca), PAPER = hex(0xdcd7ba), KRED = hex(0xc34043);
const Col HOT = hex(0xe82424), GOLD = hex(0xe6c384), OCHRE = hex(0xc0a36e), ORANGE = hex(0xc17158);
const Col VIOLET = hex(0x957fb8);
const Col OINK(0.012f, 0.030f, 0.024f), WARM_T(0.99f, 0.78f, 0.42f), WARM_B(0.98f, 0.90f, 0.66f);
constexpr double HZ = 612;

// Causal energy controls: water has inertia. Averaging measured band energy
// over overlapping windows lets a phrase change a swell without kicking the
// entire sea upward on every bass attack. The analyser and Score stay intact.
double fluidBand(const Ctx& c, int band, double t) {
    if (!c.score) return 0;
    auto mean = [&](double span) {
        return (c.score->integral(band, t) - c.score->integral(band, t - span)) / span;
    };
    return 0.20 * mean(0.4) + 0.50 * mean(0.9) + 0.30 * mean(1.6);
}

double fluidBand(const Ctx& c, int band) { return fluidBand(c, band, c.t); }

double plungeDuration(const Ctx& c) {
    const double release = kanagawaBreak(c).t;
    if (c.score) for (const Event& e : c.score->bassHits) {
        if (e.t >= release + 0.45 && e.t < release + 0.95 && e.strength >= 0.35)
            return e.t - release;
    }
    return 0.60;
}

const char* kanagawaSkyFs = R"(
uniform float u_grade;
vec3 ramp(float l) {
    vec3 c0 = vec3(0.008, 0.026, 0.022), c1 = vec3(0.022, 0.150, 0.112), c2 = vec3(0.110, 0.520, 0.370);
    vec3 c3 = vec3(0.42, 0.93, 0.68), c4 = vec3(0.97, 0.91, 0.70);
    if (l < 0.22) return mix(c0, c1, l / 0.22);
    if (l < 0.5) return mix(c1, c2, (l - 0.22) / 0.28);
    if (l < 0.78) return mix(c2, c3, (l - 0.5) / 0.28);
    return mix(c3, c4, (l - 0.78) / 0.22);
}
void main() {
    float y = design().y;
    vec3 s0 = vec3(0.071, 0.071, 0.102), s1 = vec3(0.110, 0.125, 0.200), s2 = vec3(0.200, 0.216, 0.353);
    vec3 s3 = vec3(0.478, 0.353, 0.471), s4 = vec3(0.757, 0.443, 0.345), s5 = vec3(0.902, 0.765, 0.518);
    vec3 c;
    if (y < 190.0) c = mix(s0, s1, y / 190.0);
    else if (y < 380.0) c = mix(s1, s2, (y - 190.0) / 190.0);
    else if (y < 500.0) c = mix(s2, s3, (y - 380.0) / 120.0);
    else if (y < 570.0) c = mix(s3, s4, (y - 500.0) / 70.0);
    else if (y < 612.0) c = mix(s4, s5, (y - 570.0) / 42.0);
    else c = s5;
    float l = clamp(dot(c, vec3(0.30, 0.56, 0.14)) * 1.25, 0.0, 1.0);
    o = vec4(mix(ramp(l), c, u_grade), 1.0);
}
)";

const char* gradeCompositeFs = R"(
uniform sampler2D u_tex;
uniform float u_grade, u_opacity;
vec3 ramp(float l) {
    vec3 c0 = vec3(0.008, 0.026, 0.022), c1 = vec3(0.022, 0.150, 0.112), c2 = vec3(0.110, 0.520, 0.370);
    vec3 c3 = vec3(0.42, 0.93, 0.68), c4 = vec3(0.97, 0.91, 0.70);
    if (l < 0.22) return mix(c0, c1, l / 0.22);
    if (l < 0.5) return mix(c1, c2, (l - 0.22) / 0.28);
    if (l < 0.78) return mix(c2, c3, (l - 0.5) / 0.28);
    return mix(c3, c4, (l - 0.78) / 0.22);
}
void main() {
    vec4 s = texture(u_tex, v_uv);
    vec3 c = s.rgb / max(s.a, 1e-4);
    float l = clamp(dot(c, vec3(0.30, 0.56, 0.14)) * 1.25, 0.0, 1.0);
    o = vec4(mix(ramp(l), c, u_grade) * s.a, s.a) * u_opacity;
}
)";

std::vector<V2> catmull(const std::vector<V2>& pts, int n) {
    std::vector<V2> P;
    P.push_back(pts[0] * 2 - pts[1]);
    for (const V2& p : pts) P.push_back(p);
    P.push_back(pts.back() * 2 - pts[pts.size() - 2]);
    std::vector<V2> out;
    for (std::size_t i = 1; i + 2 < P.size(); ++i) {
        const V2 p0 = P[i - 1], p1 = P[i], p2 = P[i + 1], p3 = P[i + 2];
        for (int k = 0; k < n; ++k) {
            const double s = double(k) / n;
            out.push_back((p1 * 2 + (p2 - p0) * s + (p0 * 2 - p1 * 5 + p2 * 4 - p3) * (s * s) + (p1 * 3 - p0 - p2 * 3 + p3) * (s * s * s)) * 0.5);
        }
    }
    out.push_back(P[P.size() - 2]);
    return out;
}

struct WaveFrame {
    std::vector<V2> S, d, n;
    std::vector<double> u, w;
    V2 at(int i, double k) const { return S[std::size_t(i)] + n[std::size_t(i)] * (w[std::size_t(i)] * k); }
    int N() const { return int(S.size()); }
};

// The wave spine from flat swell (rise 0) to the towering crest (rise 1).
// `fall` throws the lip forward and down when it breaks.
WaveFrame waveFrame(double t, double r, double bass, double fall, double collapse, double lean = 0, double kick = 0, double duration = .6) {
    static const V2 base[10] = {{-260, 1120}, {40, 930}, {250, 720}, {400, 520}, {520, 372}, {650, 286}, {775, 262}, {885, 288}, {958, 350}, {992, 428}};
    static const V2 flat[10] = {{-260, 1120}, {40, 1010}, {250, 940}, {420, 900}, {560, 880}, {700, 872}, {830, 870}, {1100, 882}, {1460, 890}, {1960, 920}};
    std::vector<V2> pts;
    const double rr = r;
    for (int i = 0; i < 10; ++i) {
        V2 p = base[i] * rr + flat[i] * (1 - rr);
        // The swell rolls up from below the frame as Kanagawa arrives (no pop at the handover).
        p.y += 210 * (1 - sstep(47.9, 51.0, t)) * (1 - rr);
        p.y -= 12 * std::sin(t * 0.9 + i * 0.5) * (i / 9.0);
        const double heave = t >= 52 ? 72 : 16;
        p.y -= bass * heave * rr * sstep(3, 7, i) + kick * 22 * rr * sstep(3, 8, i);
        if (lean > 0) {
            // Towering: the crest climbs and reaches out over the boats.
            const double k = sstep(3, 9, i);
            p = p + V2(230 * k * k * lean, -(110 * k - 70 * k * k * k) * lean);
        }
        if (fall > 0) {
            // The crown remains supported while the lip accelerates toward
            // one physical contact point. The shoulder follows after impact.
            static const V2 landed[10] = {{-260,1120},{40,930},{250,730},{435,585},{590,420},
                {760,410},{960,500},{1110,675},{1250,810},{1305,850}};
            const double plunge = std::min(1.0,fall) * sstep(3, 7, i);
            const V2 velocity=(landed[i]-p)*(2/duration)*sstep(3,7,i);
            p = lerp(p, landed[i], plunge);
            if (i >= 7 && fall>1) {
                const double after=fall-1;
                p.x+=velocity.x*.20*(1-std::exp(-after/.20));
                p.y+=velocity.y*.05*(1-std::exp(-after/.05));
            }
        }
        if (collapse > 0) p = lerp(p, flat[i], collapse);
        if(i==9 && t>68.5) p.x+=460*sstep(68.5,77,t)*(1-rr*(1-collapse));
        pts.push_back(p);
    }
    WaveFrame f;
    f.S = catmull(pts, 36);
    const int N = f.N();
    f.d.resize(std::size_t(N)); f.n.resize(std::size_t(N)); f.u.resize(std::size_t(N)); f.w.resize(std::size_t(N));
    for (int i = 0; i < N; ++i) {
        const V2 a = f.S[std::size_t(std::max(0, i - 1))], b = f.S[std::size_t(std::min(N - 1, i + 1))];
        V2 d = b - a;
        const double L = std::max(1e-6, d.len());
        d = d * (1 / L);
        f.d[std::size_t(i)] = d;
        f.n[std::size_t(i)] = {-d.y, d.x};
        f.u[std::size_t(i)] = double(i) / (N - 1);
        f.w[std::size_t(i)] = (400 * std::pow(1 - f.u[std::size_t(i)], 1.1) + 22) * (0.35 + 0.65 * rr * (1 - collapse))
            * (1 - sstep(0.92, 1.0, f.u[std::size_t(i)]));
    }
    if (t >= 52) {
        double length = 0;
        for (int i=1;i<N;++i) length += (f.S[i]-f.S[i-1]).len();
        double distance = 0;
        for (int i=0;i<N;++i) {
            if(i) distance += (f.S[i]-f.S[i-1]).len();
            f.u[i] = distance / std::max(1.0,length);
            const double taper = 1-sstep(.94,1.0,f.u[i]);
            f.w[i] = (400*std::pow(1-f.u[i],1.1)+22)*(0.35+0.65*rr*(1-collapse))*taper;
            f.w[i] *= lerp(1.0,.65,sstep(.1,.7,r));
        }
        auto width=f.w;
        for(int i=3;i<N-3;++i) {
            double total=0,weight=0;
            for(int j=std::max(0,i-10);j<=std::min(N-1,i+10);++j) {
                const double a=11-std::abs(i-j); total+=width[j]*a;weight+=a;
            }
            f.w[i]=total/weight;
        }
    }
    return f;
}

void bandPath(Canvas& c, const WaveFrame& f, double k0, double k1, int i0 = 0, int i1 = -1) {
    if (i1 < 0) i1 = f.N();
    for (int i = i0; i < i1; ++i) {
        const V2 p = f.at(i, k0);
        if (i == i0) c.moveTo(p.x, p.y); else c.lineTo(p.x, p.y);
    }
    for (int i = i1 - 1; i >= i0; --i) { const V2 p = f.at(i, k1); c.lineTo(p.x, p.y); }
    c.closePath();
}

// A hooked foam finger: a chain of shrinking discs along a curling path.
V2 claw(Canvas& c, V2 p, double ang, double ln, double r0, double curl, Col col, const Col* out, int n = 13, double alpha = 1) {
    std::vector<std::array<double, 3>> pts;
    double a = ang;
    for (int j = 0; j < n; ++j) {
        const double f = double(j) / (n - 1);
        pts.push_back({p.x, p.y, r0 * std::pow(1 - f, 0.8) + 1.2});
        a += curl / n;
        p = p + V2(std::cos(a), std::sin(a)) * (ln / n);
    }
    if (out) for (auto& q : pts) c.disc(q[0], q[1], q[2] + 1.8, *out,alpha);
    for (auto& q : pts) c.disc(q[0], q[1], q[2], col,alpha);
    return {pts.back()[0], pts.back()[1]};
}

double passingSwell(double x, double t) {
    const double centre = 1700 - 175 * (t - 55.0);
    return 52 * window(t, 55.0, 1.8, 59.6, 2.2) * std::exp(-std::pow((x - centre) / 420, 2));
}

double washHeight(double x,double age);

double rawSwellY(double x, int row, double t, const Ctx* c) {
    static const double rows[7][3] = {{HZ, 5, 0.0}, {640, 9, 0.6}, {676, 13, 1.4}, {724, 20, 2.3}, {790, 30, 3.1}, {874, 42, 4.4}, {980, 56, 5.2}};
    const double f = row / 6.0;
    const double* r = rows[row];
    const int band = std::clamp(5 - int(std::round(f * 5)), 0, 5);
    const double response = c ? 1 + 2.0 * c->lift(band) : 1;
    const double kick = c ? c->kick(5) : 0;
    // A band's lift bends a travelling patch of its own crest, rather than
    // making the whole sea flash. The anchored boats sample this same patch.
    const double packet = c ? 32 * f * c->lift(band) * std::cos(x / (210 + 50 * f) + t * 0.45 + row) : 0;
    const double drawBack = 14 * sstep(61.5, 65.5, t) * f;
    const double ambient = 0.6 * std::sin(x / (90 + 60 * f) + r[2] + t * (0.5 + 0.2 * row))
        + 0.4 * std::sin(x / (37 + 30 * f) + r[2] * 2.1 - t * 0.7);
    const double old = r[0] + drawBack - passingSwell(x, t) * (row == 4 || row == 5 ? 1 : 0.18 * f)
        - 10 * f * kick - packet + r[1] * response * ambient;
    if (t <= T::crossingEnd) return old;
    const double energy = c ? fluidBand(*c, band, t) : 0;
    const double travel = c && c->score ? c->score->integral(band, t) : 0;
    const double longWave = std::sin(x / (155 + 90 * f) + r[2] - t * (0.18 + 0.065 * row) - 2.0 * travel);
    const double detail = std::sin(x / (63 + 45 * f) - t * 0.38 + r[2] * 2.1);
    static const double pressure[7]={0,.1,.25,.38,.57,.80,.30};
    const double wash=c && c->waterContact>=0 ? pressure[row]*washHeight(x,t-c->waterContact) : 0;
    const double natural = r[0] - wash + 12 * sstep(58, 62, t) * f
        - passingSwell(x, t) * (row == 4 || row == 5 ? 1 : 0.18 * f)
        + r[1] * (0.38 + 3.2 * energy) * (0.78 * longWave + 0.22 * detail);
    return lerp(old, natural, sstep(48, 51.8, t));
}

double swellY(double x, int row, double t, const Ctx* c = nullptr) {
    double y = rawSwellY(x, 0, t, c);
    for (int i = 1; i <= row; ++i)
        y = std::max(y + 18 + 3 * i, rawSwellY(x, i, t, c));
    return y;
}

void sea(Ctx& c, double t) {
    Canvas& cv = c.canvas();
    // Resolve ordered depth rows once per x. Re-solving every nearer row's
    // predecessors repeats the same Score lookups and trigonometry.
    std::array<std::vector<V2>, 7> surfaces;
    for (double x = c.cameraX - 20; x <= c.cameraX + 1941; x += 6) {
        double y = rawSwellY(x, 0, t, &c);
        surfaces[0].push_back({x, y});
        for (int i = 1; i < 7; ++i) {
            y = std::max(y + 18 + 3 * i, rawSwellY(x, i, t, &c));
            surfaces[i].push_back({x, y});
        }
    }
    for (int i = 0; i < 7; ++i) {
        const double f = i / 6.0;
        const Col col = mix(mix(MID, GOLD, 0.25 * std::pow(1 - f, 3)), DEEP, f * 0.85);
        const std::vector<V2>& ys = surfaces[i];
        cv.color(col);
        cv.moveTo(c.cameraX - 20, 1080);
        for (const V2& p : ys) cv.lineTo(p.x, p.y);
        cv.lineTo(c.cameraX + 1941, 1080);
        cv.closePath();
        cv.fill();
        // Crest lines brighten with their depth's band: far rows high, near rows low.
        const int band = std::clamp(5 - int(std::round(f * 5)), 0, 5);
        const double fast = 0.50 + 0.7 * c.band(band) + 0.75 * c.lift(band);
        const double crest = (0.75 - 0.2 * f) * lerp(fast, 0.50 + 1.25 * fluidBand(c, band), sstep(48, 51.8, t));
        cv.polyline(ys, 1.2 + 1.4 * f, mix(PAPER, BLUE, 0.3 + 0.3 * f), std::min(1.0, crest));
        for (int k = 1; k < 3; ++k) {
            const int first=t<52 ? 0 : int(std::floor(c.cameraX/270))-1;
            const int count=t<52 ? 8 : 10;
            for (int mark = first; mark < first+count; ++mark) {
                const double x0 = mark * 270 - 160 + 35 * std::sin(t * 0.25 + i + k);
                const double span = 90 + 65 * hash2(mark + i * 8, k);
                for (double x = x0; x < x0 + span; x += 6) {
                    const double u = (x - x0) / span;
                    const double y = swellY(x, i, t, &c) + k * (6 + 16 * f);
                    const double y1 = swellY(x + 6, i, t, &c) + k * (6 + 16 * f);
                    cv.line(x, y, x + 6, y1, 1.0 + f, BLUE, (0.25 - 0.04 * k) * std::sin(Pi * u));
                }
            }
        }
    }
    c.gpu.over(cv);
    Canvas& g = c.canvas();
    Rng rng(5);
    for (int i = 0; i < 90; ++i) {
        const double yy = HZ + 4 + std::pow(rng.uni(), 1.8) * 250;
        const double w = (10 + rng.uni() * 60) * (0.4 + (yy - HZ) / 200);
        const double xx = 1310 + .90*c.cameraX + rng.normal() * (26 + (yy - HZ) * 0.42) + std::sin(t * 2 + i) * 4;
        const bool hot = rng.uni() >= 0.65;
        const double tw = 0.6 + 0.4 * std::sin(t * (1.5 + i % 7 * 0.4) + i);
        const double energy = lerp(0.50 + 0.5 * c.lift(0) + 0.3 * c.kick(5),
            0.50 + 0.65 * fluidBand(c, 0), sstep(48, 51.8, t));
        g.line(xx - w / 2, yy, xx + w / 2, yy, 1.6 + (yy - HZ) / 90, hot ? HOT : GOLD,
            energy * (1 - (yy - HZ) / 300) * tw);
    }
    c.gpu.over(g, 1.15f);
    if (c.score && t >= 46.0 && t < 52.0) {
        Canvas& spray = c.canvas();
        for (auto it = c.score->onsets.rbegin(); it != c.score->onsets.rend(); ++it) {
            const double age = t - it->t;
            if (age < 0) continue;
            if (age > 1.4 || it->t < 46.0) break;
            const int event = int(&*it - c.score->onsets.data());
            const double x = 520 + 1320 * hash2(event, 18 + c.seed);
            const int row = 3 + event % 3;
            const double y = swellY(x, row, it->t);
            for (int j = 0; j < 16; ++j) {
                const double speed = 45 + 80 * hash2(event * 17 + j, 3);
                const double side = (hash2(event * 17 + j, 7) - 0.4) * 70;
                const V2 p(x + side * age, y - speed * age + 95 * age * age);
                spray.disc(p.x, p.y, (1.4 + 1.9 * hash2(j, event + 5)) * (1 - age / 1.4),
                           PAPER, (0.35 + 0.60 * it->strength) * (1 - age / 1.4));
            }
        }
        c.gpu.over(spray, 1, 0, float(1 - sstep(48, 52, t)));
    }
}

void mistBars(Ctx& c, double t) {
    Canvas& cv = c.canvas();
    const double bars[6][5] = {{1320, 430, 520, 20, 0.34}, {1500, 462, 600, 16, 0.28}, {700, 470, 380, 14, 0.2},
                               {1040, 548, 300, 12, 0.3}, {1560, 560, 460, 14, 0.36}, {860, 566, 240, 10, 0.26}};
    for (int i = 0; i < 6; ++i) {
        const double x = bars[i][0] + std::sin(t * 0.05 + i) * 30 - (t - T::crossingEnd) * (3 + i);
        const double y = bars[i][1], width = bars[i][2], h = bars[i][3] * 0.5;
        cv.color(PAPER, bars[i][4] * 0.7);
        cv.moveTo(x, y);
        cv.curveTo(x + width * 0.25, y - h, x + width * 0.70, y - h * 0.35, x + width, y);
        cv.curveTo(x + width * 0.72, y + h * 0.35, x + width * 0.25, y + h, x, y);
        cv.closePath(); cv.fill();
    }
    c.gpu.over(cv);
}
}  // namespace

Col duotone(Col c, double g) {
    const double l = clamp01((c.r * 0.30 + c.g * 0.56 + c.b * 0.14) * 1.25);
    const double st[5] = {0, 0.22, 0.5, 0.78, 1.0};
    const Col cs[5] = {Col(0.008f, 0.026f, 0.022f), Col(0.022f, 0.150f, 0.112f), Col(0.110f, 0.520f, 0.370f), Col(0.42f, 0.93f, 0.68f), Col(0.97f, 0.91f, 0.70f)};
    Col r = cs[4];
    for (int i = 0; i < 4; ++i) {
        if (l <= st[i + 1]) { r = mix(cs[i], cs[i + 1], (l - st[i]) / (st[i + 1] - st[i])); break; }
    }
    return mix(r, c, g);
}

double rowingClock(const Ctx& c) {
    const double measured = c.score ? c.score->integral(0, c.t) - c.score->integral(0, 35.4) : 0;
    return 0.40 * (c.t - 35.4) + 0.90 * measured;
}

double seaStrandY(const Ctx& c, double x, int band) {
    return swellY(x, 1 + band, c.t, &c) + 14 + 3 * band;
}

std::vector<V2> resampleStrand(const std::vector<V2>& p, int n) {
    std::vector<double> acc{0};
    for (std::size_t i = 1; i < p.size(); ++i) acc.push_back(acc.back() + (p[i] - p[i - 1]).len());
    std::vector<V2> out;
    const double L = acc.back();
    std::size_t j = 0;
    for (int k = 0; k < n; ++k) {
        const double s = L * k / (n - 1);
        while (j + 2 < acc.size() && acc[j + 1] < s) ++j;
        const double f = (s - acc[j]) / std::max(1e-9, acc[j + 1] - acc[j]);
        out.push_back(lerp(p[j], p[j + 1], clamp01(f)));
    }
    return out;
}

double strandClock(const Ctx& c, int band) {
    return (0.07 + 0.03 * band) * (0.6 * c.t + 5.0 * (c.score ? c.score->integral(band, c.t) : 0));
}

BoatPose seaBoatPose(const Ctx& c, bool near) {
    const double t = c.t;
    const double travel = std::max(0.0, t - T::crossingEnd);
    const double x = near ? 1250 + travel * 1.2 : 1620 + 340 * (1 - easeOut(travel / 7)) + travel * 2;
    auto surface = [&](double at) {
        return near ? lerp(swellY(at, 4, t, &c), swellY(at, 5, t, &c), 0.34)
                    : lerp(swellY(at, 2, t, &c), swellY(at, 3, t, &c), 0.40);
    };
    // Average the support under a long hull, with the same swell for both
    // camera sections. Boats follow the plane instead of bobbing through it.
    const double half = near ? 140 : 85;
    const double left = surface(x - half), right = surface(x + half);
    const double y = (left + 2 * surface(x) + right) * 0.25 - 9;
    const double tilt = std::clamp(std::atan2(right - left, 2 * half), -0.13, 0.13);
    return {{x, y}, tilt};
}

void drawBoat(Ctx& c, Canvas& cv, Canvas& light, V2 at, double ln, double tilt, int crew, double style, double sc,
              bool bearer, double stroke, double bearerAt, double bearerH, double bearerWalk, bool showBearer, double moored) {
    const double t = c.t;
    const bool drama = t >= 52;
    const double release = drama ? kanagawaBreak(c).t : 74.2;
    const double age = t - release;
    const double brace = drama ? window(t, release - 1.8, 0.9, release + plungeDuration(c) + 2.6, 1.2) : 0;
    const double slip = drama ? window(age - plungeDuration(c), 0.05, 0.28, 3.1, 1.5) : 0;
    const double check = drama ? window(age - plungeDuration(c), 1.7, 0.5, 3.4, 0.7) : 0;
    if (drama) {
        Ctx rest = c; rest.t = release - 0.9;
        const double frozen = rowingClock(rest);
        Ctx resume = c; resume.t = release + plungeDuration(c) + 2.3;
        const double recovered = frozen + rowingClock(c) - rowingClock(resume);
        stroke = lerp(stroke, frozen, sstep(release - 1.8, release - 0.9, t));
        stroke = lerp(stroke, recovered, sstep(2.3, 3.7, age - plungeDuration(c)));
    }
    if (moored > 0) {
        Ctx hold=c; hold.t=72.8;
        double stopped=rowingClock(hold);
        if(drama) {
            Ctx rest=c;rest.t=release-.9;
            Ctx resume=c;resume.t=release+plungeDuration(c)+2.3;
            stopped=rowingClock(rest)+rowingClock(hold)-rowingClock(resume);
        }
        stroke=lerp(stroke,stopped,moored);
    }
    const double readable = sstep(34.0, 42.0, t);
    const Col hullOut = mix(OINK, WAVE, style), hull = mix(mix(OINK, WARM_T * 0.28f, readable), OCHRE, style);
    const Col body = mix(OINK, mix(WAVE, KINK, 0.3), style), oar = mix(OINK, OCHRE, style);
    cv.save();
    cv.translate(at.x, at.y);
    cv.rotate(tilt);
    cv.color(hullOut);
    cv.moveTo(-ln / 2 - 26 * sc, -16 * sc); cv.curveTo(-ln / 4, 14 * sc, ln / 4, 14 * sc, ln / 2 + 12 * sc, -8 * sc);
    cv.lineTo(ln / 2 - 4 * sc, 10 * sc); cv.curveTo(ln / 4, 26 * sc, -ln / 4, 26 * sc, -ln / 2, 6 * sc); cv.closePath();
    cv.fill();
    // A distinct gunwale and side keep the shallow fishing hull above water.
    cv.color(mix(WARM_T * 0.32f, OCHRE, style), 0.85);
    cv.moveTo(-ln / 2 - 20 * sc, -13 * sc);
    cv.curveTo(-ln / 4, 12 * sc, ln / 4, 12 * sc, ln / 2 + 6 * sc, -6 * sc);
    cv.stroke(2.6 * sc);
    cv.color(hull);
    cv.moveTo(-ln / 2 - 20 * sc, -13 * sc); cv.curveTo(-ln / 4, 12 * sc, ln / 4, 12 * sc, ln / 2 + 6 * sc, -6 * sc);
    cv.lineTo(ln / 2 - 6 * sc, 6 * sc); cv.curveTo(ln / 4, 20 * sc, -ln / 4, 20 * sc, -ln / 2, 3 * sc); cv.closePath();
    cv.fill();
    // Bent arms and planted seated feet: recovery, catch, drive, settle.
    const double lookout = window(t, 58.0, 0.7, 61.0, 0.9) * double(drama);
    std::vector<std::array<V2, 2>> oars;
    for (int i = 0; i < crew; ++i) {
        const double f = (i + 0.5) / crew;
        const double bx = -ln / 2 + ln * (0.12 + (bearer ? 0.62 : 0.72) * f), by = 2 * sc + 8 * sc * std::sin(f * Pi);
        const double phase = stroke - i * 0.025;
        const double drive = std::sin(Tau * phase), dip = std::max(0.0, std::sin(Tau * phase + 0.6));
        RigIn r;
        r.h = 62 * sc; r.facing = 1; r.hair = Hair::Short; r.garment = Garment::Jacket;
        r.hip = {bx, by - 13 * sc + 5 * sc * brace};
        r.lean = lerp(0.18 + 0.30 * drive - 0.16 * lookout, 1.05, brace);
        r.footF = {bx + 21 * sc, by + 3 * sc}; r.footB = {bx + 7 * sc, by + 5 * sc};
        V2 wanted(bx + (17 + 10 * drive) * sc, by - (16 - 2 * drive) * sc);
        wanted = lerp(wanted, {bx + 24 * sc, by - 8 * sc}, brace);
        const V2 shoulder = r.hip + V2(std::sin(r.lean), -std::cos(r.lean)) * (0.275 * r.h);
        const V2 reach = wanted - shoulder;
        const V2 grip = shoulder + reach * std::min(1.0, 0.29 * r.h / std::max(1e-6, reach.len()));
        r.handF = grip; r.handB = grip + V2(-5 * sc, 1 * sc);
        r.headTilt = 0.10 * drive + 0.30 * lookout;
        if (bearer && i == crew - 1) {
            r.headTurn = check;
            r.headTilt += 0.25 * check;
            r.handB = lerp(r.handB, r.hip + V2(20 * sc, -23 * sc), check);
        }
        if (drama) r.flutter = 0.35 * brace * std::sin(t * 6 + i);
        const Body pose = solve(r);
        drawBody(cv, pose, body);
        cv.line(pose.shoulder.x, pose.shoulder.y + 2 * sc, pose.chest.x, pose.chest.y + 10 * sc,
                4 * sc, mix(WARM_B, PAPER, style), 0.35 + 0.35 * style);
        const V2 blade = grip + lerp(V2((23 + 18 * drive) * sc, (34 + 18 * dip) * sc),V2(42*sc,6*sc),moored);
        oars.push_back({grip, blade});
        if (t > 35.4) {
            const double ca = std::cos(tilt), sa = std::sin(tilt);
            const V2 water = at + V2(blade.x * ca - blade.y * sa, blade.x * sa + blade.y * ca);
            const double ring = std::fmod(phase + 1000.0, 1.0);
            light.color(mix(Col(0.62f, 0.95f, 0.78f), PAPER, style), (1-moored)*(0.18 + 0.25 * c.lift(0)) * (1 - ring) * sstep(0.05, 0.35, dip));
            light.ellipse(water.x, water.y, (7 + 24 * ring) * sc, (1.5 + 4 * ring) * sc);
            light.stroke(1.1 * sc);
        }
    }
    // The near side contains the seated knees/feet. Hands and oars stay in
    // front of it: redraw their shafts below after the hull face.
    cv.color(hullOut);
    cv.moveTo(-ln / 2 - 20 * sc, -10 * sc);
    cv.curveTo(-ln / 4, 13 * sc, ln / 4, 13 * sc, ln / 2 + 8 * sc, -5 * sc);
    cv.lineTo(ln / 2 - 4 * sc, 10 * sc);
    cv.curveTo(ln / 4, 26 * sc, -ln / 4, 26 * sc, -ln / 2, 6 * sc); cv.closePath(); cv.fill();
    cv.color(hull);
    cv.moveTo(-ln / 2 - 18 * sc, -7 * sc);
    cv.curveTo(-ln / 4, 14 * sc, ln / 4, 14 * sc, ln / 2 + 4 * sc, -3 * sc);
    cv.lineTo(ln / 2 - 6 * sc, 6 * sc);
    cv.curveTo(ln / 4, 20 * sc, -ln / 4, 20 * sc, -ln / 2, 3 * sc); cv.closePath(); cv.fill();
    for (const auto& shaft : oars) {
        const V2 grip = shaft[0], blade = shaft[1];
        cv.line(grip.x - 8 * sc, grip.y - 9 * sc, blade.x, blade.y, 1.8 * sc, oar);
        cv.line(blade.x - 3 * sc, blade.y - 7 * sc, blade.x + 3 * sc, blade.y + 7 * sc, 4 * sc, oar);
    }
    if (bearer && showBearer) {
        // The lantern-bearer stands at the bow, hat on, lantern out front.
        const double stern = -ln / 2 + 40 * sc, bow = ln / 2 - 34 * sc;
        const double px = lerp(stern, bow, bearerAt);
        RigIn r;
        r.h = bearerH > 0 ? bearerH : 74 * sc; r.facing = 1; r.hat = true; r.garment = Garment::Jacket;
        r.lean = (0.05 + 0.03 * std::sin(t * 0.8)) * sstep(35.3, 36.0, t) + 0.02 * (1 - sstep(35.3, 36.0, t)) - 0.09 * lookout;
        if (bearerWalk > 0 && bearerAt < 1) {
            Gait g; g.stride = 0.6 * r.h; g.lift = 0.06 * r.h; g.bob = 0.015 * r.h;
            const Steps st = gaitAt(stern, px - stern, -0.04 * r.h, 1, g, 1.0);
            r.hip = {px, 2 * sc - hipHeight(r.h) + st.hipBob};
            r.footF = st.footF; r.footB = st.footB;
        } else {
            r.hip = {px, 2 * sc - hipHeight(r.h)};
            r.footF = {px + 0.05 * r.h, -0.04 * r.h}; r.footB = {px - 0.08 * r.h, -0.04 * r.h};
        }
        r.handF = r.hip + V2((lerp(0.24, 0.30, sstep(35.3, 36.5, t)) - 0.08 * lookout) * r.h, (-0.18 - 0.10 * lookout - 0.03 * c.kick(5)) * r.h);
        r.handB = r.hip + V2(-0.05 * r.h, 0.05 * r.h);
        r.headTilt = (0.1 + 0.05 * std::sin(t * 1.1)) * sstep(35.3, 36.5, t) + 0.3 * lookout;
        if (drama) {
            const double enter = sstep(52,54,t);
            const double raise = window(t, release - 2.7, 1.1, release + plungeDuration(c) + 2.6, 1.0);
            const double guide = window(age - plungeDuration(c), 3.3, 0.7, 5.5, 0.9);
            r.hip = r.hip + V2(12 * sc * slip, 24 * sc * slip);
            r.lean = lerp(r.lean - 0.22 * brace, -0.62, slip);
            r.footF = lerp(r.footF, {px + 25 * sc, -10 * sc}, slip);
            r.handB = lerp(r.handB, {px - 7 * sc, -5 * sc}, slip);
            r.handF = lerp(r.handF, r.hip + V2((0.24 + 0.04 * guide) * r.h, (-0.18 - 0.14 * raise) * r.h), enter);
            r.headTilt += 0.25 * raise - 0.30 * slip;
            r.flutter = enter * (0.2 + 0.6 * slip) * std::sin(t * 5.5);
        }
        const Body pose = solve(r);
        drawBody(cv, pose, mix(OINK, mix(WAVE, KINK, 0.3), style));
        V2 hand = r.handF;
        if (drama) {
            // The lantern follows the drawn wrist when the bracing pose
            // reaches the rig's arm limit, never an unreachable target.
            const V2 reach = hand - pose.elbowF;
            hand = pose.elbowF + reach * std::min(1.0,0.16*r.h/std::max(1e-6,reach.len()));
        }
        const double ca = std::cos(tilt), sa = std::sin(tilt);
        const V2 handWorld = at + V2(hand.x * ca - hand.y * sa, hand.x * sa + hand.y * ca);
        const double swing = 0.05 * std::sin(t * 1.3) - tilt * 0.3;
        const double ls = r.h / 146;
        const V2 world = handWorld + V2((14 + std::sin(swing) * 26) * ls, (-6 + std::cos(swing) * 26) * ls);
        for (int j = 0; j < 18; ++j) {
            const double depth = j * (3.0 + 1.7 * hash2(j, 31));
            const double x = world.x + 5 * std::sin(t * 2.4 + j * 0.8) - depth * 0.30;
            const double width = (7 + 8 * hash2(j, 32)) * (1 - j / 22.0) * (0.75 + 0.45 * c.lift(1));
            const double y = at.y + 24 + depth + 2.5 * std::sin(t * 1.2 + j * 0.9);
            light.line(x - width, y - tilt * width, x + width, y + tilt * width, 1.3 + hash2(j, 33),
                       mix(WARM_B, GOLD, style), (0.35 + 0.25 * c.lift(1)) * (1 - 0.65 * style) * (1 - j / 18.0));
        }
        drawBearerLantern(light, light, handWorld, swing, ls, 1 + 0.45 * c.hit(9));
    }
    cv.restore();
    // Waterline and a short trailing wake share the hull's world transform.
    // These are surface marks, drawn beneath the hull rather than a second
    // reflection of the crew across the whole harbour.
    const double ca = std::cos(tilt), sa = std::sin(tilt);
    auto wp = [&](double x, double y) { return at + V2(x * ca - y * sa, x * sa + y * ca); };
    const V2 stern = wp(-ln * 0.47, 15 * sc), bow = wp(ln * 0.45, 14 * sc);
    light.line(stern.x, stern.y, bow.x, bow.y, 1.4 * sc, mix(Col(0.62f, 0.95f, 0.78f), BLUE, style), 0.40);
    for (int j = 0; j < 3; ++j) {
        const V2 a = wp(-ln * 0.43 - (18 + j * 13) * sc, (18 + j * 5) * sc);
        const V2 b = wp(-ln * 0.10, (17 + j * 4) * sc);
        light.line(a.x, a.y, b.x, b.y, 1.1 * sc, mix(Col(0.62f, 0.95f, 0.78f), BLUE, style), (0.25 + 0.15 * c.lift(0)) * (1 - j * 0.2));
    }
}

double kanagawaRise(const Ctx& c) {
    const double end = kanagawaBreak(c).t - 0.65;
    const double clock = clamp01((c.t - 52.0) / (end - 52.0));
    double music = clock;
    if (c.score) {
        const auto weight = [&](double at) { return c.score->integral(0, at) + 0.45 * c.score->integral(1, at); };
        const double whole = weight(end) - weight(52);
        if (whole > 0.05) music = clamp01((weight(c.t) - weight(52)) / whole);
    }
    // Cumulative musical weight keeps progress between beats. The face
    // becomes steep early enough to carry the chapter, then gathers a curl.
    const double u = 0.65 * clock + 0.35 * music;
    const double pressure=fluidBand(c,0)+.45*fluidBand(c,1);
    const double volume=.68+.32*sstep(.03,.32,pressure);
    return volume*(1-std::pow(1-sstep(0,1,u),2.1));
}

double kanagawaLean(const Ctx& c) {
    const double release = kanagawaBreak(c).t;
    return sstep(release - 5.2, release - 0.25, c.t);
}

// Bass hits jolt the crest upward; it settles back.
double crestKick(const Ctx& c) {
    if (!c.score) return 0;
    double k = 0;
    for (auto it = c.score->bassHits.rbegin(); it != c.score->bassHits.rend(); ++it) {
        const double age = c.t - it->t;
        if (age < 0) continue;
        if (age > 1.2) break;
        k += it->strength * std::exp(-age * 4.5) * (1 - std::exp(-age * 30));
    }
    return k;
}

Surge kanagawaBreak(const Ctx& c) {
    if (c.score) {
        // A bright accent releases the lip, followed by a measured contact.
        // Fixture:63.9167 onset,64.4917 bass return; no pitch detection.
        for (const Event& e : c.score->onsets) {
            if (e.t < 63.8 || e.t >= 64.25 || e.strength < 0.6) continue;
            const double bright = (c.score->integral(4, e.t) - c.score->integral(4, e.t - 0.12)
                + c.score->integral(5, e.t) - c.score->integral(5, e.t - 0.12)) / 0.12;
            if (bright > 0.25) return {e.t, e.strength, false};
        }
    }
    return {63.92, 0.6, true};
}

void kanagawaSky(Ctx& c) {
    Program& sky = c.gpu.effect("kanagawaSky", kanagawaSkyFs);
    c.gpu.pass(sky, Blend::Replace, [&](Program& q) { q.set("u_grade", 1.f); });
}
void kanagawaMist(Ctx& c) { mistBars(c, c.t); }
void kanagawaSea(Ctx& c) { sea(c, c.t); }

void gradeComposite(Ctx& c, int tex, double grade, double opacity) {
    Program& p = c.gpu.effect("gradeComposite", gradeCompositeFs);
    c.gpu.pass(p, Blend::Over, [&](Program& q) {
        c.gpu.bindTexture(0, tex, q, "u_tex");
        q.set("u_grade", float(grade)); q.set("u_opacity", float(opacity));
    });
}

std::array<std::vector<V2>, 6> waveStrands(double t, double rise) {
    const WaveFrame f = waveFrame(t, rise, 0, 0, 0);
    const double ks[6] = {-0.52, -0.18, 0.11, 0.29, 0.59, 0.87};
    std::array<std::vector<V2>, 6> out;
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < f.N(); ++j) out[std::size_t(i)].push_back(f.at(j, ks[i]));
    return out;
}

void kanagawaFuji(Ctx& c, double grade) {
    MountainLook m;
    m.px = 1086; m.peak = 486 - 10 * sstep(T::crossingEnd, T::crossingEnd + 8, c.t); m.base = HZ + 4; m.width = 150;
    m.top = m.bot = duotone(mix(WAVE, VIOLET, 0.25), grade);
    m.snow = 1; m.snowScale = 1; m.snowCol = duotone(PAPER, grade);
    m.foot = 0;
    const double camera=c.cameraX; c.cameraX*=.20;
    drawMountain(c, m); c.cameraX=camera;
}

void kanagawaSun(Ctx& c) {
    DiscLook d;
    d.pos = {1310 - .10*c.cameraX, 512 - 12 * sstep(T::crossingEnd, T::crossingEnd + 10, c.t)};
    d.r = 112;
    d.col = KRED * 1.08f; d.col2 = HOT * 1.08f;
    d.halo = ORANGE; d.ring = GOLD;
    d.haloA = 90; d.haloB = 50; d.haloC = 0.34; d.haloD = 0; d.haloFar = 0;
    d.veil = 0; d.tex = 0;
    d.energy = 0.35 + 0.6 * c.a.bass + 0.55 * c.lift(0) + 0.35 * c.kick(5);
    d.restRings = 1.6;
    drawDisc(c, d, 0);
}

namespace {
// The flock that left Osaka arrives as plovers over the sea.
void plovers(Ctx& c, const KState& k) {
    Canvas& cv = c.canvas();
    for (int pass = 0; pass < 7; ++pass) {
        const double start = 46.0 + pass * 2.8, age = c.t - start;
        if (age < 0 || age > 9.5) continue;
        for (int j = 0; j < 5; ++j) {
            const double x = 2020 - age * 230 + j * 35;
            const double y = 530 - 26 * std::sin(age * 0.5) + j * 7 - 12 * c.lift(4);
            drawBirdFly(cv, x, y, 7 + 2 * hash2(j, pass + 4), mix(OINK, PAPER, k.grade),
                        0.5 + 0.5 * std::sin(c.t * 8 + 3 * (c.score ? c.score->integral(5, c.t) : 0) + j * 1.7), -0.08);
        }
    }
    c.gpu.over(cv);
}

struct ClawBirth { double t; double u; double s; };

std::vector<ClawBirth> clawBirths(const Ctx& c, double from, double until) {
    std::vector<ClawBirth> out;
    double last = -10;
    int k = 0;
    auto add = [&](double t, double s) {
        out.push_back({t, 0.6 + 0.38 * hash2(k, 17 + c.seed), s});
        ++k; last = t;
    };
    if (c.score && !c.score->onsets.empty()) {
        for (const Event& e : c.score->onsets) {
            if (e.t < from || e.t > until) continue;
            if (e.t - last < (e.t > 71 ? 0.38 : 0.55)) continue;
            add(e.t, 0.6 + 0.4 * e.strength);
        }
    } else {
        for (double t = from; t < until; t += 0.8) add(t, 0.8);
    }
    return out;
}

void greatWave(Ctx& c, const KState& k, const Surge& br) {
    const double t = k.t, r = k.rise;
    const double age = br.t >= 0 ? t - br.t : -1;
    const double duration=plungeDuration(c);
    const double fall=age>0 ? std::pow(clamp01(age/duration),2) + std::max(0.0,age-duration) : 0;
    const double collapse=age>0 ? sstep(duration+.12,duration+3.4,age) : 0;
    const double lean=kanagawaLean(c);
    const WaveFrame f=waveFrame(t,r,t>=52 ? fluidBand(c,0) : c.a.bass,fall,collapse,lean, t>=52 ? 0 : crestKick(c),duration);
    const int N = f.N();
    const double fade = 1;
    Canvas& body = c.canvas();
    body.linear(0, 150, 0, 1080, {{0, mix(WAVE, MID, 0.35), 1}, {1, DEEP, 1}});
    bandPath(body, f, -1.0, 1.0);
    body.fill();
    const double stripes[5][3] = {{0.02, 0.20, 0.75}, {0.36, 0.52, 0.7}, {0.66, 0.80, 0.6}, {-0.42, -0.30, 0.45}, {-0.74, -0.64, 0.35}};
    int stripeBand=0;
    for (auto& stripe : stripes) {
        body.color(MID,stripe[2]);
        if(t<52) bandPath(body,f,stripe[0],stripe[1]);
        else {
            const int band=stripeBand++;
            const double energy=fluidBand(c,band),flow=.6*t+4*(c.score?c.score->integral(band,t):0);
            auto edge=[&](int j,double k) {
                const double pressure=.12*energy*std::sin(5*f.u[j]-flow)*sstep(.08,.28,f.u[j])*(1-sstep(.87,1,f.u[j]));
                return f.at(j,k+pressure);
            };
            for(int j=0;j<N;++j) {const V2 p=edge(j,stripe[0]);if(j==0)body.moveTo(p.x,p.y);else body.lineTo(p.x,p.y);}
            for(int j=N-1;j>=0;--j) {const V2 p=edge(j,stripe[1]);body.lineTo(p.x,p.y);}
            body.closePath();
        }
        body.fill();
    }
    c.gpu.over(body, 1, 0, float(fade));
    // The six strands run up the face; pulses travel with their bands.
    if (t >= 47.9 && t < 52) crossingStrands(c, t);
    if (t >= 52) {
        Canvas& st = c.canvas();
        const double ks[6] = {-0.52, -0.18, 0.11, 0.29, 0.59, 0.87};
        for (int i = 0; i < 6; ++i) {
            Rng rng(900 + i);
            std::vector<V2> P;
            for (int j = 0; j < N; ++j) P.push_back(f.at(j, ks[i]));
            P = resampleStrand(P, 80);
            st.polyline(P, 1.6 + 0.6 * c.lift(i), mix(SPRING, PAPER, 0.35), 0.38 + 0.42 * c.lift(i) + 0.20 * c.band(i));
            const double travel = strandClock(c, i);
            const double lvl = std::min(1.0, 0.3 + 1.1 * c.band(i) + 0.7 * c.lift(i));
            for (int q = 0; q < 4; ++q) {
                const double phase=std::fmod(rng.uni()+travel,1.0), head=phase*79;
                auto sample=[&](double h) { h=std::clamp(h,0.0,79.0); int j=std::min(78,int(h)); return lerp(P[j],P[j+1],h-j); };
                const double alpha=lvl*window(phase,0,.06,1,.09);
                for(int back=0;back<12;++back) {
                    if(head-back<=0) break;
                    const V2 a=sample(head-back),b=sample(head-back+1);
                    st.line(a.x,a.y,b.x,b.y,1.5,PAPER,alpha*std::pow(1-back/12.0,1.5));
                }
                const V2 at=sample(head);
                st.glowEllipse(at.x,at.y,10,2.5,i<2?GOLD:PAPER,.45*alpha);
            }
        }
        c.gpu.over(st, 1.15f, 0, float(fade));
        c.gpu.add(st, 0.22f * float(fade), 10);
    }
    // Connected crest foam. Fixed material anchors keep its identity stable;
    // small irregular fingers describe the lip instead of a row of beads.
    const double grow=sstep(.24,.65,r)*(1-collapse);
    if(t>=52 && grow>.001) {
        Canvas& foam=c.canvas();
        const int start=int(N*.40);
        std::vector<V2> cap,inside;
        for(int j=start;j<N;++j) {
            const double u=double(j-start)/(N-1-start);
            const double thickness=grow*sstep(0,.14,u)*(7+5*std::sin(u*39+0.25*t)+3*std::sin(u*93));
            cap.push_back(f.at(j,-1)); inside.push_back(cap.back()+f.n[j]*std::max(1.0,thickness));
        }
        foam.color(PAPER);
        foam.moveTo(cap[0].x,cap[0].y);
        for(auto p:cap) foam.lineTo(p.x,p.y);
        for(auto it=inside.rbegin();it!=inside.rend();++it) foam.lineTo(it->x,it->y);
        foam.closePath();foam.fill();
        for(int q=0;q<7;++q) {
            const int j=int((.80+.026*q)*(N-1));
            const V2 p=f.at(j,.88);
            const double ang=std::atan2(f.d[j].y,f.d[j].x)+.35;
            claw(foam,p,ang,(14+20*hash2(q,42))*grow,(2+2*hash2(q,41))*grow,1.1,PAPER,nullptr,9);
        }
        c.gpu.over(foam,1,0,float(grow));
        // Birth geometry is evaluated at the birth time, never moved with
        // the current crest. Gravity and drag carry each fragment onward.
        Canvas& spray=c.canvas();
        const auto births=clawBirths(c,54,std::min(t,br.t));
        for(int b=0;b<int(births.size());++b) {
            const double dt=t-births[b].t;
            if(dt<0 || dt>2.4) continue;
            Ctx born=c;born.t=births[b].t;
            const WaveFrame source=waveFrame(born.t,kanagawaRise(born),fluidBand(born,0),0,0,kanagawaLean(born),0);
            const int j=int((.83+.15*hash2(b,31))*(source.N()-1));
            const V2 o=source.at(j,-.95);
            for(int q=0;q<14;++q) {
                const double speed=65+120*hash2(b*17+q,3),angle=-.7+.9*hash2(b*17+q,4);
                const double travel=(1-std::exp(-.65*dt))/.65;
                const V2 p=o+V2(std::cos(angle)*speed*travel,std::sin(angle)*speed*travel+78*dt*dt);
                const double alpha=window(dt,0,.12,2.4,.8)*sstep(.24,.65,kanagawaRise(born));
                spray.capsule(p,p+V2(4,-3),1.6,0.6,PAPER,alpha);
            }
        }
        c.gpu.over(spray);
    }

}

void frontSwell(Ctx& c, double t, double lift) {
    Canvas& cv = c.canvas();
    // The near swell rolls up into view as Kanagawa arrives instead of
    // appearing on the handover frame.
    const double drop = 170 * (1 - sstep(47.9, 50.8, t));
    std::vector<V2> ys;
    for (double x = c.cameraX - 20; x <= c.cameraX + 1941; x += 6)
        ys.push_back({x, drop + 990 - (96 + 40 * lift) * std::exp(-std::pow((x - 1500 + 30 * std::sin(t * 0.2) - 65*std::max(0.0,t-64.5)) / 300, 2)) - 30 * std::exp(-std::pow((x - 760) / 200, 2)) + 12 * lerp(std::sin(x/60+t),std::sin(x/60+t*.35),sstep(52,54,t))});
    cv.color(mix(DEEP, KINK, 0.25));
    cv.moveTo(c.cameraX - 20, 1080);
    for (const V2& p : ys) cv.lineTo(p.x, p.y);
    cv.lineTo(c.cameraX + 1941, 1080);
    cv.closePath();
    cv.fill();
    for (int k = 0; k < 4; ++k) {
        std::vector<V2> s;
        for (const V2& p : ys) s.push_back({p.x, p.y + 14 + k * 18});
        cv.polyline(s, 1.6, BLUE, 0.4 - 0.08 * k);
    }
    Rng rs(23);
    if(t<54) {
    for (std::size_t j = 1; j + 1 < ys.size();) {
        if (ys[j].x > 1340 && ys[j].x < 1640) {
            const double a = std::atan2(ys[j + 1].y - ys[j - 1].y, 12);
            claw(cv, ys[j] + V2(0, 3), a - 2.5 + rs.normal() * 0.15 + 0.2 * std::sin(t * 1.4 + j), 30 + 34 * rs.uni(), 6 + 5 * rs.uni(), -1.8, PAPER, &WAVE, 10,1-sstep(52,54,t));
            j += std::size_t(4 + 5 * rs.uni());
        } else ++j;
    }
    }
    cv.polyline(ys, lerp(3,1.8,sstep(52,54,t)), PAPER, lerp(.9,.45,sstep(52,54,t)));
    c.gpu.over(cv);
}

// Pressure from the contact point lifts the hull; the expanding wake then
// settles. Both boats and foam use this same finite displacement.
double washHeight(double x, double age) {
    if (age < 0 || age > 7) return 0;
    const double a = age;
    const double front = 1305 + 155 * a;
    return 72 * window(a, 0, 0.4, 6.5, 3.0)
        * std::exp(-std::pow((x - front) / (230 + 65 * a), 2));
}

void wash(Ctx& c, double age, bool front) {
    if (age < 0 || age > 8) return;
    Canvas& cv = c.canvas();
    const double a = age, t=c.t;
    const double life = window(a, 0, 0.16, 9, 3.2);
    if (!front) {
        // Separate sheets thrown upward by impact, with negative space
        // between them. Their roots remain at the water contact point.
        for (int q = 0; q < 3; ++q) {
            const double born = 0.06 * q, dt = a - born;
            if (dt < 0 || dt > 2.9) continue;
            const double open = sstep(0, 0.28, dt), fade = 1 - sstep(.7+.2*hash2(q,45),1.6+.4*hash2(q,46),dt);
            const double rootX = 1250 + q * 47 + 14*(hash2(q,47)-.5);
            const double rootY=swellY(rootX,5,t,&c)+12;
            const double speeds[3]={-175,70,220};
            const double dx = speeds[q] * dt;
            const double height = std::max(0.0, (290 + 80 * hash2(q, 42)) * dt - 155 * dt * dt);
            const double width = (45 + 22 * hash2(q, 41)) * open * fade;
            std::vector<V2> centre, left, right;
            for(int j=0;j<=30;++j) {
                const double u=j/30.0;
                const V2 pos(rootX+dx*u*u+6*std::sin(u*13+q*3)*open,rootY-(12+height)*u);
                const V2 dir(2*dx*u,-12-height);
                const V2 normal(-dir.y/std::max(1.0,dir.len()),dir.x/std::max(1.0,dir.len()));
                // The thrown sheet tears from its tip downward, at unequal
                // material stations. Its silhouette breaks before alpha fades.
                const double tornAt=.34+.045*q+.28*(1-u)+.09*hash2(q*37+j,50);
                const double torn=sstep(tornAt,tornAt+.22,dt);
                const double w=width*(.55+.45*std::pow(std::sin(Pi*u),.7))*(1-.25*u)
                    *(.78+.22*std::sin(u*31+q))*(1-torn)
                    *(1-.5*torn*std::max(0.0,std::sin(u*47+q*2)));
                centre.push_back(pos);left.push_back(pos+normal*w);right.push_back(pos-normal*w);
            }
            cv.color(PAPER,fade);
            cv.moveTo(left.front().x,left.front().y);
            for(auto pos:left) cv.lineTo(pos.x,pos.y);
            for(auto it=right.rbegin();it!=right.rend();++it) cv.lineTo(it->x,it->y);
            cv.closePath();cv.fill();
            // Detached pieces inherit the sheet's position and velocity at
            // their own release, then arc downward into the existing wash.
            for(int j=4;j<=30;j+=2) {
                const double u=j/30.0;
                const double tornAt=.34+.045*q+.28*(1-u)+.09*hash2(q*37+j,50);
                const double flight=dt-tornAt;
                if(flight<0 || flight>1.4)continue;
                const double bornT=c.waterContact+born+tornAt;
                const double bornY=swellY(rootX,5,bornT,&c)+12;
                const double launch=290+80*hash2(q,42);
                const double bornHeight=std::max(0.0,launch*tornAt-155*tornAt*tornAt);
                const V2 origin(rootX+speeds[q]*tornAt*u*u+6*std::sin(u*13+q*3),bornY-(12+bornHeight)*u);
                const V2 velocity(speeds[q]*u*u+55*(hash2(q*37+j,51)-.5),
                    -(launch-310*tornAt)*u-35*hash2(q*37+j,52));
                const double drag=(1-std::exp(-.8*flight))/.8;
                const V2 at=origin+velocity*drag+V2(0,175*flight*flight);
                const double radius=(4+7*hash2(q*37+j,53))*(1-.35*sstep(.5,1.4,flight));
                const double alpha=window(flight,0,.08,1.4,.55);
                const V2 tail=velocity*(.018*(1-sstep(.5,1.2,flight)));
                cv.capsule(at-tail,at+tail,radius,radius*.6,PAPER,alpha);
            }

        }
        // Deterministic ballistic spray: drag limits horizontal reach and
        // gravity returns the droplets. Their origin never follows the crest.
        for (int i = 0; i < 420; ++i) {
            const double born = 0.26 * hash2(i, 31), dt = a - born;
            const double duration = 1.8 + 1.2 * hash2(i, 32);
            if (dt < 0 || dt >= duration) continue;
            const double angle = -Pi / 2 + (hash2(i, 33) - 0.5) * 2.55;
            const double speed = 140 + 420 * std::pow(hash2(i, 34), 0.55);
            const double travel = (1 - std::exp(-0.75 * dt)) / 0.75;
            const V2 pos = V2(1305 + 110 * (hash2(i,35)-0.5),850)
                + V2(std::cos(angle)*speed*travel, std::sin(angle)*speed*travel+175*dt*dt);
            const double alpha = window(dt,0,0.06,duration,0.65);
            const double size = (1.1+3.8*hash2(i,36)) * (1-0.4*dt/duration);
            if (i % 5 == 0) cv.capsule(pos, pos+V2(std::cos(angle),std::sin(angle))*size*3,
                size*0.6,size*0.22,PAPER,alpha);
            else cv.disc(pos.x,pos.y,size,PAPER,alpha);
        }
    }
    // Displaced foam stays in separated flowing patches. The near layer
    // reaches the foreground after the impact rather than covering the sea.
    for (int i = 0; i < (front ? 28 : 44); ++i) {
        const double born = 0.08 * (i % 7) + (front ? 0.65 : 0);
        const double dt = a - born;
        if (dt < 0) continue;
        const double side = hash2(i,front?71:72)-0.5;
        const double x = 1305 + side * (240 + 220 * dt);
        const double y = swellY(x,front?6:5,t,&c)+9+20*(hash2(i,73)-.5);
        const double alpha = window(dt,0,0.28,6.5,2.8)*0.50;
        const double width = (18+46*hash2(i,74))*(1+0.12*dt);
        const double height = (3+8*hash2(i,75))*(1-0.055*std::min(8.0,dt));
        cv.color(PAPER,alpha);
        cv.moveTo(x-width,y);
        cv.curveTo(x-width*.4,y-height*1.5,x+width*.1,y-height,x+width,y);
        cv.curveTo(x+width*.4,y+height,x-width*.2,y+height*.6,x-width,y);
        cv.closePath(); cv.fill();
        cv.line(x-width*.45,y+height*.35,x+width*.25,y+height*.35,1,WAVE,alpha*.6);
    }
    // Broken wake arcs follow the same surface as the supported hull.
    if(!front) for(int j=0;j<5;++j) {
        const double dt=a-.2-j*.22;
        if(dt<=0 || dt>5.5) continue;
        std::vector<V2> arc;
        for(int q=0;q<25;++q) {
            const double x=1305+(q/24.0-.5)*(140+dt*(190+j*12));
            const double y=swellY(x,5,t,&c)+16+10*j;
            arc.push_back({x,y});
        }
        cv.polyline(arc,1.3,mix(PAPER,BLUE,.4),.4*window(dt,0,.2,5.5,2.5));
    }
    c.gpu.over(cv,1,0,float(life));
}

// One coast supplies the visible bank and the water occlusion mask.
void shoreShape(Canvas& cv) {
    cv.moveTo(1785,645);cv.curveTo(1820,630,1840,595,1870,602);
    cv.curveTo(1900,570,1930,578,1970,563);
    cv.curveTo(2100,515,2280,535,2520,471);cv.lineTo(3100,430);
    cv.lineTo(3100,1200);cv.lineTo(2160,1200);cv.lineTo(2100,1020);
    cv.curveTo(2030,973,1970,903,1920,835);
    cv.curveTo(1898,806,1888,780,1886,764);
    cv.curveTo(1875,729,1820,675,1785,645);cv.closePath();
}

int shoreMask(Ctx& c) {
    Canvas& cv=c.canvas();cv.color(Col(1,1,1),sstep(51.8,54,c.t));shoreShape(cv);cv.fill();
    return c.gpu.layer(cv);
}

void waterComposite(Ctx& c,int texture,int mask) {
    static const char* shader=R"(
        uniform sampler2D u_tex,u_mask;
        void main() { o=texture(u_tex,v_uv)*(1.0-texture(u_mask,v_uv).a); }
    )";
    Program& p=c.gpu.effect("inletWater",shader);
    c.gpu.pass(p,Blend::Over,[&](Program& q){c.gpu.bindTexture(0,texture,q,"u_tex");c.gpu.bindTexture(1,mask,q,"u_mask");});
}

void inlet(Ctx& c,double) {
    const double reveal=sstep(51.8,54,c.t);
    Canvas& cv=c.canvas();
    cv.linear(1800,550,2200,1000,{{0,mix(WAVE,hex(0xad8ee6),.36),float(reveal)},{1,hex(0x24283b),float(reveal)}});
    shoreShape(cv);cv.fill();
    cv.polyline({{1785,645},{1840,702},{1876,742},{1886,764},{1920,835},{1980,912},{2100,1020}},2,BLUE,.35*reveal);
    // A sheltered stone lip, exactly at the arriving bow/deck depth.
    cv.color(hex(0x24283b),reveal);
    cv.moveTo(1886,764);cv.lineTo(1940,754);cv.lineTo(1942,763);cv.lineTo(1889,773);cv.closePath();cv.fill();
    cv.line(1886,764,1940,754,3,OCHRE,reveal);
    cv.line(1889,765,1889,737,2.4,OCHRE,reveal);cv.disc(1889,737,2.7,GOLD,reveal);
    cv.polyline({{1892,761},{1940,751},{1957,736},{1978,715},{2004,695},{2020,690}},8,hex(0x24283b),reveal);
    cv.polyline({{1892,761},{1940,751},{1957,736},{1978,715},{2004,695},{2020,690}},1.6,hex(0xe0af68),.45*reveal);
    cv.line(1944,747,1955,747,2,OCHRE,reveal);cv.line(1957,736,1967,736,2,OCHRE,reveal);
    cv.line(1937,746,1937,725,2,WAVE,reveal);cv.disc(1937,725,3,GOLD,reveal);
    // The real Tokyo Night reference is a mountain road with violet slopes.
    // Its first two bends belong to this connected hillside, not a wipe.
    auto road=catmull({{2020,690},{2108,658},{2140,638},{2100,620},{2120,598},{2320,560},{2460,530}},24);
    std::vector<V2> edge,other;
    for(int j=0;j<int(road.size());++j) {
        const V2 dir=road[std::min(j+1,int(road.size())-1)]-road[std::max(0,j-1)];
        const V2 n(-dir.y/std::max(1.0,dir.len()),dir.x/std::max(1.0,dir.len()));
        const double width=lerp(13,2,std::pow(j/double(road.size()-1),.7));
        edge.push_back(road[j]+n*width);other.push_back(road[j]-n*width);
    }
    cv.color(hex(0x13141c),reveal);cv.moveTo(edge[0].x,edge[0].y);
    for(auto at:edge)cv.lineTo(at.x,at.y);
    for(auto it=other.rbegin();it!=other.rend();++it)cv.lineTo(it->x,it->y);
    cv.closePath();cv.fill();
    cv.polyline(edge,1.5,hex(0xad8ee6),reveal*(.45+.3*fluidBand(c,3)));
    cv.polyline(other,1.2,hex(0xe0af68),reveal*.45);
    for(int j=3;j<20;j+=6) {V2 at=edge[j];cv.line(at.x,at.y,at.x,at.y-7,1.7,OCHRE,reveal);}
    // Ink strata and sparse pines give the shore a material and depth.
    cv.polyline({{1945,827},{2020,815},{2090,765},{2180,739}},1.2,BLUE,.22*reveal);
    cv.polyline({{2040,912},{2160,858},{2270,830},{2440,782}},1.3,VIOLET,.28*reveal);
    const V2 trees[]={{2060,670},{2165,638},{2250,580},{2350,550}};
    for(int i=0;i<4;++i) {
        const V2 at=trees[i];const double h=29-4*i;
        cv.line(at.x,at.y,at.x,at.y-h,2,hex(0x13141c),reveal);
        for(int j=0;j<3;++j) {
            const double y=at.y-h+j*h*.22,w=(5+j*2)*(1-i*.12);
            cv.moveTo(at.x,y);cv.lineTo(at.x+w,y+10);cv.lineTo(at.x-w,y+10);cv.closePath();cv.color(hex(0x13141c),reveal);cv.fill();
        }
    }
    c.gpu.over(cv);
}

double footpathY(double x) {
    const V2 points[]={{1886,764},{1940,751},{1957,736},{1978,715},{2004,695}};
    if(x<=1886)return 764;
    for(int i=0;i<4;++i) if(x<=points[i+1].x)return lerp(points[i].y,points[i+1].y,(x-points[i].x)/(points[i+1].x-points[i].x));
    return 695;
}

void landingBearer(Ctx& c,const BoatPose& boat,double scale) {
    const double t=c.t,h=74*scale;
    const double ca=std::cos(boat.tilt),sa=std::sin(boat.tilt);
    auto world=[&](V2 p){return boat.at+V2(p.x*ca-p.y*sa,p.x*sa+p.y*ca);};
    const double px=200*scale-34*scale;
    RigIn r;r.h=h;r.hat=true;r.garment=Garment::Jacket;r.facing=1;
    r.lean=.05+.03*std::sin(t*.8)+boat.tilt;
    const V2 hip=world({px,2*scale-hipHeight(h)}),front=world({px+.05*h,-.04*h}),back=world({px-.08*h,-.04*h});
    r.hip=hip;r.footF=front;r.footB=back;
    const V2 advance(20*ca,20*sa);
    r.hip=r.hip+advance*sstep(73.8,74.2,t);
    const double bowFront=sstep(73.8,74.0,t),bowRear=sstep(74.0,74.2,t);
    r.footF=front+advance*bowFront;r.footF.y-=4*std::sin(Pi*bowFront);
    r.footB=back+advance*bowRear;r.footB.y-=3*std::sin(Pi*bowRear);
    if(t>=74.2) {
        const double step=sstep(74.2,74.8,t),weight=sstep(74.35,75.1,t),rear=sstep(74.8,75.4,t);
        r.footF=lerp(front+advance,{1891,footpathY(1891)},step);r.footF.y-=7*std::sin(Pi*step);
        r.footB=lerp(back+advance,{1898,footpathY(1898)},rear);r.footB.y-=6*std::sin(Pi*rear);
        const double hipX=lerp(hip.x+advance.x,(r.footF.x+r.footB.x)*.5+.8,step);
        const double leg=.505*h;
        auto supportedY=[&](V2 foot) {
            const double dx=foot.x-hipX;
            return foot.y-std::sqrt(std::max(0.0,leg*leg-dx*dx));
        };
        const double wantedY=lerp(hip.y,footpathY(1894)-hipHeight(h),weight);
        r.hip={hipX,std::max(wantedY,std::max(supportedY(r.footF),supportedY(r.footB)))};
        r.lean=lerp(r.lean,.12,weight);
    }
    if(t>=75.4) {
        const double distance=90*sstep(75.4,77.4,t),blend=sstep(75.4,75.7,t);
        Gait gait;gait.stride=30;gait.lift=4;gait.bob=1;
        const Steps feet=gaitAt(1894,distance,760,1,gait,1-sstep(77.1,77.6,t));
        const V2 ff(feet.footF.x,footpathY(feet.footF.x)+feet.footF.y-760),fb(feet.footB.x,footpathY(feet.footB.x)+feet.footB.y-760);
        r.footF=lerp(r.footF,ff,blend);r.footB=lerp(r.footB,fb,blend);
        r.hip=lerp(r.hip,{1894+distance,footpathY(1894+distance)-hipHeight(h)+feet.hipBob},blend);
        r.lean=.10;
    }
    if(t>=74.2) for(V2 foot:{r.footF,r.footB}) {
        const double dx=foot.x-r.hip.x,leg=.505*h;
        r.hip.y=std::max(r.hip.y,foot.y-std::sqrt(std::max(0.0,leg*leg-dx*dx)));
    }
    const double arm=lerp(.24,.30,sstep(73.8,74.5,t));
    r.handF=r.hip+V2((arm*ca+.18*sa)*h,(arm*sa-.18*ca)*h);r.handB=r.hip+V2(-.05*h,.05*h);
    r.handB=lerp(r.handB,{1889,739},window(t,74.62,.08,74.95,.1));
    r.headTilt=.1+.05*std::sin(t*1.1);
    const Body pose=solve(r);
    const V2 reach=r.handF-pose.elbowF,hand=pose.elbowF+reach*std::min(1.0,.16*h/std::max(1e-6,reach.len()));
    Canvas& body=c.canvas();drawBody(body,pose,mix(WAVE,KINK,.3));c.gpu.over(body);
    Canvas& light=c.canvas();
    drawBearerLantern(light,light,hand,.05*std::sin(t*1.3)-boat.tilt*.3,h/146,1+.45*c.hit(9));
    c.gpu.over(light,1.5);c.gpu.add(light,.3,12);
}
}  // namespace

void kanagawaEntry(Ctx& c) {
    KState k; k.t = c.t; k.grade = sstep(37.0, 51.0, c.t);
    greatWave(c, k, Surge{});
}

void kanagawaEntryFront(Ctx& c) { frontSwell(c, c.t, 0); }

void kanagawaPlovers(Ctx& c, double grade) { KState k; k.grade = grade; plovers(c, k); }

void kanagawaForeground(Ctx& c, const KState& k) {
    const double t = k.t;
    const Surge br = k.breakAt >= 0 ? Surge{k.breakAt, 1, false} : Surge{};
    const double age = br.t >= 0 ? t - br.t : -1;
    if (k.plovers) {
        plovers(c, k);
        if (t < 54) drawFlock(c, OINK, 1.0, 1 - sstep(50.0, 54.0, t));
    }
    // Boats climb and drop on the swell; the near boat tilts as the wave towers,
    // and both ride through the wash.
    const double stroke = rowingClock(c);
    if (t >= 52) {
        const double contact=age-plungeDuration(c);
        const int mask=shoreMask(c);
        c.gpu.beginOffscreen(true);
        if(k.wave) { greatWave(c,k,br);wash(c,contact,false); }
        waterComposite(c,c.gpu.endOffscreen(),mask);
        inlet(c,age);
        Canvas& boats=c.canvas();Canvas& lights=c.canvas();
        BoatPose near=seaBoatPose(c,true),far=seaBoatPose(c,false);
        const double departure=sstep(67.3,73.0,t),sc=lerp(1.0,.78,departure),moor=sstep(71.8,73.0,t);
        const double shove=26*window(contact,0,.35,4.3,2.0);
        const double nx=lerp(near.at.x,1715,departure)+shove,fx=lerp(far.at.x,1650,departure);
        auto support=[&](double x,double row,double half) {
            const int i=int(row);const double f=row-i;
            auto y=[&](double xx) {return lerp(swellY(xx,i,t,&c),swellY(xx,i+1,t,&c),f);};
            const double left=y(x-half),right=y(x+half);
            return BoatPose{{x,(left+2*y(x)+right)*.25-9},std::clamp(std::atan2(right-left,2*half),-.13,.13)};
        };
        near=support(nx,lerp(4.34,3.35,departure),140*sc);
        far=support(fx,lerp(2.40,2.1,departure),85);
        near.tilt+=.13*ring(contact,.32,.95);far.tilt+=.07*ring(contact-.25,.32,.95);
        near.at.y-=8*ring(contact,.38,1.0);
        near.at=lerp(near.at,{1715,774+1.1*std::sin(t*.7)},moor);
        near.tilt=lerp(near.tilt,-.035+.002*std::sin(t*.7),moor);
        if(k.boats) {
            drawBoat(c,boats,lights,far.at,250,far.tilt,6,k.grade,.7,false,stroke+.3);
            drawBoat(c,boats,lights,near.at,400*sc,near.tilt,5,k.grade,sc,true,stroke,1,0,0,t<73.8,moor);
        }
        c.gpu.over(boats);c.gpu.over(lights,1.5f);c.gpu.add(lights,.4f,16);
        if(contact>0 && contact<2.2) {
            Canvas& edge=c.canvas();
            const double ca=std::cos(near.tilt),sa=std::sin(near.tilt);
            for(int patch=0;patch<3;++patch) {
                std::vector<V2> wet;
                for(int j=0;j<9;++j) {
                    const double x=-100+patch*60+j*6,y=15*sc+2*std::sin(j*.8-contact*2+patch);
                    wet.push_back(near.at+V2(x*ca-y*sa,x*sa+y*ca));
                }
                edge.polyline(wet,1.8,mix(PAPER,SPRING,.5),.5*window(contact,.08,.15,2.2,.8));
            }
            c.gpu.over(edge);
        }
        if(t>=73.8) landingBearer(c,near,sc);
        c.gpu.beginOffscreen(true);
        frontSwell(c,t,k.rise*(1-sstep(0,3.4,contact)));
        if(k.wave)wash(c,contact,true);
        waterComposite(c,c.gpu.endOffscreen(),mask);
        return;
    }
    const double washLift = age > 0 ? washHeight(1250, age) : 0;
    Canvas& bc = c.canvas();
    Canvas& bl = c.canvas();
    if (k.boats) {
        BoatPose far = seaBoatPose(c, false), near = seaBoatPose(c, true);
        far.at.y -= std::min(60.0, 0.12 * washHeight(1620, std::max(0.0, age))) * (age > 0);
        drawBoat(c, bc, bl, far.at, 250, far.tilt, 6, k.grade, 0.7, false, stroke + 0.3);
        const double lean = kanagawaLean(c) * (1 - sstep(0, 1.5, age));
        near.at.y += 14 * k.rise * std::sin(t * 0.55) - std::min(110.0, 0.2 * washLift) - 26 * lean;
        near.tilt += 0.12 * k.rise * (1 - sstep(0, 2, age)) + 0.08 * lean - 0.1 * std::sin(clamp01(age / 3) * Pi);
        drawBoat(c, bc, bl, near.at, 400, near.tilt, 5, k.grade, 1.0, true, stroke);
    }
    if (k.wave) {
        wash(c, age, false);
        // Open-water strands stay behind the hull while joining the swell.
        // The later foreground build/crash retains its existing boat depth.
        if (t < 52) greatWave(c, k, br);
        c.gpu.over(bc);
        c.gpu.over(bl, 1.5f);
        c.gpu.add(bl, 0.4f, 16);
        if (t >= 52) greatWave(c, k, br);
    } else {
        c.gpu.over(bc);
        c.gpu.over(bl, 1.5f);
        c.gpu.add(bl, 0.4f, 16);
    }
    frontSwell(c, t, k.rise);
    if (k.wave) wash(c, age, true);
}

void drawKanagawa(Ctx& c) {
    const double t = c.t;
    c.cameraX=460*sstep(68.5,77.0,t);
    c.waterContact=kanagawaBreak(c).t+plungeDuration(c);
    KState k;
    k.t = t;
    k.grade = sstep(37.0, 51.0, t);
    k.rise = kanagawaRise(c);
    k.breakAt = kanagawaBreak(c).t;
    c.gpu.beginOffscreen();
    kanagawaSky(c);
    mistBars(c, t);
    gradeComposite(c, c.gpu.endOffscreen(), k.grade, 1);
    kanagawaSun(c);
    kanagawaFuji(c, k.grade);
    const int mask=shoreMask(c);
    c.gpu.beginOffscreen(true);
    sea(c, t);
    waterComposite(c,c.gpu.endOffscreen(),mask);
    kanagawaForeground(c, k);
    FinishParams f;
    f.time = float(t);
    f.bloom = float(lerp(0.55, 0.3, k.grade)); f.grain = 0.016f; f.vignette = 0.34f; f.paper = float(k.grade);
    Canvas& title = c.canvas();
    title.translate(c.cameraX,0);
    drawTitle(title, 1, window(t, 48.6, 0.8, 53.8, 1.2), BLUE, PAPER);
    drawTitle(title,2,window(t,76.3,.8,78,.5),hex(0xad8ee6),PAPER);
    c.gpu.finish(f, &title);
}
}
