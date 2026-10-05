#include "actors.h"
#include "palette.h"
#include "lanterns.h"
#include "sky-lanterns.h"
#include "town.h"
#include "onset.h"
#include <cmath>
namespace Journey::Kit {
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
}
