#include "rig.h"

#include <cmath>
#include <vector>

namespace Journey {
namespace {
V2 dirOf(double angle) { return {std::cos(angle), std::sin(angle)}; }
V2 rot(V2 v, double a) {
    const double c = std::cos(a), s = std::sin(a);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}

// Closed smooth outline through the points (Catmull-Rom as cubic Beziers).
void smoothClosed(Canvas& c, const std::vector<V2>& p) {
    const int n = int(p.size());
    c.moveTo(p[0].x, p[0].y);
    for (int i = 0; i < n; ++i) {
        const V2 p0 = p[std::size_t((i + n - 1) % n)], p1 = p[std::size_t(i)], p2 = p[std::size_t((i + 1) % n)],
                 p3 = p[std::size_t((i + 2) % n)];
        const V2 a = p1 + (p2 - p0) * (1.0 / 6), b = p2 - (p3 - p1) * (1.0 / 6);
        c.curveTo(a.x, a.y, b.x, b.y, p2.x, p2.y);
    }
    c.closePath();
}

// Profile torso: a chest that leads, a waist, a back with a gentle curve and
// rounded shoulders, instead of a straight capsule.
void torso(Canvas& c, const Body& b, Col col, double alpha) {
    const RigIn& in = b.in;
    const double h = in.h, k = in.bulk;
    const V2 axis = b.chest - in.hip;
    const double len = std::max(1e-6, axis.len());
    const V2 up = axis * (1.0 / len);
    const V2 fwd = V2(-up.y, up.x) * in.facing;
    auto at = [&](double s, double d) { return in.hip + up * (s * h) + fwd * (d * h * k); };
    const std::vector<V2> pts = {
        at(-0.03, 0.058), at(0.07, 0.050), at(0.17, 0.066), at(0.25, 0.058), at(0.305, 0.030),
        at(0.315, -0.012), at(0.29, -0.052), at(0.20, -0.060), at(0.10, -0.050), at(0.02, -0.064), at(-0.04, -0.040),
    };
    c.color(col, alpha);
    smoothClosed(c, pts);
    c.fill();
}
}

V2 ik(V2 root, V2 target, double l1, double l2, double bend) {
    const V2 d = target - root;
    const double dist = std::clamp(d.len(), std::abs(l1 - l2) + 1e-4, l1 + l2 - 1e-4);
    const double a = std::acos(std::clamp((l1 * l1 + dist * dist - l2 * l2) / (2 * l1 * dist), -1.0, 1.0));
    const double base = std::atan2(d.y, d.x);
    return root + dirOf(base + bend * a) * l1;
}

Body solve(const RigIn& in) {
    Body b;
    b.in = in;
    const double h = in.h, f = in.facing;
    const V2 up(f * std::sin(in.lean), -std::cos(in.lean));
    b.chest = in.hip + up * (0.29 * h);
    b.shoulder = in.hip + up * (0.275 * h);
    b.neck = b.chest + up * (0.045 * h);
    const double headAngle = in.lean - in.headTilt * 0.45;
    b.head = b.neck + V2(f * std::sin(headAngle), -std::cos(headAngle)) * (0.085 * h);
    const double leg = 0.255 * h;
    b.kneeF = ik(in.hip, in.footF, leg, leg, -f * in.kneeBend);
    b.kneeB = ik(in.hip, in.footB, leg, leg, -f * in.kneeBend);
    b.elbowF = ik(b.shoulder, in.handF, 0.17 * h, 0.16 * h, f);
    b.elbowB = ik(b.shoulder, in.handB, 0.17 * h, 0.16 * h, f);
    return b;
}

void drawBody(Canvas& c, const Body& b, Col col, double alpha) {
    const RigIn& in = b.in;
    const double h = in.h, f = in.facing, k = in.bulk;
    const double flutter = std::clamp(in.flutter, -1.0, 1.0);
    const V2 up = (b.chest - in.hip) * (1.0 / std::max(1e-6, (b.chest - in.hip).len()));
    const V2 fwd = V2(-up.y, up.x) * f;
    auto atBody = [&](double s, double d) { return in.hip + up * (s * h) + fwd * (d * h * k); };
    auto limbEnd = [](V2, V2 joint, V2 target, double l2) {
        const V2 d = target - joint;
        const double L = d.len();
        return L > l2 ? joint + d * (l2 / L) : target;
    };
    const double leg = 0.255 * h;
    const V2 footF = limbEnd(in.hip, b.kneeF, in.footF, leg);
    const V2 footB = limbEnd(in.hip, b.kneeB, in.footB, leg);
    const V2 handF = limbEnd(b.shoulder, b.elbowF, in.handF, 0.16 * h);
    const V2 handB = limbEnd(b.shoulder, b.elbowB, in.handB, 0.16 * h);
    auto legDraw = [&](V2 knee, V2 foot) {
        c.capsule(in.hip, knee, 0.052 * h * k, 0.038 * h * k, col, alpha);
        c.capsule(knee, foot, 0.038 * h * k, 0.025 * h * k, col, alpha);
        c.capsule(foot, foot + V2(f * 0.07 * h, 0.025 * h), 0.022 * h * k, 0.016 * h * k, col, alpha);
    };
    auto armDraw = [&](V2 elbow, V2 hand, double phase) {
        c.capsule(b.shoulder, elbow, 0.033 * h * k, 0.026 * h * k, col, alpha);
        c.capsule(elbow, hand, 0.026 * h * k, 0.019 * h * k, col, alpha);
        c.disc(hand.x, hand.y, 0.024 * h * k, col, alpha);
        if (in.garment == Garment::Jacket || in.garment == Garment::Happi) {
            const bool happi = in.garment == Garment::Happi;
            const V2 cuff = lerp(elbow, hand, happi ? 0.34 : 0.24)
                          + V2(flutter * (happi ? 0.014 : 0.010) * h, 0);
            c.capsule(b.shoulder, cuff, (happi ? 0.049 : 0.044) * h * k, (happi ? 0.040 : 0.035) * h * k, col, alpha);
        }
        if (in.sleeve) {
            // Kimono sleeve hanging from the forearm.
            const V2 mid = lerp(elbow, hand, 0.35);
            const double sleeveSway = flutter * (0.016 + 0.002 * phase) * h;
            c.color(col, alpha);
            c.moveTo(elbow.x, elbow.y);
            c.lineTo(hand.x, hand.y);
            c.lineTo(mid.x - f * 0.02 * h + sleeveSway, mid.y + 0.15 * h);
            c.lineTo(elbow.x - f * 0.03 * h + sleeveSway * 0.7, elbow.y + 0.11 * h);
            c.closePath();
            c.fill();
        }
    };
    if (in.garment == Garment::Jacket || in.garment == Garment::Happi) {
        // Short coat: shoulder line to a lightly flared hem around the hip.
        const bool happi = in.garment == Garment::Happi;
        const V2 sF = atBody(0.275, happi ? 0.080 : 0.072);
        const V2 sB = atBody(0.275, happi ? -0.076 : -0.068);
        const V2 wF = atBody(0.020, happi ? 0.105 : 0.088);
        const V2 wB = atBody(0.020, happi ? -0.105 : -0.088);
        const V2 hF = atBody(-0.055, happi ? 0.115 : 0.095) + V2(flutter * 0.026 * h, 0);
        const V2 hB = atBody(-0.050, happi ? -0.115 : -0.095) + V2(flutter * 0.018 * h, 0);
        c.color(col, alpha);
        c.moveTo(sF.x, sF.y);
        c.quadTo(wF.x, wF.y, hF.x, hF.y);
        c.quadTo(atBody(-0.035, 0).x, hF.y + 0.008 * h, hB.x, hB.y);
        c.quadTo(wB.x, wB.y, sB.x, sB.y);
        c.quadTo(b.shoulder.x, b.shoulder.y - 0.014 * h, sF.x, sF.y);
        c.closePath();
        c.fill();
    }
    if (in.robe) {
        // Robe from shoulders to a hem that follows the feet.
        const double hemY = std::max(footF.y, footB.y) - 0.01 * h;
        const double x0 = std::min(footF.x, footB.x) - 0.055 * h, x1 = std::max(footF.x, footB.x) + 0.055 * h;
        const V2 side(-up.y, up.x);
        const V2 sL = b.chest + side * (0.085 * h), sR = b.chest - side * (0.085 * h);
        const V2 wL = in.hip + side * (0.075 * h), wR = in.hip - side * (0.075 * h);
        const V2 hemL(std::min(x0, wL.x - 0.01 * h) + flutter * 0.012 * h, hemY);
        const V2 hemR(std::max(x1, wR.x + 0.01 * h) + flutter * 0.026 * h, hemY);
        c.color(col, alpha);
        c.moveTo(sL.x, sL.y);
        c.curveTo(wL.x, wL.y - 0.05 * h, wL.x, wL.y + 0.05 * h, hemL.x, hemL.y);
        c.lineTo(hemR.x, hemR.y);
        c.curveTo(wR.x, wR.y + 0.05 * h, wR.x, wR.y - 0.05 * h, sR.x, sR.y);
        c.closePath();
        c.fill();
        c.capsule(footF, footF + V2(f * 0.06 * h, 0.022 * h), 0.02 * h, 0.015 * h, col, alpha);
        c.capsule(footB, footB + V2(f * 0.06 * h, 0.022 * h), 0.02 * h, 0.015 * h, col, alpha);
    } else {
        legDraw(b.kneeB, footB);
    }
    if (in.garment == Garment::Apron) {
        // Front panel with two rear ties, kept close to the body.
        const V2 top = atBody(0.060, 0.052);
        const V2 backTop = atBody(0.050, 0.024);
        const V2 front = atBody(0.005, 0.080);
        const V2 hemF = atBody(-0.145, 0.078) + V2(flutter * 0.015 * h, 0);
        const V2 hemB = atBody(-0.135, 0.026);
        c.color(col, alpha);
        c.moveTo(backTop.x, backTop.y);
        c.lineTo(top.x, top.y);
        c.quadTo(front.x, front.y, hemF.x, hemF.y);
        c.quadTo(atBody(-0.165, 0).x, hemF.y, hemB.x, hemB.y);
        c.closePath();
        c.fill();
        const V2 tieRoot = atBody(0.045, -0.052);
        const V2 tieA = atBody(0.015, -0.098) + V2(flutter * 0.014 * h, 0);
        const V2 tieB = atBody(0.070, -0.105) + V2(flutter * 0.018 * h, 0);
        c.capsule(tieRoot, tieA, 0.009 * h, 0.004 * h, col, alpha);
        c.capsule(tieRoot, tieB, 0.009 * h, 0.004 * h, col, alpha);
    }
    armDraw(b.elbowB, handB, -1);
    if (in.obi) {
        const std::vector<V2> obi = {
            atBody(0.055, 0.035), atBody(0.045, 0.068), atBody(0.005, 0.078), atBody(-0.035, 0.067),
            atBody(-0.050, 0.028), atBody(-0.045, -0.025), atBody(-0.005, -0.066), atBody(0.038, -0.068),
            atBody(0.058, -0.038),
        };
        c.color(col, alpha);
        smoothClosed(c, obi);
        c.fill();
    }
    torso(c, b, col, alpha);
    c.capsule(b.chest, b.neck, 0.026 * h * k, 0.022 * h * k, col, alpha);
    if (in.shoulderTowel) {
        const V2 front = b.shoulder + fwd * (0.055 * h) - up * (0.008 * h);
        const V2 back = b.shoulder - fwd * (0.065 * h) - up * (0.012 * h);
        const V2 tail = b.shoulder - fwd * (0.055 * h) - up * (0.155 * h) + V2(flutter * 0.024 * h, 0);
        c.capsule(front, back, 0.016 * h, 0.018 * h, col, alpha);
        c.capsule(back, tail, 0.020 * h, 0.008 * h, col, alpha);
    }
    if (!in.robe) legDraw(b.kneeF, footF);
    armDraw(b.elbowF, handF, 1);
    // Head with a small nose cue so the facing and gaze read in silhouette.
    const double tilt = -f * in.headTilt;
    const double headRot = tilt * 0.6 + f * in.lean * 0.5;
    auto headPoint = [&](double dx, double dy) { return b.head + rot(V2(f * dx * h, dy * h), headRot); };
    if (in.hair == Hair::Ponytail) {
        // A low tail sits behind the head and stays rounded at the tip.
        const V2 base = headPoint(-0.045, -0.010);
        const V2 mid = headPoint(-0.080, 0.055) + V2(flutter * 0.015 * h, 0);
        const V2 tip = headPoint(-0.070, 0.135) + V2(flutter * 0.032 * h, 0);
        c.capsule(base, mid, 0.020 * h, 0.015 * h, col, alpha);
        c.capsule(mid, tip, 0.015 * h, 0.008 * h, col, alpha);
    }
    if (in.hair == Hair::Short) {
        // A shallow cap follows the crown and back without adding ears or spikes.
        c.color(col, alpha);
        c.moveTo(headPoint(0.030, -0.060).x, headPoint(0.030, -0.060).y);
        c.quadTo(headPoint(0.012, -0.085).x, headPoint(0.012, -0.085).y,
                 headPoint(-0.012, -0.087).x, headPoint(-0.012, -0.087).y);
        c.quadTo(headPoint(-0.060, -0.080).x, headPoint(-0.060, -0.080).y,
                 headPoint(-0.074, -0.028).x, headPoint(-0.074, -0.028).y);
        c.quadTo(headPoint(-0.085, -0.005).x, headPoint(-0.085, -0.005).y,
                 headPoint(-0.060, 0.032).x, headPoint(-0.060, 0.032).y);
        c.quadTo(headPoint(-0.045, 0.040).x, headPoint(-0.045, 0.040).y,
                 headPoint(-0.028, 0.012).x, headPoint(-0.028, 0.012).y);
        c.quadTo(headPoint(0.005, -0.025).x, headPoint(0.005, -0.025).y,
                 headPoint(0.030, -0.060).x, headPoint(0.030, -0.060).y);
        c.closePath();
        c.fill();
    }
    c.color(col, alpha);
    c.ellipse(b.head.x, b.head.y, 0.064 * h, 0.075 * h, headRot);
    c.fill();
    const double nose = 0.012 * h * (1 - in.headTurn);
    if (nose > 0.3) {
        const V2 n0 = b.head + rot(V2(f * 0.05 * h, -0.02 * h), tilt);
        const V2 n1 = b.head + rot(V2(f * (0.064 * h + nose), 0.005 * h), tilt);
        const V2 n2 = b.head + rot(V2(f * 0.045 * h, 0.03 * h), tilt);
        c.tri(n0, n1, n2, col, alpha);
    }
    if (in.bun) {
        const V2 p = b.head + rot(V2(-f * 0.042 * h, -0.058 * h), tilt);
        c.disc(p.x, p.y, 0.036 * h, col, alpha);
    }
    if (in.hat) {
        const double ht = tilt * 0.7 - f * in.hatLift;
        const V2 lift(0, -0.06 * h * std::abs(in.hatLift));
        const V2 top = b.head + lift + rot(V2(0, -0.135 * h), ht);
        const V2 l = b.head + lift + rot(V2(-0.15 * h, -0.025 * h), ht);
        const V2 r = b.head + lift + rot(V2(0.15 * h, -0.025 * h), ht);
        c.color(col, alpha);
        c.moveTo(l.x, l.y);
        c.quadTo(top.x - 0.02 * h, top.y + 0.02 * h, top.x, top.y);
        c.quadTo(top.x + 0.02 * h, top.y + 0.02 * h, r.x, r.y);
        c.quadTo(b.head.x, b.head.y - 0.01 * h, l.x, l.y);
        c.closePath();
        c.fill();
    }
}

Steps gaitAt(double startX, double d, double groundY, double facing, const Gait& g, double motion) {
    Steps s;
    const double phi = d / g.stride;
    auto foot = [&](double offset) {
        const double q = phi + offset;
        const double k = std::floor(q), u = q - k;
        const double dk = (k - offset) * g.stride;
        double x, y = groundY;
        if (u < 0.6) x = dk + 0.3 * g.stride;
        else {
            const double t = (u - 0.6) / 0.4;
            const double e = 0.5 - 0.5 * std::cos(Pi * t);
            x = dk + g.stride * (0.3 + e);
            y -= g.lift * std::sin(Pi * t) * motion;
        }
        return V2(startX + facing * x, y);
    };
    s.footF = foot(0);
    s.footB = foot(0.5);
    s.hipBob = g.bob * std::cos(4 * Pi * (phi - 0.3)) * motion;
    s.phase = phi;
    return s;
}

void drawCat(Canvas& c, const CatPose& p, Col col, double alpha) {
    const double s = p.s, f = p.facing, x = p.pos.x, y = p.pos.y;
    const double k = clamp01(p.sit);
    const double crouch = p.crouch;
    // Body: a long low ellipse standing, an upright one sitting.
    const V2 standC(x, y - (0.5 - 0.14 * crouch) * s), sitC(x - f * 0.05 * s, y - 0.55 * s);
    const V2 bc = lerp(standC, sitC, k);
    const double rx = lerp(0.75, 0.36, k) * s, ry = lerp(0.26, 0.55, k) * s;
    const double walkBob = (1 - k) * 0.03 * s * std::sin(4 * Pi * p.distance / (1.1 * s));
    // Legs with a lateral-sequence gait (hind, fore, hind, fore).
    if (k < 0.98) {
        const double stride = 1.1 * s;
        const double offs[4] = {0.0, 0.25, 0.5, 0.75};
        const double dx[4] = {-0.5, 0.45, -0.38, 0.55};
        for (int i = 0; i < 4; ++i) {
            const double phi = p.distance / stride + offs[i];
            const double kk = std::floor(phi), u = phi - kk;
            double fx, fy = y;
            const double dk = (kk - offs[i]) * stride;
            if (u < 0.65) fx = dk + 0.25 * stride;
            else {
                const double t = (u - 0.65) / 0.35;
                fx = dk + stride * (0.25 + 0.5 - 0.5 * std::cos(Pi * t));
                fy -= 0.14 * s * std::sin(Pi * t);
            }
            V2 foot(x + f * (fx - p.distance + dx[i] * s), fy);
            V2 hip(x + f * dx[i] * s * 0.95, bc.y + 0.1 * s + walkBob);
            // Sitting: front legs straight under the chest, hind legs tuck away.
            const bool fore = dx[i] > 0;
            const V2 sitHip = fore ? V2(x + f * 0.12 * s, y - 0.55 * s) : V2(x - f * 0.1 * s, y - 0.2 * s);
            const V2 sitFoot = fore ? V2(x + f * (0.16 + 0.05 * (i == 3)) * s, y) : V2(x - f * 0.05 * s, y - 0.05 * s);
            hip = lerp(hip, sitHip, k);
            foot = lerp(foot, sitFoot, k);
            c.capsule(hip, foot, 0.075 * s, 0.05 * s, col, alpha);
        }
    }
    c.color(col, alpha);
    c.ellipse(bc.x, bc.y + walkBob, rx, ry, -f * 0.12 * k);
    c.fill();
    // Haunch when sitting.
    if (k > 0.05) c.disc(x - f * 0.12 * s, y - 0.28 * s, 0.3 * s * k, col, alpha);
    // Head.
    const V2 standH(x + f * 0.8 * s, y - (0.72 - 0.2 * crouch) * s), sitH(x + f * 0.12 * s, y - 1.12 * s);
    V2 hc = lerp(standH, sitH, k) + V2(-f * 0.05 * s * p.look, -0.06 * s * p.look);
    hc.y += walkBob;
    c.disc(hc.x, hc.y, 0.25 * s, col, alpha);
    c.disc(hc.x + f * 0.17 * s, hc.y + 0.06 * s - 0.04 * s * p.look, 0.13 * s, col, alpha);
    for (int e = -1; e <= 1; e += 2) {
        const V2 base0(hc.x + e * 0.2 * s, hc.y - 0.1 * s), base1(hc.x + e * 0.03 * s, hc.y - 0.2 * s);
        const V2 tip(hc.x + e * 0.15 * s - f * 0.03 * s * p.look, hc.y - 0.44 * s);
        c.tri(base0, tip, base1, col, alpha);
    }
    // Tail: a curling chain with a travelling wave.
    const V2 rear = bc + V2(-f * rx * 0.85, -ry * 0.2 * (1 - k) + ry * 0.75 * k);
    std::vector<V2> pts{rear};
    const int n = 12;
    const double len = 1.15 * s / n;
    for (int i = 0; i < n; ++i) {
        const double u = double(i) / n;
        const double standA = Pi + 0.35 + p.tailBase + u * 1.6;
        const double sitA = 2.35 - u * 2.5;
        double a = lerp(standA, sitA, k) + p.tailWave * std::sin(p.tailPhase - u * 2.4) * (0.3 + u);
        V2 d = dirOf(a);
        if (f < 0) d.x = -d.x;
        pts.push_back(pts.back() + d * len);
    }
    c.polyline(pts, 0.12 * s, col, alpha);
}

void drawBirdPerched(Canvas& c, double x, double y, double s, Col col, double face, double dip, double alpha) {
    c.color(col, alpha);
    c.ellipse(x, y - s * 0.55, s * 0.42, s * 0.62, -0.5 * face - dip * 0.6 * face);
    c.fill();
    const double hx = x + face * s * (0.26 + 0.25 * dip), hy = y - s * (1.12 - 0.55 * dip);
    c.disc(hx, hy, s * 0.3, col, alpha);
    c.tri({hx + face * s * 0.24, hy - s * 0.04}, {hx + face * s * 0.62, hy + s * 0.06}, {hx + face * s * 0.24, hy + s * 0.14}, col, alpha);
    c.line(x - face * s * 0.25, y - s * 0.2, x - face * s * 0.75, y + s * 0.45 - dip * s * 0.4, s * 0.2, col, alpha);
}

void drawBirdFly(Canvas& c, double x, double y, double s, Col col, double flap, double ang, double spread, double alpha) {
    c.save();
    c.translate(x, y);
    c.rotate(ang);
    c.color(col, alpha);
    const double up = (flap - 0.5) * 2;
    const double w = spread;
    for (int e = -1; e <= 1; e += 2) {
        c.moveTo(0, 0);
        c.curveTo(e * s * 0.4 * w, -s * 0.5 * up - s * 0.15, e * s * 0.8 * w, -s * 0.7 * up, e * s * 1.25 * w, -s * 0.25 * up);
        c.curveTo(e * s * 0.8 * w, -s * 0.45 * up + s * 0.12, e * s * 0.4 * w, -s * 0.2 * up + s * 0.14, 0, s * 0.14);
        c.fill();
    }
    c.ellipse(0, s * 0.04, s * 0.36, s * 0.14);
    c.fill();
    c.restore();
}
}
