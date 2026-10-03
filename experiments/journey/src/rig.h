#pragma once

// Silhouette rigs: jointed people with two-bone IK, a planted-foot gait,
// cats and birds. Poses are absolute joint targets; callers key and ease
// them over time. Everything draws in one colour so a figure reads as a
// single silhouette against light.
#include "canvas.h"

namespace Journey {
V2 ik(V2 root, V2 target, double l1, double l2, double bend);

enum class Hair { Default, Short, Ponytail };
enum class Garment { Default, Apron, Jacket, Happi };

struct RigIn {
    V2 hip;              // absolute hip joint
    double h = 140;      // standing height
    double facing = 1;   // +1 faces right, -1 left
    double lean = 0;     // torso lean toward facing, radians
    double headTilt = 0; // + looks up, - looks down
    double headTurn = 0; // 0 profile, 1 faces viewer (narrows the nose cue)
    V2 footF, footB;     // ankle targets
    V2 handF, handB;     // wrist targets
    bool robe = false, bun = false, sleeve = false, hat = false;
    double hatLift = 0;  // gust lifts the hat brim
    double bulk = 1;     // limb thickness multiplier
    double kneeBend = 1; // +1 natural (knees toward facing)
    Hair hair = Hair::Default;
    Garment garment = Garment::Default;
    double flutter = 0;  // signed wind/movement, -1..1
    bool obi = false, shoulderTowel = false;
};

struct Body {
    RigIn in;
    V2 chest, neck, head, shoulder;
    V2 kneeF, kneeB, elbowF, elbowB;
};

Body solve(const RigIn& in);
void drawBody(Canvas& c, const Body& b, Col col, double alpha = 1.0);

// Standing hip for a figure of height h whose feet rest at groundY.
inline double hipHeight(double h) { return h * 0.49; }

// Planted-foot gait. `distance` is how far the hip has travelled along +x
// (or -x when facing left); feet stay fixed in world space during stance.
struct Gait {
    double stride = 60;  // world distance per full cycle (two steps)
    double lift = 10;    // swing height
    double bob = 3;      // hip bob
};
struct Steps { V2 footF, footB; double hipBob, phase; };
Steps gaitAt(double startX, double distance, double groundY, double facing, const Gait& g, double motion);

struct CatPose {
    V2 pos;              // ground point under the body centre
    double s = 22;       // size
    double facing = 1;
    double sit = 0;      // 0 standing/walking, 1 sitting
    double distance = 0; // walked distance for the leg cycle
    double tailBase = 0.0, tailWave = 0.2, tailPhase = 0;
    double look = 0;     // + head up
    double crouch = 0;   // 0..1 lowers the body (stalk / startle)
};
void drawCat(Canvas& c, const CatPose& p, Col col, double alpha = 1.0);

void drawBirdPerched(Canvas& c, double x, double y, double s, Col col, double face, double dip = 0, double alpha = 1.0);
// flap 0 wings down, 1 wings up; spread scales the wingspan (0 folded).
void drawBirdFly(Canvas& c, double x, double y, double s, Col col, double flap, double ang = 0, double spread = 1, double alpha = 1.0);
}
