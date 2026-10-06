// Theme titles: accent bar, name in the Omarchy mono face, small label.
// Fixed vector outlines (no font lookup), drawn after the grade.
#include "../world.h"

#include <charconv>
#include <cstring>
#include <vector>

namespace Journey {
namespace {
#include "../caption-assets/sign-outlines.inc"

struct Op { char code; double v[6]; };

std::vector<Op> parse(const char* data) {
    std::vector<Op> ops;
    const char* p = data;
    const char* end = data + std::strlen(data);
    auto num = [&] {
        while (p != end && *p == ' ') ++p;
        double v = 0;
        auto r = std::from_chars(p, end, v);
        p = r.ptr;
        return v;
    };
    while (p != end) {
        while (p != end && *p == ' ') ++p;
        if (p == end) break;
        Op op{*p++, {}};
        const int n = op.code == 'M' || op.code == 'L' ? 2 : op.code == 'Q' ? 4 : op.code == 'C' ? 6 : 0;
        for (int i = 0; i < n; ++i) op.v[i] = num();
        ops.push_back(op);
    }
    return ops;
}

void fillText(Canvas& c, const std::vector<Op>& ops, double x, double y, Col col, double a, double size = 1) {
    c.save();
    c.translate(x, y);
    c.scale(size, size);
    c.color(col, a);
    for (const Op& o : ops) {
        switch (o.code) {
        case 'M': c.moveTo(o.v[0], o.v[1]); break;
        case 'L': c.lineTo(o.v[0], o.v[1]); break;
        case 'Q': c.quadTo(o.v[0], o.v[1], o.v[2], o.v[3]); break;
        case 'C': c.curveTo(o.v[0], o.v[1], o.v[2], o.v[3], o.v[4], o.v[5]); break;
        case 'Z': c.closePath(); break;
        }
    }
    c.fill();
    c.restore();
}
}

void drawSignGlyph(Canvas& c, int index, double x, double y, double size, Col col, double alpha) {
    static const std::vector<std::vector<Op>> glyphs = [] {
        std::vector<std::vector<Op>> v;
        for (const char* g : signGlyphs) v.push_back(parse(g));
        return v;
    }();
    if (index < 0 || index >= int(glyphs.size()) || alpha <= 0.002) return;
    fillText(c, glyphs[std::size_t(index)], x, y, col, alpha, size);
}

}
