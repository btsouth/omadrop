#pragma once
#include "art.h"

namespace Journey {
struct Ctx;
class Canvas;
// Draw after sky/sun/Fuji/sea at movie time >= 69. Left rescue remains owner
// drawn. Water owns the bearer before 70.7; this function owns it thereafter.
// Does not finish, draw a title, move the camera, or sample live Audio.
void drawEndingRoad(Ctx& c);
// Water trajectory point, exactly (1100, 1010) from movie second 70.7.
V2 endingBearerWashAt(double t);
// Optional final-grade overlay. Tokyo Night title arrives at 81 and holds.
void drawEndingRoadTitle(Canvas& overlay, double t);
}
