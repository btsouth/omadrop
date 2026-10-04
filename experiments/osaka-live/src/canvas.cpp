#include "canvas.h"

#include <cmath>

namespace Journey {
void Canvas::reset(double pixelScale) {
    ++revision_;
    frozen_ = false;
    preserveRaster=false;batchSpatially=false;
    retained_.clear();
    pixelScale_ = pixelScale;
    states_.assign(1, {});
    src_ = {};
    recyclePaths();
    hasCursor_ = false;
    verts_.clear();
    cmds_.clear();
    gradients_.clear();
}


void Canvas::append(const Canvas& retained) {
    const int index = int(retained_.size());
    retained_.push_back(&retained);
    cmds_.push_back({CmdKind::Cached, index, 0, 0, 0});
}

void Canvas::translate(double x, double y) {
    Affine& m = states_.back().m;
    m.e += m.a * x + m.c * y;
    m.f += m.b * x + m.d * y;
}

void Canvas::rotate(double t) {
    Affine& m = states_.back().m;
    const double cs = std::cos(t), sn = std::sin(t);
    const Affine o = m;
    m.a = o.a * cs + o.c * sn;
    m.b = o.b * cs + o.d * sn;
    m.c = -o.a * sn + o.c * cs;
    m.d = -o.b * sn + o.d * cs;
}

void Canvas::scale(double sx, double sy) {
    Affine& m = states_.back().m;
    m.a *= sx; m.b *= sx; m.c *= sy; m.d *= sy;
}

V2 Canvas::apply(double x, double y) const {
    const Affine& m = states_.back().m;
    return {m.a * x + m.c * y + m.e, m.b * x + m.d * y + m.f};
}

double Canvas::linearScale() const {
    const Affine& m = states_.back().m;
    return std::sqrt(std::abs(m.a * m.d - m.b * m.c));
}

void Canvas::color(Col c, double alpha) {
    const float a = float(std::clamp(alpha, 0.0, 1.0));
    src_ = {c.r * a, c.g * a, c.b * a, a, -1, a < 0.999f};
}

void Canvas::pushRow(const GradientRow& row, bool translucent) {
    ++revision_;
    gradients_.push_back(row);
    src_ = {1, 1, 1, 1, int(gradients_.size()) - 1, translucent};
}

namespace {
void fillStops(GradientRow& row, std::initializer_list<Stop> stops, bool& translucent) {
    int i = 0;
    translucent = false;
    for (const Stop& s : stops) {
        if (i >= 8) break;
        const float a = std::clamp(s.a, 0.f, 1.f);
        row.data[16 + i] = s.offset;
        row.data[24 + i * 4 + 0] = s.c.r * a;
        row.data[24 + i * 4 + 1] = s.c.g * a;
        row.data[24 + i * 4 + 2] = s.c.b * a;
        row.data[24 + i * 4 + 3] = a;
        if (a < 0.999f) translucent = true;
        ++i;
    }
    row.data[1] = float(i);
}
}

void Canvas::linear(double x0, double y0, double x1, double y1, std::initializer_list<Stop> stops) {
    GradientRow row;
    bool translucent = false;
    fillStops(row, stops, translucent);
    const Affine& m = states_.back().m;
    const double det = m.a * m.d - m.b * m.c;
    if (std::abs(det) < 1e-12) return;
    row.data[0] = 1;
    row.data[4] = float(m.d / det); row.data[5] = float(-m.b / det);
    row.data[6] = float(-m.c / det); row.data[7] = float(m.a / det);
    row.data[8] = float((m.c * m.f - m.d * m.e) / det);
    row.data[9] = float((m.b * m.e - m.a * m.f) / det);
    row.data[10] = float(x0); row.data[11] = float(y0);
    row.data[12] = float(x1); row.data[13] = float(y1);
    pushRow(row, translucent);
}

void Canvas::radial(double cx, double cy, double r, std::initializer_list<Stop> stops) {
    GradientRow row;
    bool translucent = false;
    fillStops(row, stops, translucent);
    const Affine& m = states_.back().m;
    const double det = m.a * m.d - m.b * m.c;
    if (std::abs(det) < 1e-12) return;
    row.data[0] = 2;
    row.data[4] = float(m.d / det); row.data[5] = float(-m.b / det);
    row.data[6] = float(-m.c / det); row.data[7] = float(m.a / det);
    row.data[8] = float((m.c * m.f - m.d * m.e) / det);
    row.data[9] = float((m.b * m.e - m.a * m.f) / det);
    row.data[10] = float(cx); row.data[11] = float(cy);
    row.data[14] = float(std::max(1e-6, r));
    pushRow(row, translucent);
}

void Canvas::recyclePaths() {
    for(auto& path:paths_) { path.pts.clear(); sparePaths_.push_back(std::move(path.pts)); }
    paths_.clear();
}
void Canvas::addSub() {
    paths_.push_back({});
    if(!sparePaths_.empty()) { paths_.back().pts=std::move(sparePaths_.back()); sparePaths_.pop_back(); }
}
Canvas::Sub& Canvas::current() {
    if (paths_.empty() || paths_.back().closed) {
        addSub();
        if (hasCursor_) paths_.back().pts.push_back(cursor_);
    }
    return paths_.back();
}

void Canvas::moveTo(double x, double y) {
    addSub();
    cursor_ = apply(x, y);
    cursorUser_ = {x, y};
    hasCursor_ = true;
    paths_.back().pts.push_back(cursor_);
}

void Canvas::lineTo(double x, double y) {
    if (!hasCursor_) { moveTo(x, y); return; }
    cursor_ = apply(x, y);
    cursorUser_ = {x, y};
    current().pts.push_back(cursor_);
}

void Canvas::curveTo(double x1, double y1, double x2, double y2, double x3, double y3) {
    if (!hasCursor_) moveTo(x1, y1);
    const V2 p0 = cursor_, p1 = apply(x1, y1), p2 = apply(x2, y2), p3 = apply(x3, y3);
    const V2 d1 = p0 - p1 * 2 + p2, d2 = p1 - p2 * 2 + p3;
    const double dd = std::max(d1.len(), d2.len()) * pixelScale_;
    const int n = std::clamp(int(std::ceil(std::sqrt(3 * dd / (4 * 0.22)))), 1, 120);
    Sub& s = current();
    for (int i = 1; i <= n; ++i) {
        const double t = double(i) / n, u = 1 - t;
        s.pts.push_back(p0 * (u * u * u) + p1 * (3 * u * u * t) + p2 * (3 * u * t * t) + p3 * (t * t * t));
    }
    cursor_ = p3;
    cursorUser_ = {x3, y3};
}

void Canvas::quadTo(double x1, double y1, double x2, double y2) {
    const double x0 = cursorUser_.x, y0 = cursorUser_.y;
    curveTo(x0 + 2.0 / 3 * (x1 - x0), y0 + 2.0 / 3 * (y1 - y0),
            x2 + 2.0 / 3 * (x1 - x2), y2 + 2.0 / 3 * (y1 - y2), x2, y2);
}

void Canvas::arc(double cx, double cy, double r, double a0, double a1) {
    while (a1 < a0) a1 += Tau;
    const double rd = std::max(0.5, r * linearScale() * pixelScale_);
    const double step = 2 * std::acos(std::max(-1.0, 1 - 0.22 / rd));
    const int n = std::clamp(int(std::ceil((a1 - a0) / std::max(step, 1e-3))), 2, 256);
    for (int i = 0; i <= n; ++i) {
        const double a = a0 + (a1 - a0) * i / n;
        const double x = cx + r * std::cos(a), y = cy + r * std::sin(a);
        if (i == 0) {
            if (hasCursor_ && !(paths_.empty() || paths_.back().closed)) lineTo(x, y);
            else moveTo(x, y);
        } else lineTo(x, y);
    }
}

void Canvas::closePath() {
    if (paths_.empty()) return;
    Sub& s = paths_.back();
    s.closed = true;
    if (!s.pts.empty()) cursor_ = s.pts.front();
}

void Canvas::rect(double x, double y, double w, double h) {
    moveTo(x, y); lineTo(x + w, y); lineTo(x + w, y + h); lineTo(x, y + h); closePath();
    paths_.back().knownConvex = w != 0 && h != 0;
}

void Canvas::ellipse(double cx, double cy, double rx, double ry, double rotation) {
    save();
    translate(cx, cy);
    rotate(rotation);
    scale(rx, ry);
    hasCursor_ = false;
    const double rd = std::max(0.5, linearScale() * pixelScale_);
    const double step = 2 * std::acos(std::max(-1.0, 1 - 0.22 / rd));
    const int n = std::clamp(int(std::ceil(Tau / std::max(step, 1e-3))), 2, 256);
    thread_local std::array<std::vector<V2>,257> circles;
    auto& circle=circles[n];
    if(circle.empty()) for(int i=0;i<=n;++i) {
        const double angle=Tau*i/n;
        circle.push_back({0.0+std::cos(angle),0.0+std::sin(angle)});
    }
    moveTo(circle[0].x,circle[0].y);
    for(std::size_t i=1;i<circle.size();++i) lineTo(circle[i].x,circle[i].y);
    closePath();
    // An affine ellipse is convex. Keep precisely the same points and fan;
    // avoid thousands of per-vertex atan2 checks every rendered frame.
    const auto& m = states_.back().m;
    paths_.back().knownConvex = paths_.back().pts.size() > 3 && std::abs(m.a*m.d-m.b*m.c) > 1e-12;
    restore();
    hasCursor_ = false;
}

Canvas::PreparedEllipse Canvas::prepareEllipse(double rx,double ry,double rotation,double pixelScale) {
    Canvas canvas(pixelScale);
    canvas.ellipse(0,0,rx,ry,rotation);
    const bool direct=canvas.convex(canvas.paths_.back());
    return {std::move(canvas.paths_.back().pts),canvas.paths_.back().knownConvex,direct};
}
void Canvas::ellipsePrepared(double x,double y,const PreparedEllipse& shape) {
    // Prepared shapes retain the original double precision affine products.
    // Their caller uses a translation-only canvas, so only the live origin changes.
    const V2 origin=apply(x,y);
    addSub();
    auto& sub=paths_.back();sub.closed=true;sub.knownConvex=shape.convex;
    for(const auto& p:shape.points)sub.pts.push_back({p.x+origin.x,p.y+origin.y});
    hasCursor_=false;
}

void Canvas::fillEllipsePrepared(double x,double y,const PreparedEllipse& shape) {
    const V2 origin=apply(x,y);
    const int first=int(verts_.size());
    const auto& pts=shape.points;
    auto moved=[&](V2 p) {return V2(p.x+origin.x,p.y+origin.y);};
    for(std::size_t i=1;i+1<pts.size();++i) {
        if(shape.direct) {put(moved(pts[0]));put(moved(pts[i]));put(moved(pts[i+1]));}
        else {putColor(moved(pts[0]),0,0,0,0,0,0,0,0);putColor(moved(pts[i]),0,0,0,0,0,0,0,0);putColor(moved(pts[i+1]),0,0,0,0,0,0,0,0);}
    }
    if(shape.direct)direct(first);
    else {
        V2 lo(1e30,1e30),hi(-1e30,-1e30);
        for(const auto& p:pts) {lo.x=std::min(lo.x,p.x);lo.y=std::min(lo.y,p.y);hi.x=std::max(hi.x,p.x);hi.y=std::max(hi.y,p.y);}
        lo=moved(lo);hi=moved(hi);
        const int cover=int(verts_.size());
        put(lo);put({hi.x,lo.y});put(hi);put(lo);put(hi);put({lo.x,hi.y});
        cmds_.push_back({CmdKind::StencilFill,first,cover-first,cover,6});
    }
    recyclePaths();hasCursor_=false;
}

void Canvas::putColor(V2 p, float r, float g, float b, float a, float u, float v, float mode, float paint) {
    ++revision_;
    const float x=float(p.x),y=float(p.y);
    verts_.push_back({x,y,r,g,b,a,u,v,mode,paint});
}

void Canvas::put(V2 p, float u, float v, float mode) {
    if (src_.gradient >= 0) putColor(p, src_.a, src_.a, src_.a, src_.a, u, v, 2, float(src_.gradient));
    else putColor(p, src_.r, src_.g, src_.b, src_.a, u, v, mode, 0);
}

void Canvas::direct(int first) {
    const int count = int(verts_.size()) - first;
    if (count <= 0) return;
    if (!cmds_.empty() && cmds_.back().kind == CmdKind::Direct
        && cmds_.back().first + cmds_.back().count == first) {
        cmds_.back().count += count;
        return;
    }
    cmds_.push_back({CmdKind::Direct, first, count, 0, 0});
}

bool Canvas::convex(const Sub& s) const {
    const auto& p = s.pts;
    const int n = int(p.size());
    if (n < 3) return false;
    int sign = 0;
    double turning = 0;
    double phaseX = 1, phaseY = 0;
    for (int i = 0; i < n; ++i) {
        const V2 a = p[i], b = p[(i + 1) % n], c = p[(i + 2) % n];
        const V2 e0 = b - a, e1 = c - b;
        const double cr = e0.x * e1.y - e0.y * e1.x;
        if (std::abs(cr) < 1e-9) continue;
        const int sg = cr > 0 ? 1 : -1;
        if (sign == 0) sign = sg;
        else if (sg != sign) return false;
        if (useKnownConvex && s.knownConvex) {
            const double dot=e0.x*e1.x+e0.y*e1.y;
            const double scale=std::max(std::abs(dot),std::abs(cr));
            const double re=dot/scale,im=cr/scale;
            const double nextX=phaseX*re-phaseY*im;
            phaseY=phaseX*im+phaseY*re;
            phaseX=nextX;
        } else
            turning += std::atan2(cr, e0.x * e1.x + e0.y * e1.y);
    }
    // An affine ellipse/rectangle has at most one winding. Compose its
    // accepted corner rotations and inspect the final angle once, retaining
    // the original 0.2-radian winding tolerance and endpoint degeneracy.
    if (useKnownConvex && s.knownConvex)
        return sign != 0 && std::abs(std::atan2(phaseY,phaseX)) < 0.2;
    return std::abs(std::abs(turning) - Tau) < 0.2;
}

void Canvas::fill() {
    auto& subs=fillSubs_;subs.clear();
    for (auto& s : paths_) if (s.pts.size() >= 3) subs.push_back(&s);
    if (!subs.empty()) {
        if (subs.size() == 1 && convex(*subs[0])) {
            const int first = int(verts_.size());
            const auto& p = subs[0]->pts;
            for (std::size_t i = 1; i + 1 < p.size(); ++i) { put(p[0]); put(p[i]); put(p[i + 1]); }
            direct(first);
        } else {
            const int first = int(verts_.size());
            double x0 = 1e30, y0 = 1e30, x1 = -1e30, y1 = -1e30;
            for (const Sub* s : subs) {
                const auto& p = s->pts;
                for (std::size_t i = 1; i + 1 < p.size(); ++i) {
                    putColor(p[0], 0, 0, 0, 0, 0, 0, 0, 0);
                    putColor(p[i], 0, 0, 0, 0, 0, 0, 0, 0);
                    putColor(p[i + 1], 0, 0, 0, 0, 0, 0, 0, 0);
                }
                for (const V2& q : p) {
                    x0 = std::min(x0, q.x); y0 = std::min(y0, q.y);
                    x1 = std::max(x1, q.x); y1 = std::max(y1, q.y);
                }
            }
            const int cover = int(verts_.size());
            put({x0, y0}); put({x1, y0}); put({x1, y1});
            put({x0, y0}); put({x1, y1}); put({x0, y1});
            cmds_.push_back({CmdKind::StencilFill, first, cover - first, cover, 6});
        }
    }
    recyclePaths();
    hasCursor_ = false;
}

void Canvas::fanArc(V2 c, double hw, double a0, double a1) {
    const double rd = std::max(0.5, hw * pixelScale_);
    const double step = 2 * std::acos(std::max(-1.0, 1 - 0.25 / rd));
    const int n = std::clamp(int(std::ceil(std::abs(a1 - a0) / std::max(step, 0.05))), 1, 64);
    V2 prev = c + V2(std::cos(a0), std::sin(a0)) * hw;
    for (int i = 1; i <= n; ++i) {
        const double a = a0 + (a1 - a0) * i / n;
        const V2 q = c + V2(std::cos(a), std::sin(a)) * hw;
        put(c); put(prev); put(q);
        prev = q;
    }
}

void Canvas::strokeGeometry(const Sub& s, double hw) {
    auto& p=strokePoints_;p.clear();
    for (const V2& q : s.pts)
        if (p.empty() || (q - p.back()).len() > 1e-6) p.push_back(q);
    if (s.closed && p.size() > 2 && (p.front() - p.back()).len() < 1e-6) p.pop_back();
    const int n = int(p.size());
    if (n == 1) { fanArc(p[0], hw, 0, Tau); return; }
    if (n < 2) return;
    const int segs = s.closed ? n : n - 1;
    auto dirAt = [&](int i) {
        const V2 d = p[(i + 1) % n] - p[i];
        return d * (1.0 / d.len());
    };
    for (int i = 0; i < segs; ++i) {
        const V2 a = p[i], b = p[(i + 1) % n], d = dirAt(i);
        const V2 nn(-d.y * hw, d.x * hw);
        put(a + nn); put(a - nn); put(b + nn);
        put(b + nn); put(a - nn); put(b - nn);
    }
    const int joins0 = s.closed ? 0 : 1, joins1 = s.closed ? n : n - 1;
    for (int i = joins0; i < joins1; ++i) {
        const V2 d0 = dirAt((i - 1 + n) % n), d1 = dirAt(i);
        const double cr = d0.x * d1.y - d0.y * d1.x, dt = d0.x * d1.x + d0.y * d1.y;
        if (std::abs(cr) < 1e-4 && dt > 0) continue;
        const double side = cr > 0 ? -1 : 1;
        const double a0 = std::atan2(d0.x * side, -d0.y * side);
        double a1 = std::atan2(d1.x * side, -d1.y * side);
        double delta = a1 - a0;
        while (delta > Pi) delta -= Tau;
        while (delta < -Pi) delta += Tau;
        fanArc(p[i], hw, a0, a0 + delta);
    }
    if (!s.closed) {
        const V2 d0 = dirAt(0), d1 = dirAt(n - 2);
        const double s0 = std::atan2(d0.x, -d0.y);
        fanArc(p[0], hw, s0, s0 + Pi);
        const double s1 = std::atan2(-d1.x, d1.y);
        fanArc(p[n - 1], hw, s1, s1 + Pi);
    }
}

void Canvas::stroke(double width) {
    double hw = 0.5 * width * linearScale();
    const Source saved = src_;
    const double device = 2 * hw * pixelScale_;
    if (device < 1.0) {
        // Keep hairlines stable: one device pixel wide, dimmed by coverage.
        const float k = float(std::max(0.0, device));
        src_.r *= k; src_.g *= k; src_.b *= k; src_.a *= k;
        src_.translucent = true;
        hw = 0.5 / pixelScale_;
    }
    const int first = int(verts_.size());
    for (const Sub& s : paths_) strokeGeometry(s, hw);
    const int count = int(verts_.size()) - first;
    if (count > 0) {
        if (src_.translucent) cmds_.push_back({CmdKind::StencilOnce, first, count, 0, 0});
        else direct(first);
    }
    src_ = saved;
    recyclePaths();
    hasCursor_ = false;
}

void Canvas::disc(double x, double y, double r, Col c, double a) {
    if (r <= 0 || a <= 0.002) return;
    color(c, a);
    ellipse(x, y, r, r);
    fill();
}

void Canvas::glowEllipse(double x, double y, double rx, double ry, Col c, double a) {
    if (a <= 0.002 || rx <= 0 || ry <= 0) return;
    const float k = float(std::min(a, 4.0));
    const int first = int(verts_.size());
    const V2 p00 = apply(x - rx, y - ry), p10 = apply(x + rx, y - ry);
    const V2 p11 = apply(x + rx, y + ry), p01 = apply(x - rx, y + ry);
    auto e = [&](V2 p, float u, float v) { putColor(p, c.r * k, c.g * k, c.b * k, std::min(k, 1.f), u, v, 1, 0); };
    e(p00, -1, -1); e(p10, 1, -1); e(p11, 1, 1);
    e(p00, -1, -1); e(p11, 1, 1); e(p01, -1, 1);
    direct(first);
}

void Canvas::glow(double x, double y, double r, Col c, double a) { glowEllipse(x, y, r, r, c, a); }

void Canvas::line(double x0, double y0, double x1, double y1, double w, Col c, double a) {
    if (a <= 0.002) return;
    color(c, a);
    moveTo(x0, y0);
    lineTo(x1, y1);
    stroke(w);
}

void Canvas::poly(std::initializer_list<V2> pts, double w, Col c, double a) {
    if (a <= 0.002) return;
    color(c, a);
    bool first = true;
    for (const V2& p : pts) {
        if (first) moveTo(p.x, p.y); else lineTo(p.x, p.y);
        first = false;
    }
    stroke(w);
}

void Canvas::polyline(const std::vector<V2>& pts, double w, Col c, double a) {
    if (a <= 0.002 || pts.size() < 2) return;
    color(c, a);
    moveTo(pts[0].x, pts[0].y);
    for (std::size_t i = 1; i < pts.size(); ++i) lineTo(pts[i].x, pts[i].y);
    stroke(w);
}

void Canvas::tri(V2 a, V2 b, V2 c, Col col, double alpha) {
    color(col, alpha);
    moveTo(a.x, a.y); lineTo(b.x, b.y); lineTo(c.x, c.y); closePath();
    fill();
}

void Canvas::fillRect(double x, double y, double w, double h, Col c, double a) {
    if (a <= 0.002) return;
    color(c, a);
    rect(x, y, w, h);
    fill();
}

void Canvas::capsule(V2 a, V2 b, double ra, double rb, Col c, double alpha) {
    if (alpha <= 0.002) return;
    const V2 d = b - a;
    const double L = d.len();
    color(c, alpha);
    if (L < 1e-6 || L <= std::abs(ra - rb)) {
        const double r = std::max(ra, rb);
        const V2 p = ra >= rb ? a : b;
        ellipse(p.x, p.y, r, r);
        fill();
        return;
    }
    // Outer tangent angle for circles of different radius.
    const double base = std::atan2(d.y, d.x);
    const double phi = std::acos(std::clamp((ra - rb) / L, -1.0, 1.0));
    const double rd = std::max(ra, rb) * linearScale() * pixelScale_;
    const int n = std::clamp(int(rd * 0.9), 6, 28);
    hasCursor_ = false;
    // Arc around b from base-phi to base+phi, then around a from base+phi to base-phi+2pi.
    for (int i = 0; i <= n; ++i) {
        const double t = base - phi + 2 * phi * i / n;
        const double x = b.x + rb * std::cos(t), y = b.y + rb * std::sin(t);
        if (i == 0) moveTo(x, y); else lineTo(x, y);
    }
    for (int i = 0; i <= n; ++i) {
        const double t = base + phi + (Tau - 2 * phi) * i / n;
        lineTo(a.x + ra * std::cos(t), a.y + ra * std::sin(t));
    }
    closePath();
    fill();
}
}
