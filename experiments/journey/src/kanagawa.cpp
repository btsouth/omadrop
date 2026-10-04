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

double plungeDuration(const Ctx&) { return .60; }

// Velocity-shaped drops: tapered silhouettes, never bead particles.
void waterDrop(Canvas& cv,V2 p,V2 velocity,double size,double alpha) {
    const double speed=std::max(1.0,velocity.len());
    const V2 d=velocity*(1/speed),n(-d.y,d.x);
    const V2 tip=p+d*(size*3.2),tail=p-d*(size*1.5);
    cv.color(PAPER,alpha);
    cv.moveTo(tip.x,tip.y);
    V2 a=p+n*size,b=tail+n*size*.45;
    cv.curveTo(a.x,a.y,b.x,b.y,tail.x,tail.y);
    a=tail-n*size*.45;b=p-n*size;
    cv.curveTo(a.x,a.y,b.x,b.y,tip.x,tip.y);cv.closePath();cv.fill();
}

// Tapered curved ribbons with a blue shadow and branching cream tips.
void foamFinger(Canvas& cv,V2 root,double angle,double length,double width,double bend,Col col,double alpha) {
    std::vector<V2> left,right;
    V2 p=root;
    for(int j=0;j<=12;++j) {
        const double u=j/12.0,a=angle+bend*u;
        const V2 n(-std::sin(a),std::cos(a));
        const double w=width*std::pow(1-u,.8)+.25;
        left.push_back(p+n*w);right.push_back(p-n*w);
        p=p+V2(std::cos(a),std::sin(a))*(length/12);
    }
    cv.color(col,alpha);cv.moveTo(left[0].x,left[0].y);
    for(auto q:left)cv.lineTo(q.x,q.y);
    for(auto it=right.rbegin();it!=right.rend();++it)cv.lineTo(it->x,it->y);
    cv.closePath();cv.fill();
}

const char* kanagawaSkyFs = R"(
uniform float u_grade,u_arrival,u_paper;
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
    vec3 sunset=mix(vec3(.23,.11,.37),vec3(.98,.47,.39),smoothstep(100.0,560.0,y));
    sunset=mix(sunset,vec3(1.0,.72,.46),smoothstep(490.0,612.0,y));
    c=mix(c,sunset,u_arrival);
    vec3 paper=mix(vec3(.91,.855,.725),vec3(.64,.645,.615),smoothstep(190.0,640.0,y));
    c=mix(c,paper,u_paper);
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
        if(t>=50) {
            const double travel=clamp01((t-50)/14.7),growth=clamp01((t-50)/15.0);
            const double crestX=lerp(140,1100,travel),top=790-610*std::pow(growth,.85)*(.72+.28*sstep(.015,.26,bass))-80*bass*growth;
            const double spill=sstep(56,64.7,t);
            const V2 moving[10]={{-420,1190},{-210,1060},{crestX-610,910},
                {crestX-350,top+380},{crestX-140,top+145},{crestX,top},
                {crestX+130,top+25},{crestX+265+50*spill,top+140},
                {crestX+330+65*spill,top+345},{crestX+275+75*spill,top+555}};
            const V2 calm[10]={{-420,1190},{-210,1060},{160,980},{440,930},{700,902},
                {960,894},{1190,898},{1430,916},{1710,950},{2160,980}};
            const double entry=sstep(50,52,t);
            p=lerp(p,moving[i],entry);
            if(fall>0) {
                const double k=sstep(3,8,i),v=clamp01(fall);
                const V2 contact[10]={{-420,1190},{-210,1060},{230,960},{540,810},{820,650},
                    {1050,620},{1210,710},{1300,850},{1430,900},{1600,920}};
                p=lerp(p,contact[i],v*k);
                p.x+=80*std::max(0.0,fall-1)*k;
            }
            p=lerp(p,calm[i],collapse);
        }
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
        if(t>=50) for(int i=0;i<N;++i) {
            const double crown=std::sin(Pi*f.u[i]);
            f.w[i]=std::max(f.w[i],(65+25*r)*crown*r*(1-collapse)*(1-sstep(.91,1.0,f.u[i])));
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
    return kanagawaPrintSupport(c,{{x,y},tilt},half);
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
    if(c.t<48) return 0;
    const double clock=sstep(50,kanagawaBreak(c).t+.3,c.t);
    const double pressure=fluidBand(c,0)+.45*fluidBand(c,1);
    return std::min(1.0,clock*(.80+.20*sstep(.03,.32,pressure)));
}

double kanagawaLean(const Ctx& c) { return sstep(56,kanagawaBreak(c).t,c.t); }

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
    // Strongest measured onset in this authored crash window. A six-tenths
    // plunge puts the actual hull fracture on it, not on the preceding beat.
    Event strongest{65.30,.65};
    bool found=false;
    if(c.score) for(const Event& e:c.score->onsets) {
        if(e.t>=62 && e.t<66 && (!found || e.strength>strongest.strength)) {strongest=e;found=true;}
    }
    return {strongest.t-.60,strongest.strength,!found};
}

void kanagawaSky(Ctx& c) {
    Program& sky = c.gpu.effect("kanagawaSky", kanagawaSkyFs);
    c.gpu.pass(sky, Blend::Replace, [&](Program& q) { q.set("u_grade", 1.f);q.set("u_arrival",float(std::max(sstep(68,78,c.t),.62*(1-kanagawaPaperWeight(c.t))*sstep(66,68.5,c.t))));q.set("u_paper",float(kanagawaPaperWeight(c.t))); });
}
void kanagawaMist(Ctx& c) { mistBars(c, c.t); }
void kanagawaSea(Ctx& c) {
    const double paper=kanagawaPaperWeight(c.t);
    if(paper<.999)sea(c,c.t);
    if(paper>0)kanagawaPrintSea(c,paper);
}
void kanagawaHorizon(Ctx& c) {
    // The ending water is opaque below HZ. Keep the original exposed horizon
    // exactly, without solving and drawing the six fully covered depth rows.
    Canvas& cv=c.canvas();std::vector<V2> ys;
    for(double x=c.cameraX-20;x<=c.cameraX+1941;x+=6)
        ys.push_back({x,rawSwellY(x,0,c.t,&c)});
    const Col col=mix(mix(MID,GOLD,.25),DEEP,0);
    cv.color(col);cv.moveTo(c.cameraX-20,1080);
    for(const V2& p:ys)cv.lineTo(p.x,p.y);
    cv.lineTo(c.cameraX+1941,1080);cv.closePath();cv.fill();
    const double fast=.50+.7*c.band(5)+.75*c.lift(5);
    const double crest=.75*lerp(fast,.50+1.25*fluidBand(c,5),sstep(48,51.8,c.t));
    cv.polyline(ys,1.2,mix(PAPER,BLUE,.3),std::min(1.0,crest));c.gpu.over(cv);
}

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
    const double paper=kanagawaPaperWeight(c.t);
    if(paper>=.999)return;
    if(paper>0)c.gpu.beginOffscreen(true);
    DiscLook d;
    d.pos = {1310 - .34*c.cameraX, 512 - 12 * sstep(T::crossingEnd, T::crossingEnd + 10, c.t)+95*sstep(69,83,c.t)};
    d.r = 112;
    d.col = mix(KRED * 1.08f,GOLD,sstep(69,83,c.t)); d.col2 = mix(HOT * 1.08f,ORANGE,sstep(69,83,c.t));
    d.halo = ORANGE; d.ring = GOLD;
    d.haloA = 90; d.haloB = 50; d.haloC = 0.34; d.haloD = 0; d.haloFar = 0;
    d.veil = 0; d.tex = 0;
    d.energy = 0.35 + 0.6 * c.a.bass + 0.55 * c.lift(0) + 0.35 * c.kick(5);
    d.restRings = 1.6;
    drawDisc(c, d, 0);
    if(paper>0)c.gpu.composite(c.gpu.endOffscreen(),Blend::Over,1,float(1-paper));
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
    const double collapse=age>0 ? sstep(duration+.06,duration+1.75,age) : 0;
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
                const double pressure=.34*energy*std::sin(5*f.u[j]-flow)*sstep(.08,.28,f.u[j])*(1-sstep(.87,1,f.u[j]));
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
    const double grow=sstep(.12,.55,r)*(1-collapse);
    if(t>=50 && grow>.001) {
        Canvas& foam=c.canvas();
        // Clusters have unequal branching and a blue under-layer. Their
        // anchors flow along the crest while the face continues travelling.
        for(int cluster=0;cluster<9;++cluster) {
            const double u=.51+.052*cluster;
            const int j=int(u*(N-1));
            const V2 root=f.at(j,-.92);
            const double direction=std::atan2(f.d[j].y,f.d[j].x)-.35;
            const double flow=.22*std::sin(t*1.8+cluster)+.65*c.lift(3+cluster%3);
            const double length=(65+85*hash2(cluster,13))*grow*(1+.25*flow);
            for(int branch=0;branch<3;++branch) {
                const double angle=direction-.65+branch*.48+flow*.20;
                const V2 origin=root+f.d[j]*(branch-1)*11*grow;
                const double len=length*(.72+.18*branch);
                foamFinger(foam,origin+V2(-4,6),angle,len,13*grow, .8,BLUE,.85);
                foamFinger(foam,origin,angle,len,9*grow,.8,PAPER,1);
                for(int twig=0;twig<2;++twig) {
                    const double at=.40+.22*twig;
                    const V2 fork=origin+V2(std::cos(angle+.3*at),std::sin(angle+.3*at))*len*at;
                    const double a=angle+.8*at+(twig?-.65:.7);
                    foamFinger(foam,fork+V2(-2,3),a,len*.38,4*grow,.55,BLUE,.8);
                    foamFinger(foam,fork,a,len*.38,2.5*grow,.55,PAPER,1);
                }
            }
        }
        // Lacy connected face veins follow the same material coordinates.
        for(int line=0;line<22;++line) {
            std::vector<V2> vein;
            const int from=int((.19+.019*line)*(N-1)),to=std::min(N-1,from+70);
            for(int j=from;j<to;j+=3) {
                const double u=(j-from)/70.0,flow=.6*t+6*(c.score?c.score->integral(line%6,t):0),
                    k=-.75+.065*(line%17)+.04*std::sin(u*18+line-flow);
                vein.push_back(f.at(j,k));
            }
            foam.polyline(vein,1.1+hash2(line,6)+.65*c.lift(line%6),PAPER,.28*grow);
            if(line%2==0 && vein.size()>10) {
                V2 q=vein[vein.size()/2];
                foam.polyline({q,q+V2(8,-14)*grow,q+V2(18,-24)*grow},1,PAPER,.30*grow);
            }
        }
        c.gpu.over(foam);
        Canvas& spray=c.canvas();
        const auto births=clawBirths(c,52,std::min(t,br.t+.45));
        for(int b=0;b<int(births.size());++b) {
            const double dt=t-births[b].t;
            if(dt<0 || dt>2.0)continue;
            Ctx born=c;born.t=births[b].t;
            const WaveFrame source=waveFrame(born.t,kanagawaRise(born),fluidBand(born,0),0,0,kanagawaLean(born),0);
            const int j=int((.67+.25*hash2(b,31))*(source.N()-1));
            const V2 origin=source.at(j,-.96);
            for(int q=0;q<22;++q) {
                const double speed=(95+230*hash2(b*23+q,3))*(.65+births[b].s);
                const V2 v(speed,-speed*(.18+.65*hash2(q,b+4)));
                const double drag=(1-std::exp(-.55*dt))/.55;
                const V2 p=origin+v*drag+V2(0,110*dt*dt);
                waterDrop(spray,p,v*std::exp(-.55*dt)+V2(0,220*dt),1+2*hash2(q,b+8),window(dt,0,.08,2,.6)*grow);
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
    if(t<50) {
    for (std::size_t j = 1; j + 1 < ys.size();) {
        if (ys[j].x > 1340 && ys[j].x < 1640) {
            const double a = std::atan2(ys[j + 1].y - ys[j - 1].y, 12);
            claw(cv, ys[j] + V2(0, 3), a - 2.5 + rs.normal() * 0.15 + 0.2 * std::sin(t * 1.4 + j), 30 + 34 * rs.uni(), 6 + 5 * rs.uni(), -1.8, PAPER, &WAVE, 10,1-sstep(48,50,t));
            j += std::size_t(4 + 5 * rs.uni());
        } else ++j;
    }
    }
    cv.polyline(ys, lerp(3,1.8,sstep(52,54,t)), PAPER, lerp(.9,.45,sstep(52,54,t)));
    c.gpu.over(cv);
}

// Pressure from the contact point lifts the hull; the expanding wake then
// settles. Both boats and foam use this same finite displacement.
double washHeight(double x,double age) {
    if(age<0 || age>8)return 0;
    return 145*window(age,0,.15,7.5,3.2)*std::exp(-std::pow((x-1280-170*age)/(300+85*age),2));
}

void wash(Ctx& c,double age,bool front) {
    // Fracture and the paper flash lead the expanding water by .14 seconds.
    age-=.14;
    if(age<0 || age>8)return;
    Canvas& cv=c.canvas();
    if(!front) {
        // A broad irregular crown, with open branching edges rather than
        // three small fountain lobes. It ruptures before the quiet aftermath.
        if(age<1.25) {
            const double grow=sstep(0,.22,age)*(1-sstep(.58,1.25,age));
            cv.color(PAPER);cv.moveTo(1280-610*grow,870);
            for(int q=0;q<=44;++q) {
                const double u=q/44.0,theta=Pi*(1-u),r=.87+.13*std::sin(u*49)+.07*std::sin(u*93);
                cv.lineTo(1280+610*grow*r*std::cos(theta),870-430*grow*r*std::sin(theta));
            }
            cv.lineTo(1280+610*grow,900);cv.closePath();cv.fill();
            for(int q=0;q<14;++q) {
                const double a=-2.9+q*.19;
                const V2 p(1280+130*grow*std::cos(a),845+50*grow*std::sin(a));
                foamFinger(cv,p,a,(80+90*hash2(q,15))*grow,5*grow,.85,BLUE,.6);
            }
        }
        for(int fan=0;fan<9;++fan) {
            const double dt=age-.025*fan;
            if(dt<0 || dt>1.35)continue;
            const double open=sstep(0,.18,dt),drain=1-sstep(.60,1.35,dt);
            const V2 root(1250+(fan-4)*42,860);
            const double angle=-2.6+fan*.27;
            const double curve=fan<4?.9:-.7;
            foamFinger(cv,root+V2(-8,10),angle,(430+100*hash2(fan,2))*open*drain,65*open*drain,curve,BLUE,1);
            foamFinger(cv,root,angle,(440+100*hash2(fan,2))*open*drain,55*open*drain,curve,PAPER,1);
            for(int b=0;b<4;++b) {
                const V2 p=root+V2(std::cos(angle),std::sin(angle))*(210+b*43)*open*drain;
                foamFinger(cv,p,angle+(b%2?.6:-.5),(65+90*hash2(b,fan))*open*drain,12*open*drain,1.45,PAPER,drain);
            }
        }
        for(int i=0;i<720;++i) {
            const double dt=age-.18*hash2(i,31),life=1.8+1.8*hash2(i,32);
            if(dt<0 || dt>life)continue;
            const double angle=-Pi/2+(hash2(i,33)-.5)*2.9;
            const double speed=240+730*std::pow(hash2(i,34),.65);
            const V2 v(std::cos(angle)*speed,std::sin(angle)*speed),o(1280+180*(hash2(i,35)-.5),850);
            const double drag=(1-std::exp(-.7*dt))/.7;
            const V2 p=o+v*drag+V2(0,210*dt*dt);
            waterDrop(cv,p,v*std::exp(-.7*dt)+V2(0,420*dt),1.1+3.7*hash2(i,36),window(dt,0,.04,life,.7));
        }
    }
    // Rolling foreground wall: large for .7 s, then tears into long lace
    // patches. It moves toward the viewer instead of fading in place.
    const double delayed=age-(front?.12:0),life=window(delayed,0,.12,6.8,2.7);
    if(delayed>=0) {
        for(int i=0;i<(front?26:38);++i) {
            const double spread=290+310*delayed;
            const double x=1280+(hash2(i,front?70:71)-.5)*spread*2;
            const double y=(front?955:850)+60*delayed+30*std::sin(i*2.4+delayed);
            const double width=(50+100*hash2(i,73))*(1+.25*delayed),height=(front?80:35)*(1-sstep(.55,1.45,delayed))+8;
            const double a=life*(.80-.30*sstep(.65,1.4,delayed));
            cv.color(PAPER,a);cv.moveTo(x-width,y);
            cv.curveTo(x-width*.55,y-height,x+width*.2,y-height,x+width,y);
            cv.curveTo(x+width*.1,y+height*.55,x-width*.5,y+height*.2,x-width,y);cv.closePath();cv.fill();
            cv.polyline({{x-width*.7,y+height*.12},{x,y+height*.3},{x+width*.5,y}},1.5,BLUE,a*.65);
        }
    }
    c.gpu.over(cv);
}

// One coast supplies the visible bank and the water occlusion mask.
void shoreShape(Canvas& cv) {
    cv.moveTo(1740,1080);cv.curveTo(1760,1000,1830,963,1868,915);
    cv.curveTo(1834,861,1840,817,1875,788);
    cv.curveTo(1930,742,2050,722,2140,642);
    cv.curveTo(2230,619,2330,650,2530,516);cv.curveTo(2760,590,2970,502,3210,470);
    cv.lineTo(4300,430);cv.lineTo(4300,1200);cv.lineTo(1740,1200);cv.closePath();
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

double footpathY(double x) {
    const V2 p[]={{1888,925},{1940,917},{2050,880},{2220,820},{2320,767},{2400,746}};
    if(x<=p[0].x)return p[0].y;
    for(int i=0;i<5;++i)if(x<p[i+1].x)return lerp(p[i].y,p[i+1].y,(x-p[i].x)/(p[i+1].x-p[i].x));
    return p[5].y;
}

void inlet(Ctx& c,double) {
    const double reveal=sstep(51.8,54,c.t),sunset=sstep(67,77,c.t);
    const Col violet=hex(0xad8ee6),pink=hex(0xf7768e),navy=hex(0x24283b);
    Canvas& cv=c.canvas();
    cv.linear(1800,630,2600,1100,{{0,mix(WAVE,violet,.45),float(reveal)},{1,hex(0x161729),float(reveal)}});
    shoreShape(cv);cv.fill();
    // Distinct ridges overlap in perspective: irregular far peaks, a
    // sheltered valley and broad foreground slopes, not repeated ribbons.
    const std::vector<std::vector<V2>> hills={
        {{1875,788},{2040,742},{2140,648},{2320,660},{2530,529},{2790,602},{3140,520},{3660,494},{4300,490}},
        {{1868,870},{2020,793},{2250,741},{2460,708},{2660,615},{2830,664},{3110,588},{3570,665},{4300,590}},
        {{1900,920},{2090,871},{2330,847},{2590,740},{2790,772},{3070,730},{3490,742},{3880,675},{4300,680}},
        {{1970,975},{2220,910},{2470,943},{2740,856},{2960,885},{3280,811},{3650,850},{4300,790}},
        {{2240,1100},{2440,997},{2690,1032},{2880,956},{3240,995},{3570,921},{3960,970},{4300,900}}
    };
    for(int layer=0;layer<int(hills.size());++layer) {
        const auto ridge=catmull(hills[layer],22);
        const Col top=mix(mix(violet,pink,.32*sunset),navy,.35+.13*layer);
        cv.linear(0,550+layer*95,0,1150,{{0,top,float(reveal)},{1,mix(navy,violet,.03),float(reveal)}});
        cv.moveTo(ridge[0].x,1200);for(auto q:ridge)cv.lineTo(q.x,q.y);
        cv.lineTo(4300,1200);cv.closePath();cv.fill();
        cv.polyline(ridge,1.4,mix(pink,GOLD,.25),reveal*sunset*(.25-.03*layer));
    }
    // A warm sand tongue connects the arriving surge to the road shoulder.
    cv.color(mix(OCHRE,violet,.55),reveal);
    cv.moveTo(1885,794);cv.curveTo(1860,850,1870,918,1930,967);
    cv.curveTo(1975,970,2000,950,2058,902);cv.lineTo(2110,862);
    cv.curveTo(1980,888,1910,869,1885,794);cv.closePath();cv.fill();
    cv.polyline({{1878,829},{1880,883},{1903,934},{1930,967}},2,PAPER,.45*reveal);
    const auto road=catmull({{1890,925},{2050,880},{2220,820},{2380,750},{2290,700},
        {2230,660},{2520,600},{2740,570},{2590,545},{3000,515},{3320,493}},24);
    std::vector<V2> edge,other,centre;
    for(int j=0;j<int(road.size());++j) {
        const V2 d=road[std::min(j+1,int(road.size())-1)]-road[std::max(0,j-1)];
        const V2 n(-d.y/std::max(1.0,d.len()),d.x/std::max(1.0,d.len()));
        const double u=j/double(road.size()-1),w=lerp(42,1.3,std::pow(u,.61));
        edge.push_back(road[j]+n*w);other.push_back(road[j]-n*w);centre.push_back(road[j]);
    }
    cv.color(hex(0x181a28),reveal);cv.moveTo(edge[0].x,edge[0].y);
    for(auto q:edge)cv.lineTo(q.x,q.y);
    for(auto it=other.rbegin();it!=other.rend();++it)cv.lineTo(it->x,it->y);
    cv.closePath();cv.fill();
    // Warm edge light, amber centre markings, and paired guardrails.
    for(int j=1;j<int(edge.size());++j) {
        const double u=j/double(edge.size()-1),w=lerp(2.5,.3,std::pow(u,.65));
        cv.line(edge[j-1].x,edge[j-1].y,edge[j].x,edge[j].y,w,mix(pink,GOLD,.55),reveal*(.55+.2*sunset));
        cv.line(other[j-1].x,other[j-1].y,other[j].x,other[j].y,w*.8,mix(violet,pink,.45),.72*reveal);
    }
    for(int j=3;j<int(road.size())-2;j+=5) {
        const double u=j/double(road.size()-1),w=lerp(2.3,.45,u);
        cv.line(centre[j].x,centre[j].y,centre[j+2].x,centre[j+2].y,w,GOLD,.7*reveal);
    }
    std::vector<V2> rail1,rail2;
    for(int j=0;j<int(road.size());j+=3) {
        const double h=lerp(15,1.5,std::pow(j/double(road.size()-1),.65));
        rail1.push_back(edge[j]+V2(0,-h));rail2.push_back(other[j]+V2(0,-h));
        if(j%9==0) {
            cv.line(edge[j].x,edge[j].y,edge[j].x,edge[j].y-h,1.8,BLUE,reveal*.65);
            cv.line(other[j].x,other[j].y,other[j].x,other[j].y-h,1.8,BLUE,reveal*.5);
        }
    }
    cv.polyline(rail1,1.7,mix(GOLD,pink,.3),reveal*.8);cv.polyline(rail2,1.5,violet,reveal*.7);
    for(int j=125;j<int(road.size());j+=22) {
        V2 q=edge[j];double h=12*(1-j/double(road.size()))+3;
        cv.line(q.x,q.y,q.x,q.y-h,1.2,navy,reveal);
        cv.glow(q.x,q.y-h,5,GOLD,.45*reveal);cv.line(q.x-2,q.y-h,q.x+2,q.y-h,1.5,PAPER,.85*reveal);
    }
    // Broken warm strata carry the direction of the slope. Small ink
    // hatching gives the broad planes a material without filling them with dots.
    for(int k=0;k<95;++k) {
        const double x=1940+hash2(k,93)*1900,y=750+hash2(k,94)*330;
        const double len=25+100*hash2(k,95);
        cv.polyline({{x,y},{x+len*.4,y-4},{x+len,y-15}},.7+hash2(k,96),
            mix(violet,pink,.45),(.055+.085*hash2(k,97))*reveal);
    }
    // Slope strata, sparse silhouettes and foreground grass at three depths.
    for(int k=0;k<17;++k) {
        const double x=1960+hash2(k,20)*1600,y=670+hash2(k,21)*410;
        cv.polyline({{x,y},{x+65,y-9},{x+125,y-28}},1.2,mix(violet,pink,.25),.18*reveal);
    }
    for(int i=0;i<24;++i) {
        const double x=2020+hash2(i,40)*1400,y=650+hash2(i,41)*230,h=16+30*hash2(i,42);
        // Keep the walking shoulder clear.
        if(x<2420 && y>730)continue;
        cv.line(x,y,x,y-h,2,hex(0x17192b),reveal);
        for(int j=0;j<4;++j) {
            const double yy=y-h+j*h*.20,w=4+j*3;
            cv.color(hex(0x17192b),reveal);cv.moveTo(x,yy);cv.lineTo(x+w,yy+12);cv.lineTo(x-w,yy+12);cv.closePath();cv.fill();
        }
    }
    for(int i=0;i<70;++i) {
        const double x=2090+hash2(i,53)*1650,y=980+hash2(i,54)*95;
        cv.line(x,y,x+5*std::sin(i),y-8-12*hash2(i,55),1,mix(violet,pink,.3),.25*reveal);
    }
    c.gpu.over(cv);
}

BoatPose wreckBoatPose(const Ctx& c,bool near) {
    BoatPose p=seaBoatPose(c,near);
    const double pull=window(c.t,54,4,65.3,.7)*(near?110:45);
    p.at.x-=pull;
    const int row=near?5:3;
    const double l=swellY(p.at.x-100,row,c.t,&c),r=swellY(p.at.x+100,row,c.t,&c);
    const double enter=sstep(50,52,c.t);
    p.at.y=lerp(p.at.y,(l+r)*.5-13,enter);
    p.tilt=lerp(p.tilt,std::clamp(std::atan2(r-l,200),-.16,.16),enter);
    return p;
}

void wreckage(Ctx& c,double age) {
    if(age<0)return;
    Canvas& cv=c.canvas();
    Ctx impact=c;impact.t=c.waterContact;
    const BoatPose boat=wreckBoatPose(impact,true);
    // Matching halves keep their original hull silhouette, then rotate away
    // around a jagged central fracture. Flight settles onto the live sea.
    for(int half=0;half<2;++half) {
        const double side=half?1:-1;
        const double flight=std::min(age,1.4),settle=sstep(.7,1.8,age);
        const double x=boat.at.x+side*(100+180*flight);
        const double y=lerp(boat.at.y-180*flight+125*flight*flight,swellY(x,5,c.t,&c)+3,settle);
        cv.save();cv.translate(x,y);cv.rotate(boat.tilt+side*(.9*std::sin(std::min(1.0,age)*Pi*.7)*(1-settle)+.11));
        cv.color(OCHRE);
        if(half==0) {
            cv.moveTo(-124,-17);cv.curveTo(-90,12,-30,24,89,14);
            cv.lineTo(102,8);cv.lineTo(87,-2);cv.lineTo(102,-9);cv.lineTo(91,-14);cv.lineTo(103,-19);
            cv.curveTo(35,-1,-75,4,-124,-17);
        } else {
            cv.moveTo(-103,-19);cv.lineTo(-88,-11);cv.lineTo(-102,-4);cv.lineTo(-90,5);cv.lineTo(-102,11);
            cv.curveTo(-5,29,80,22,112,-8);cv.lineTo(122,-20);cv.curveTo(60,2,-20,2,-103,-19);
        }
        cv.closePath();cv.fill();
        cv.polyline({{-95,-7},{-45,4},{40,8},{93,-7}},2.2,GOLD,.65);cv.restore();
    }
    for(int i=0;i<18;++i) {
        const double flight=std::min(age,1.8),settle=sstep(1.0,2.1,age);
        const double vx=(hash2(i,71)-.45)*520,vy=-140-300*hash2(i,72);
        const double x=boat.at.x+vx*flight+10*std::sin(c.t*.7+i)*settle;
        const double y=lerp(boat.at.y+vy*flight+190*flight*flight,swellY(x,5,c.t,&c)+10,settle);
        const double angle=lerp((hash2(i,73)-.5)*3+flight*(hash2(i,74)-.5)*6,.07*std::sin(c.t+i),settle);
        cv.save();cv.translate(x,y);cv.rotate(angle);
        const double len=28+48*hash2(i,75);
        cv.color(i%4?OCHRE:GOLD);cv.moveTo(-len/2,-3);cv.lineTo(len/2-5,-3);cv.lineTo(len/2,0);cv.lineTo(len/2-4,4);cv.lineTo(-len/2,3);cv.closePath();cv.fill();
        if(i%4==0)cv.line(len/2-12,-3,len/2+14,-3,7,OCHRE);
        cv.restore();
    }
    // Five separated crew hold planks; heads and bent arms stay above water.
    for(int i=0;i<5;++i) {
        const double f=sstep(.15,1.5,age),x=boat.at.x-165+i*82+(i-2)*32*f;
        const double y=swellY(x,4+(i%2),c.t,&c)+12;
        RigIn r;r.h=62;r.hair=Hair::Short;r.garment=Garment::Jacket;r.facing=i%2?1:-1;
        r.hip={x,y+8};r.lean=.22*r.facing;r.footF={x+12,y+40};r.footB={x-8,y+42};
        r.handF={x+24,y-2};r.handB={x-20,y};r.headTilt=.1*std::sin(c.t+i);
        drawBody(cv,solve(r),KINK,sstep(.45,1.25,age));
        cv.line(x-33,y,x+35,y+2,6,OCHRE,sstep(.45,1.25,age));
        cv.line(x-28,y+1,x+28,y+3,1.5,PAPER,.35*sstep(.45,1.25,age));
    }
    c.gpu.over(cv);
}

void washedBearer(Ctx& c,double age) {
    if(age<0)return;
    const double wash=sstep(0,4.8,age),arrive=c.waterContact+4.8;
    Ctx born=c;born.t=c.waterContact;
    const BoatPose boat=wreckBoatPose(born,true);
    const V2 origin=boat.at+V2(166,-36);
    const double grow=sstep(2.0,5.0,age),h=lerp(74,118,grow);
    V2 hip=lerp(origin,{1916,905},wash);
    double roll=-1.50*sstep(.05,1.2,age)+.55*std::sin(age*4)*window(age,0,.2,4.8,1.0)+Tau*sstep(3.8,4.8,age);
    if(c.t>arrive) {
        const double u=c.t-arrive;
        roll=Tau+lerp(-1.5,0,sstep(1.15,2.65,u));
        hip.y=lerp(905,footpathY(1916)-hipHeight(h),sstep(1.15,2.65,u));
    }
    // The surge remains visibly underneath him through the approach and
    // spreads onto the sand, then recedes while he rests.
    if(age<5.45) {
        Canvas& surf=c.canvas();
        const double life=window(age,0,.12,5.45,.90);
        const double y=hip.y+std::abs(std::cos(roll))*h*.49+12;
        for(int patch=0;patch<5;++patch) {
            const double dx=-patch*32,w=70+patch*12,yy=y+patch*4;
            surf.color(PAPER,life*(.72-.09*patch));
            surf.moveTo(hip.x+dx-w,yy);
            surf.curveTo(hip.x+dx-w*.35,yy-10,hip.x+dx+w*.6,yy-20,hip.x+dx+w,yy-3);
            surf.curveTo(hip.x+dx+w*.3,yy+9,hip.x+dx-w*.5,yy+8,hip.x+dx-w,yy);surf.closePath();surf.fill();
            surf.polyline({{hip.x+dx-w*.6,yy+2},{hip.x+dx,yy+5},{hip.x+dx+w*.5,yy-1}},1.2,BLUE,life*.7);
        }
        c.gpu.over(surf);
    }
    RigIn r;r.h=h;r.hat=true;r.garment=Garment::Jacket;r.facing=1;
    r.hip={0,0};r.lean=.08;r.footF={.05*h,hipHeight(h)};r.footB={-.08*h,hipHeight(h)};
    r.handF={.24*h,-.18*h};r.handB={-.12*h,.06*h};
    // Reach to the ground during the push-up, then check the rescued light.
    const double push=window(c.t,arrive+1.15,.3,arrive+2.65,.35);
    r.handB=lerp(r.handB,{-.15*h,.28*h},push);
    r.headTilt=.20*window(c.t,arrive+2.0,.2,arrive+3.2,.4);
    r.flutter=.2*std::sin(c.t*4)*(1-sstep(4.8,7.8,age));
    const double walkStart=arrive+3.0,distance=320*sstep(walkStart,84.0,c.t);
    if(c.t>=walkStart) {
        hip.x=1916+distance;hip.y=footpathY(hip.x)-hipHeight(h);roll=0;
        Gait g;g.stride=48;g.lift=6;g.bob=1.8;
        const Steps st=gaitAt(1916,distance,925,1,g,1-sstep(83.6,84.5,c.t));
        r.footF={st.footF.x-hip.x,footpathY(st.footF.x)+st.footF.y-925-hip.y};
        r.footB={st.footB.x-hip.x,footpathY(st.footB.x)+st.footB.y-925-hip.y};
        r.hip.y=st.hipBob;r.handB={-.14*h+.035*h*std::sin(st.phase*Tau),.08*h};
    }
    if(c.t>=walkStart) for(V2 foot:{r.footF,r.footB}) {
        const double dx=foot.x-r.hip.x,leg=.505*h;
        r.hip.y=std::max(r.hip.y,foot.y-std::sqrt(std::max(0.0,leg*leg-dx*dx)));
    }
    Canvas& body=c.canvas();body.save();body.translate(hip.x,hip.y);body.rotate(roll);
    const Body pose=solve(r);drawBody(body,pose,KINK);
    // Jacket seam catches the lantern without losing the established hat.
    body.line(pose.shoulder.x,pose.shoulder.y,pose.chest.x+4,pose.chest.y+13,2.5,GOLD,.5);body.restore();c.gpu.over(body);
    const V2 reach=r.handF-pose.elbowF,hand=pose.elbowF+reach*std::min(1.0,.16*h/std::max(1.0,reach.len()));
    const double ca=std::cos(roll),sa=std::sin(roll);
    const V2 world=hip+V2(hand.x*ca-hand.y*sa,hand.x*sa+hand.y*ca);
    Canvas& light=c.canvas();drawBearerLantern(light,light,world,.08*std::sin(c.t*1.2),h/146,1+.35*c.hit(9));
    c.gpu.over(light,1.5);c.gpu.add(light,.28,12);
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
    if (t >= 50) {
        const double contact=t-c.waterContact;
        const int mask=shoreMask(c);
        c.gpu.beginOffscreen(true);
        if(k.wave)greatWave(c,k,br);
        waterComposite(c,c.gpu.endOffscreen(),mask);
        inlet(c,age);
        Canvas& boats=c.canvas();Canvas& lights=c.canvas();
        const BoatPose near=wreckBoatPose(c,true),far=wreckBoatPose(c,false);
        if(k.boats) {
            drawBoat(c,boats,lights,far.at,250,far.tilt+.10*ring(contact-.25,.28,1.2),6,k.grade,.7,false,stroke+.3);
            if(contact<0)drawBoat(c,boats,lights,near.at,400,near.tilt,5,k.grade,1,true,stroke);
        }
        c.gpu.over(boats);c.gpu.over(lights,1.5);c.gpu.add(lights,.4,16);
        if(contact>=0)wreckage(c,contact);
        c.gpu.beginOffscreen(true);
        if(k.wave)wash(c,contact,false);
        frontSwell(c,t,k.rise*(1-sstep(0,2.4,contact)));
        if(k.wave)wash(c,contact,true);
        waterComposite(c,c.gpu.endOffscreen(),mask);
        if(contact>=0)washedBearer(c,contact);
        // A short impact flash; the silhouette fracture is visible either
        // side of it and the following white crown lasts about .8 seconds.
        const double flash=window(contact,.04,.025,.14,.07);
        if(flash>0) {Canvas& light=c.canvas();light.fillRect(c.cameraX,0,1920,1080,PAPER,.76*flash);c.gpu.over(light);}
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
    c.cameraX=1640*sstep(66,84.0,t);
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
    drawTitle(title,2,window(t,82.5,.8,87,.6),hex(0xad8ee6),PAPER);
    c.gpu.finish(f, &title);
}
}
