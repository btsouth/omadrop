#pragma once

namespace Journey::Kit {
struct NearPane { double x, y, w, h; int cols, rows; double on; int band; };
    inline const NearPane nearPanes[4] = {{96, 312, 178, 196, 3, 4, 6.4, 1}, {284, 312, 104, 93, 2, 2, 9.6, 4},
                    {284, 415, 104, 93, 2, 2, -100, 2}, {398, 312, 96, 196, 2, 4, 3.9, 3}};
struct UpperPane { double wx, ww; bool cyan; double on; int band; };
    inline const UpperPane upperPanes[4] = {{30, 96, false, 16.4, 3}, {134, 96, false, 7.5, 1}, {262, 60, true, 4.8, 5}, {330, 110, false, 10.2, 0}};
}
