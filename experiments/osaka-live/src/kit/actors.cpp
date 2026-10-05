#include "actors.h"
#include "palette.h"
#include "lanterns.h"
#include "sky-lanterns.h"
#include "town.h"
#include "onset.h"
#include <cmath>
namespace Journey::Kit {
double easedDistance(double t, double t0, double t1, double d0, double d1) {
    if (t <= t0) return d0;
    if (t >= t1) return d1;
    const double u = (t - t0) / (t1 - t0);
    // Accelerate over the first 12%, cruise, decelerate over the last 15%.
    const double a = 0.12, b = 0.15;
    const double v = 1.0 / (1 - a / 2 - b / 2);
    double s;
    if (u < a) s = v * u * u / (2 * a);
    else if (u < 1 - b) s = v * (a / 2 + (u - a));
    else { const double r = 1 - u; s = 1 - v * r * r / (2 * b); }
    return lerp(d0, d1, s);
}
double speedOf(double t, double t0, double t1, double d0, double d1) {
    return (easedDistance(t + 0.02, t0, t1, d0, d1) - easedDistance(t - 0.02, t0, t1, d0, d1)) / 0.04;
}


void OsakaWomanFanV1::draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& f, double t, double ox) {
    // Woman on the deck edge: opens her fan at 2 s and fans slowly.
    {
        const double h = 290, x = 260 + ox, y = 968;
        const double open = s.chapter ? backOut(clamp01((t - 2.15) / 0.35), 2.0) : 1.0;
        const double antic = s.chapter ? std::sin(Pi * clamp01((t - 1.85) / 0.3)) * (t < 2.15) : 0;
        const double fanPh = std::max(0.0, t - 2.6);
        const double fanning = (0.5 - 0.5 * std::cos(fanPh * Tau / 1.9)) * sstep(2.5, 3.2, t) * (1 - 0.6 * L.look);
        RigIn r;
        r.h = h; r.facing = 1; r.bun = true; r.robe = false; r.sleeve = true; r.obi = true; r.flutter = L.wind * std::sin(t * 2);
        r.hip = {x, y - 0.06 * h};
        r.lean = 0.04 + 0.03 * std::sin(t * 0.7) - 0.03 * L.look;
        r.footF = {x + 0.235 * h, y + 0.2 * h}; r.footB = {x + 0.22 * h, y + 0.21 * h};
        const V2 lap(x + 0.16 * h, y - 0.12 * h), raise(x + 0.2 * h, y - 0.27 * h);
        r.handF = lerp(lap, raise, clamp01(open)) + V2(0, 10 * antic) + V2(-4, -12) * fanning;
        const double wave = ActionWindow{9.2, 0.35, 10.25, 0.4}.gesture(c);
        r.handB = lerp(V2(x + 0.13 * h, y - 0.1 * h), V2(x + 0.15 * h + 8 * std::sin(t * 8), y - 0.48 * h), wave);
        const double glance = ActionWindow{9.0, 0.5, 10.6, 0.6}.gesture(c);
        r.headTilt = 0.05 * std::sin(t * 0.5) - 0.25 * glance + 0.8 * L.look;
        OsakaFigureV1::draw(f, r, INK);
        const double spread = 0.62 * clamp01(open) + 0.04;
        const double ang = -0.85 + 0.5 * fanning + 0.5 * (1 - clamp01(open));
        f.save();
        f.translate(r.handF.x + 2, r.handF.y - 2);
        f.rotate(ang);
        f.color(INK);
        f.moveTo(0, 0);
        f.arc(0, 0, 48, -spread, spread);
        f.closePath();
        f.fill();
        f.restore();
    }
}
void OsakaTeaV1::draw(Ctx& c, const OsakaEventState& L, Canvas& sh, double ox) {
        const double t=c.schedule->action(Moment::Tea,c.t,9.5);
        // Tea-pourer: walks in, lifts the kettle, pours, sets it down, sips.
        const double enter = 1;
        const double next = 0;
        const double hx = lerp(40, 158, enter) + next + ox;
        const double walking=0;
        Gait g; g.stride = 160; g.lift = 14; g.bob = 5;
        const Steps st = gaitAt(40 + ox, 118 * enter + next, 640, 1, g, walking);
        const double bob = st.hipBob;
        const double lift = sstep(9.5, 10.2, t) * (1 - sstep(12.6, 13.3, t));
        const double tilt = ActionWindow{10.4, 0.5, 12.4, 0.4}.at(t);
        const double sip = ActionWindow{14.6, 0.6, 16.2, 0.6}.at(t);
        RigIn r;
        r.h = 300; r.facing = 1; r.obi = true; r.flutter = 0.16 * std::sin(c.t * 1.8) + 0.4 * walking; r.robe = true; r.bun = true; r.sleeve = true;
        r.lean = 0.10 + 0.04 * tilt + 0.05 * ActionWindow{16.6, 0.25, 17.1, 0.2}.at(t) - 0.05 * L.look;
        r.hip = {hx, 640 - hipHeight(300) + bob};
        r.footF = st.footF; r.footB = st.footB;
        const V2 low(hx + 40, 470), kettleUp(hx + 88, 446 - 10 * tilt);
        r.handF = lerp(low, kettleUp, lift);
        r.handF = lerp(r.handF, V2(hx + 30, 400), sip);
        r.handB = lerp(V2(hx + 20, 480), V2(hx + 60, 470), lift);
        const double slide = ActionWindow{19.0, 0.4, 19.8, 0.35}.at(t);
        r.handF = lerp(r.handF, V2(hx + 42 - 25 * sstep(19.4, 20.0, t), 435), slide);
        r.handF = lerp(r.handF, V2(hx + 18, 474), L.look);
        r.headTilt = -0.2 * tilt + 0.15 * sip + 0.5 * L.look;
        OsakaFigureV1::draw(sh, r, SHADOW);
        if (t < 13.4 && sip < 0.5) {
            const V2 k = r.handF + V2(6, 0);
            const double ang = 0.65 * tilt;
            sh.save();
            sh.translate(k.x, k.y);
            sh.rotate(ang);
            sh.color(SHADOW); sh.ellipse(0, 0, 20, 15); sh.fill();
            sh.line(16, -4, 34, 8, 5, SHADOW);
            sh.color(SHADOW); sh.arc(0, -12, 13, 3.4, 6.0); sh.stroke(3);
            sh.restore();
            if (tilt > 0.6) {
                const V2 spout = k + V2(std::cos(ang) * 34 - std::sin(ang) * 8, std::sin(ang) * 34 + std::cos(ang) * 8);
                sh.line(spout.x, spout.y, spout.x + 4, 498, 2.2 * (tilt - 0.6) / 0.4, SHADOW);
            }
        } else if (t > 14.3 && t < 17.2) {
            sh.fillRect(r.handF.x - 6, r.handF.y - 10, 12, 12, SHADOW);
        }
        if (t > 17.2) sh.fillRect(200 + ox, 487, 12, 12, SHADOW);
        // The tea is left on a low shelf; the paper door slides a little.
        sh.fillRect(187 + ox, 501, 64, 5, SHADOW);
        const double door = 20 * ActionWindow{19.1, 0.5, 21.0, 0.8}.at(t);
        sh.fillRect(260 + ox - door, 312, 3, 196, SHADOW, 0.55);
}
void OsakaPatronsV1::draw(Ctx& c, const OsakaEventState& L, Canvas& f, double t, double x0) {
    // Patrons: lean, gesture and drink, each on their own clock.
    for (int i = 0; i < 3; ++i) {
        const double px = x0 + (i == 0 ? 98 : i == 1 ? 152 : 214);
        const double face = i == 1 ? -1 : 1;
        const double ph = t * (0.7 + 0.13 * i) + i * 2.1;
        // A shared toast starts on the first strong onset in this phrase.
        const double toastAt=c.schedule->moments[int(Moment::Toast)].start;
        const double toastAge=c.schedule->action(Moment::Toast,t,0);
        const double order=c.schedule->parameter(Moment::Toast,1,0,2,0);
        const double delay=std::fmod(i+std::floor(order),3.0)*0.08;
        const double toast=c.schedule->moments[int(Moment::Toast)].cycle<=1
            ? window(t,toastAt+i*0.08,0.24,toastAt+0.7,0.35)
            : window(toastAge,delay,0.24,0.7,0.35);
        const double drink = std::max(toast, std::pow(std::max(0.0, std::sin(ph)), 6) * (1 - L.hush));
        const double laugh = std::max(0.0, std::sin(t * 0.43 + i * 1.9)) * 0.08;
        RigIn r;
        r.h = 104; r.facing = face;
        r.hair = i == 1 ? Hair::Ponytail : Hair::Short;
        r.garment = i == 2 ? Garment::Happi : Garment::Jacket;
        r.flutter = 0.25 * std::sin(t * 2 + i) + L.wind * 0.5;
        r.sleeve = i == 1; r.obi = i == 1;
        r.hip = {px, 902 - 6};
        r.lean = 0.05 + laugh + 0.05 * std::sin(ph * 0.5) + 0.10 * window(t, toastAt + 1, 0.3, toastAt + 1.6, 0.5);
        r.footF = {px + face * 22, 902 + 22}; r.footB = {px + face * 16, 902 + 24};
        r.handF = {px + face * (18 + 6 * drink), 902 - 30 - 28 * drink};
        r.handB = {px + face * 14, 902 - 22};
        r.headTilt = 0.12 * drink + L.look * 0.5;
        OsakaFigureV1::draw(f, r, INK);
        if (drink > 0.02) f.fillRect(px + face * (20 + 6 * drink) - 3, 902 - 40 - 28 * drink, 6, 9, INK);
    }
}
void OsakaCookV1::draw(Ctx& c, const OsakaEventState& L, Canvas& p, double t, double yx) {
    // Cook: ladles in a loop once the cart opens; passes a bowl at 12 s.
    {
        const double cookT=c.schedule->action(Moment::Cook,t,11.4);
        const double step=ActionWindow{11.4, 0.75, 13.3, 0.7}.at(cookT);
        const double cx = yx + 72 + 68 * step;
        const double cadence=c.schedule->parameter(Moment::Cook,1,2.3,2.9,2.6);
        const double ph = std::fmod(std::max(0.0, t - 5.6), cadence) / cadence;
        V2 hand;
        // Hand targets sit where the ladle tip (hand + 18, 20) meets the pot
        // or the bowl, so the elbow stays bent instead of pointing.
        const V2 pot(yx + 108, 814), bowl(yx + 92, 826), rest(cx + 20, 846);
        if (t < 5.6) hand = rest;
        else if (ph < 0.3) hand = lerp(rest, pot, easeInOut(ph / 0.3));
        else if (ph < 0.45) hand = pot + V2(2 * std::sin(Tau * (ph - 0.3) / 0.15), 7 * std::sin(Pi * (ph - 0.3) / 0.15));
        else if (ph < 0.7) hand = lerp(pot, bowl, easeInOut((ph - 0.45) / 0.25));
        else if (ph < 0.85) hand = bowl + V2(0, 3 * std::sin(Pi * (ph - 0.7) / 0.15));
        else hand = lerp(bowl, rest, easeInOut((ph - 0.85) / 0.15));
        const double serve = ActionWindow{11.9, 0.4, 12.9, 0.4}.at(cookT);
        const double nod = ActionWindow{13.05, 0.18, 13.25, 0.32}.at(cookT);
        hand = lerp(hand, rest, L.hush);
        hand = lerp(hand, V2(yx + 170, 838), serve);
        RigIn r;
        r.h = 108; r.facing = 1; r.hair = Hair::Short; r.garment = Garment::Happi; r.shoulderTowel = true; r.flutter = L.wind * std::sin(t * 3); r.lean = 0.05 + 0.08 * serve + 0.05 * std::max(0.0, (hand.x - rest.x) / 20);
        r.hip = {cx, 902 - hipHeight(108)};
        Gait g; g.stride = 62; g.lift = 7; g.bob = 2;
        const double mv = clamp01(std::abs(68 * (window(cookT + 0.02, 11.4, 0.75, 13.3, 0.7) - window(cookT - 0.02, 11.4, 0.75, 13.3, 0.7)) / 0.04) / 35);
        const Steps feet = gaitAt(yx + 72, 68 * step, 898, 1, g, mv);
        r.footF = feet.footF; r.footB = feet.footB; r.hip.y += feet.hipBob;
        r.handF = hand; r.handB = {cx + 16 + 2 * std::sin(t * 1.7), 852};
        r.headTilt = -0.25 + 0.2 * serve - 0.38 * nod + L.look * 0.6;
        r.lean = std::min(r.lean, 0.18);
        // Keep the ladle inside the rig's reach through the serving step.
        const V2 shoulder = solve(r).shoulder;
        const V2 reach = hand - shoulder;
        if (reach.len() > 0.29 * r.h) hand = shoulder + reach * (0.29 * r.h / reach.len());
        r.handF = hand;
        OsakaFigureV1::draw(p, r, INK);
        // Headband knot, ladle and steaming pot.
        const Body bd = solve(r);
        p.line(bd.head.x - 9, bd.head.y - 6, bd.head.x - 19, bd.head.y - 1, 3.5, INK);
        if (serve < 0.5) {
            p.line(hand.x, hand.y, hand.x + 18, hand.y + 20, 3, INK);
            p.disc(hand.x + 20, hand.y + 23, 6, INK);
        } else {
            p.color(INK);
            p.arc(hand.x + 6, hand.y - 2, 10, 0, Pi);
            p.closePath();
            p.fill();
        }
        p.fillRect(yx + 104, 836, 46, 24, INK);
    }
}
void OsakaCustomerV1::draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& p, double t, double yx) {
    // Customer: arrives from the right, sits, takes the bowl and eats.
    if (s.chapter || t > 9) {
        const double sx = yx + 262;
        p.poly({{sx - 14, 936}, {sx - 10, 900}, {sx + 12, 900}, {sx + 16, 936}}, 3.5, INK);
        const double sit=1;
        {
            const double h=118;
            {
                // Sit with a small dip of anticipation, then eat in a loop.
                const double dip = 0;
                const double eat=std::pow(std::max(0.0,std::sin((t-12.6)*2.1)),3);
                const double hold = 1;
                RigIn r;
                r.h = h; r.facing = -1; r.hair = Hair::Short; r.garment = Garment::Jacket; r.flutter = 0.15 * std::sin(t * 2.2) + L.wind * 0.4;
                const V2 stand(sx + 10, 936 - hipHeight(h)), seat(sx, 898 - 0.06 * h);
                r.hip = lerp(stand, seat, sit) + V2(0, dip);
                r.lean = 0.12 * sit + 0.06 * eat;
                r.footF = {sx - 0.22 * h * sit - 6, 932}; r.footB = {sx - 0.19 * h * sit, 934};
                const V2 counter(sx - 40, 870), mouth(sx - 30, 820);
                r.handF = lerp(counter, mouth, eat * hold);
                r.handB = lerp(counter + V2(10, 6), mouth + V2(8, 6), eat * hold);
                r.headTilt = 0.15 * eat * hold - 0.1 * (1 - eat) - 0.38 * ActionWindow{13.4, 0.18, 13.65, 0.32}.gesture(c) + L.look * 0.6;
                OsakaFigureV1::street(p, r, BCYAN);
                if (hold > 0.5) {
                    const V2 bw = r.handF + V2(-4, -4);
                    p.color(INK); p.arc(bw.x, bw.y, 9, 0, Pi); p.closePath(); p.fill();
                    p.line(bw.x + 2, bw.y - 2, bw.x + 14, bw.y - 18, 1.6, INK);
                }
            }
        }
    }
}
void OsakaCoupleV1::draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& p, Canvas& l, double t, double ox) {
    // Couple stroll to the railing; one points at the moon, the other leans in.
    {
        const double bx = 1180 + ox;
        for (int k = 0; k < 2; ++k) {
            const double h = k == 0 ? 126 : 116;
            const double endX = k == 0 ? bx : bx - 48;
            const double startX = endX - 540;
            const double d = 540;
            const double mv = 0;
            Gait g; g.stride = 0.7 * h; g.lift = 0.06 * h; g.bob = 0.016 * h;
            const Steps st = gaitAt(startX, d, 936 - 0.04 * h, 1, g, mv);
            RigIn r;
            r.h = h; r.facing = 1; r.hair = k == 0 ? Hair::Short : Hair::Default; r.garment = k == 0 ? Garment::Jacket : Garment::Default; r.obi = k == 1; r.flutter = L.wind * std::sin(t * 2.4 + k) + 0.2 * mv;
            r.hip = {startX + d, 936 - hipHeight(h) + st.hipBob};
            r.footF = st.footF; r.footB = st.footB;
            const double arm = std::sin(Tau * st.phase) * mv;
            const double hold = (L.surge.t >= 0) ? window(t, L.surge.t + 1.2, 0.5, L.surge.t + 2.4, 0.25) : 0;
            const V2 lanternAt(bx - 24, 828);
            if (k == 0) {
                const double point = s.chapter ? ActionWindow{12.4, 0.5, 15.6, 0.7}.gesture(c) : 0;
                const double antic = 0;
                r.lean = 0.10 * (1 - point) + 0.02 * point + 0.05 * mv - 0.03 * antic;
                r.handF = lerp(r.hip + V2(0.05 * h + 0.07 * h * arm, 0.08 * h), r.hip + V2(0.24 * h, -0.52 * h), backOut(point, 1.2));
                // Points the firework out to her (it is up and to the left),
                // other hand on her shoulder.
                const double cheer = L.look;
                r.handF = lerp(r.handF, r.hip + V2(-0.12 * h, -0.54 * h), cheer);
                r.handB = lerp(r.hip + V2(-0.05 * h - 0.07 * h * arm, 0.08 * h), r.hip + V2(-0.16 * h, -0.30 * h), cheer);
                r.headTilt = 0.35 * point + 0.75 * L.look - 0.2 * ActionWindow{17.3, 0.3, 17.7, 0.4}.gesture(c);
                r.handF = lerp(r.handF, lanternAt + V2(10, 12), hold);
                r.handB = lerp(r.handB, lanternAt + V2(4, 14), hold);
            } else {
                const double lean = s.chapter ? ActionWindow{14.4, 0.8, 19.5, 1.0}.gesture(c) : 0;
                r.robe = true; r.bun = true; r.sleeve = true;
                r.lean = 0.12 + 0.12 * lean;
                r.handF = lerp(r.hip + V2(0.18 * h, -0.08 * h + 0.03 * h * arm), r.hip + V2(0.14 * h, -0.36 * h), L.look);
                r.handB = lerp(r.hip + V2(0.1 * h, -0.05 * h), r.hip + V2(0.10 * h, -0.33 * h), L.look);
                r.headTilt = 0.15 * lean + 0.7 * L.look + 0.1 - 0.22 * ActionWindow{17.8, 0.25, 18.15, 0.4}.gesture(c);
                r.handF = lerp(r.handF, lanternAt + V2(-8, 14), hold);
            }
            OsakaFigureV1::street(p, r, k == 0 ? WARM_T : RED);
        }
        OsakaCoupleLanternV1::draw(L, p, l, t, bx);
    }
}
void OsakaChildV1::draw(Ctx& c, const OsakaEventState& L, Canvas& p, double t, double ox) {
    // Child runs from the izakaya toward the flock, brakes and points.
    if (L.surge.t >= 0 && t > L.surge.t + 0.18) {
        const double start = L.surge.t + 0.18, h = 80;
        const double out=easedDistance(t,start,start+1.9,0,104);
        const double back=easedDistance(t,start+5.5,start+7.6,0,104);
        const double d=out-back;
        const double mv=clamp01((speedOf(t,start,start+1.9,0,104)+speedOf(t,start+5.5,start+7.6,0,104))/40);
        Gait g; g.stride = 48; g.lift = 9; g.bob = 3;
        const Steps st = gaitAt(1440+ox-(back>0?104:0),back>0?back:out,932,back>0?1:-1,g,mv);
        RigIn r; r.h = h; r.facing = back>0?1:-1; r.hair = Hair::Short; r.garment = Garment::Happi;
        r.hip = {1440 + ox - d, 936 - hipHeight(h) + st.hipBob};
        r.footF = st.footF; r.footB = st.footB;
        r.lean = 0.10 + 0.14 * mv; r.headTilt = 0.65 + 0.06 * std::sin(t * 3);
        const double point=window(t,start+1.65,0.5,start+5.5,0.8);
        r.handF = r.hip + V2(-14 - 6 * std::sin(Tau * st.phase) * mv, -8 - 36 * point);
        r.handB = r.hip + V2(6 + 9 * std::sin(Tau * st.phase) * mv, 4);
        r.flutter = 0.4 * mv + L.wind * std::sin(t * 4);
        if(t<start+7.8) OsakaFigureV1::street(p,r,RED);
    }
}
}
