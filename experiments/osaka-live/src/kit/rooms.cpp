#include "rooms.h"
#include "primitives.h"
#include "pane.h"
#include "layout.h"

namespace Journey::Kit {
void OsakaNearRoomV1::draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, Canvas& w, double t, double ox, const NearPane (&U)[4], double (&lv)[4], double& room) {
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
void OsakaRightRoom2V1::draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, Canvas& w, double x0) {
    const double h2on[3] = {9.2, 12.0, 5.8};
    const double h2win[3][4] = {{22, 724, 62, 50}, {104, 724, 62, 50}, {24, 826, 96, 74}};
    for (int i = 0; i < 3; ++i) {
        const auto& q = h2win[i];
        const double lv = s.chapter ? paneLevel(c, L, h2on[i] + c.jit(300 + i) * 0.6, (i + 2) % 6, {x0 + q[0], q[1]}) : 1.0;
        if (lv > 0.01) {
            warmPane(w, x0 + q[0], q[1], q[2], q[3], lv);
            c.retain(w, "right-house-lattice-" + std::to_string(i), [&](Canvas& w) {
                lattice(w, x0 + q[0], q[1], q[2], q[3], 3, 2, INK, 1.2);
            }, {s.cam});
        } else darkPane(w, x0 + q[0], q[1], q[2], q[3]);
    }

}
}
