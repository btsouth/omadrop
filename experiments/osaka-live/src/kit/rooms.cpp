#include "parameters.h"
#include "rooms.h"
#include "primitives.h"
#include "pane.h"
#include "layout.h"

namespace Journey::Kit {
void OsakaNearRoomV1::draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& w, double t, double ox, const NearPane (&U)[4], double (&lv)[4], double& room) {
    for (int i = 0; i < 4; ++i) {
        lv[i] = s.chapter ? paneLevel(c, L, U[i].on, U[i].band, {U[i].x + ox, U[i].y}) : 1.0;
        if (lv[i] > 0.01) warmPane(w, U[i].x + ox, U[i].y, U[i].w, U[i].h, lv[i]);
        else darkPane(w, U[i].x + ox, U[i].y, U[i].w, U[i].h);
    }
    // Ground floor: open engawa with a lit room behind.
    room = 1.0 + 0.08 * c.band(1) + 0.03 * std::sin(t * 1.1);
    w.linear(0, 640, 0, 968, {{0, mix(WARM_T, RED, 0.18) * float(room), 0.80f}, {0.55f, WARM_T * float(room), 0.92f},
                              {1, mix(WARM_T, RED, 0.1) * float(room), 0.86f}});
    w.rect(70 + ox, 640, 400, 328);
    w.fill();
    w.glow(270 + ox, 720, 150, WARM_B, 0.65);
    // The paper lamp in the room swells a little on the bass.
    w.glow(270 + ox, 716, 80, Col(1.0f, 0.86f, 0.62f), 0.40 * c.kick(5));

}
void OsakaRightRoom2V1::draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& w, double x0) {
    const auto& panes = osakaParameters().windows.right;
    for (int i = 0; i < 3; ++i) {
        const auto& q = panes[i];
        const double lv = s.chapter ? paneLevel(c, L, q.on + c.jit(300 + i) * 0.6, (i + 2) % 6, {x0 + q.x, q.y}) : 1.0;
        if (lv > 0.01) {
            warmPane(w, x0 + q.x, q.y, q.w, q.h, lv);
            c.retain(w, "right-house-lattice-" + std::to_string(i), [&](Canvas& w) {
                lattice(w, x0 + q.x, q.y, q.w, q.h, 3, 2, INK, 1.2);
            }, {s.cam});
        } else darkPane(w, x0 + q.x, q.y, q.w, q.h);
    }

}
void OsakaRightRoom3V1::draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& w, double t, double x0, const UpperPane (&ups)[4], double& shamisenPane) {
    for (int i = 0; i < 4; ++i) {
        const UpperPane& u = ups[i];
        double lv = s.chapter ? paneLevel(c, L, u.on, u.band, {x0 + u.wx, 690}) : 1.0;
        if (u.cyan) lv *= 0.6 + 0.3 * std::sin(t * 9) * std::sin(t * 2.3 + 1);
        if (i == 0) shamisenPane = lv;
        if (lv > 0.01) warmPane(w, x0 + u.wx, 640, u.ww, 96, lv, u.cyan ? &BCYAN : nullptr);
        else darkPane(w, x0 + u.wx, 640, u.ww, 96);
    }
    // Ground floor: open front, patrons inside.
    const double lamp = 0.95 + 0.06 * std::sin(t * 1.7) + 0.1 * c.band(2);
    w.linear(0, 800, 0, 936, {{0, WARM_T * float(lamp), 0.95f}, {1, mix(WARM_T, RED, 0.2) * float(lamp), 0.9f}});
    w.rect(x0 + 40, 800, 230, 136);
    w.fill();

}
void OsakaCartRoomV1::draw(Canvas& b, double yx, double yo) {
    if (yo > 0) {
        b.linear(0, 757, 0, 860, {{0, WARM_T, float(0.95 * yo)}, {1, mix(WARM_T, RED, 0.25), float(0.85 * yo)}});
        b.rect(yx + 9, 757, 160, 101);
        b.fill();
    }

}
double flickerOn(double dt) {
    if (dt < 0) return 0;
    if (dt < 0.05) return 0.55;
    if (dt < 0.11) return 0.08;
    if (dt < 0.17) return 0.8;
    if (dt < 0.22) return 0.35;
    return 0.75 + 0.25 * sstep(0.22, 0.6, dt);
}

double OsakaCartRoomV1::level(const OsakaState& s, double t) {
    return s.chapter ? std::min(1.0, flickerOn(t - 5.0)) : 1.0;
}
}
