#pragma once

// A small cairo-like vector canvas that tessellates on the CPU and draws on
// the GPU. Coordinates are design pixels (1920 x 1080); the GPU maps them to
// the output size, so art is resolution independent. Fills use the nonzero
// rule through the stencil buffer unless a contour is convex; strokes use
// round caps and joins. Everything is antialiased by multisampling.
#include "art.h"
#include <array>
#include <cstdint>
#include <initializer_list>
#include <vector>
#include <memory>

namespace Journey {
struct Vertex {
    float x, y;        // design pixels
    float r, g, b, a;  // premultiplied colour
    float u, v;        // local coordinates for sprites
    float mode, paint; // 0 solid, 1 glow sprite, 2 gradient
};

struct Stop { float offset; Col c; float a; };

struct GradientRow {
    std::array<float, 64> data{}; // original 16 RGBA32F texels, see gpu.cpp
    std::vector<Stop> extraStops; // SVG stops after the original eight
};

class Canvas {
public:
    inline static bool useKnownConvex = true;
    enum class CmdKind { Direct, StencilFill, StencilEvenOdd, StencilOnce, Cached };
    struct Cmd { CmdKind kind; int first, count, coverFirst, coverCount; float opacity = 1; };

    explicit Canvas(double pixelScale = 1.0) : pixelScale_(pixelScale) { states_.push_back({}); }
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;
    // Insert an immutable geometry span without copying or changing its place
    // in the draw stream. The retained canvas must outlive this canvas's frame.
    void append(const Canvas& retained);
    void appendOwned(std::shared_ptr<const Canvas> retained, double opacity = 1);
    const Canvas& retained(int index) const { return *retained_[index]; }
    std::uint64_t identity() const { return identity_; }
    std::uint64_t revision() const { return revision_; }
    void freeze() { frozen_ = true; }
    bool frozen() const { return frozen_; }
    bool preserveRaster = false;
    bool batchSpatially = false;
    void reset(double pixelScale);
    double pixelScale() const { return pixelScale_; }
    bool empty() const { return cmds_.empty(); }
    std::array<float,4> bounds() const;

    // Transform stack.
    void save() { states_.push_back(states_.back()); }
    void restore() { if (states_.size() > 1) states_.pop_back(); }
    void translate(double x, double y);
    void rotate(double radians);
    void scale(double sx, double sy);
    void transform(double a,double b,double c,double d,double e,double f);
    void gradientUserSpace(GradientRow row, bool translucent);

    // Source.
    void color(Col c, double alpha = 1.0);
    void gradient(const GradientRow& row, bool translucent) { pushRow(row, translucent); }
    void linear(double x0, double y0, double x1, double y1, std::initializer_list<Stop> stops);
    void radial(double cx, double cy, double r, std::initializer_list<Stop> stops);

    // Path construction (user space, transformed immediately).
    void moveTo(double x, double y);
    void lineTo(double x, double y);
    void curveTo(double x1, double y1, double x2, double y2, double x3, double y3);
    void quadTo(double x1, double y1, double x2, double y2);
    void arc(double cx, double cy, double r, double a0, double a1);
    void closePath();
    void rect(double x, double y, double w, double h);
    void ellipse(double cx, double cy, double rx, double ry, double rotation = 0);

    struct PreparedEllipse {std::vector<V2> points;bool convex=false;bool direct=false;};
    static PreparedEllipse prepareEllipse(double rx,double ry,double rotation,double pixelScale);
    void ellipsePrepared(double x,double y,const PreparedEllipse&);
    void fillEllipsePrepared(double x,double y,const PreparedEllipse&);

    void fill(bool evenOdd = false);
    void stroke(double width);
    void newPath() { recyclePaths(); }

    // Convenience shapes (each clears the current path).
    void disc(double x, double y, double r, Col c, double a = 1.0);
    void glow(double x, double y, double r, Col c, double a = 1.0);
    void glowEllipse(double x, double y, double rx, double ry, Col c, double a = 1.0);
    void line(double x0, double y0, double x1, double y1, double w, Col c, double a = 1.0);
    void poly(std::initializer_list<V2> pts, double w, Col c, double a = 1.0);
    void polyline(const std::vector<V2>& pts, double w, Col c, double a = 1.0);
    void tri(V2 a, V2 b, V2 c, Col col, double alpha = 1.0);
    void fillRect(double x, double y, double w, double h, Col c, double a = 1.0);
    // Tapered capsule: convex hull of two circles.
    void capsule(V2 a, V2 b, double ra, double rb, Col c, double alpha = 1.0);

    const std::vector<Vertex>& vertices() const { return verts_; }
    const std::vector<Cmd>& commands() const { return cmds_; }
    const std::vector<GradientRow>& gradients() const { return gradients_; }

private:
    struct Affine { double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0; };
    struct State { Affine m; };
    struct Source { float r = 0, g = 0, b = 0, a = 1; int gradient = -1; bool translucent = false; };
    struct Sub { std::vector<V2> pts; bool closed = false; bool knownConvex = false; };

    V2 apply(double x, double y) const;
    double linearScale() const;
    double tolerance() const { return 0.22 / pixelScale_; }
    Sub& current();
    void recyclePaths();
    void addSub();
    void put(V2 p, float u = 0, float v = 0, float mode = 0);
    void putColor(V2 p, float r, float g, float b, float a, float u, float v, float mode, float paint);
    void direct(int first);
    void strokeGeometry(const Sub& s, double hw);
    void fanArc(V2 c, double hw, double a0, double a1);
    bool convex(const Sub& s) const;
    void pushRow(const GradientRow& row, bool translucent);

    inline static std::uint64_t nextIdentity_ = 0;
    const std::uint64_t identity_ = ++nextIdentity_;
    std::uint64_t revision_ = 1;
    bool frozen_ = false;

    std::vector<const Canvas*> retained_;
    std::vector<std::shared_ptr<const Canvas>> owned_;
    double pixelScale_;
    std::vector<State> states_;
    Source src_;
    std::vector<Sub> paths_;
    std::vector<const Sub*> fillSubs_;
    std::vector<V2> strokePoints_;
    std::vector<std::vector<V2>> sparePaths_;
    V2 cursor_{}, cursorUser_{};
    bool hasCursor_ = false;
    std::vector<Vertex> verts_;
    std::vector<Cmd> cmds_;
    std::vector<GradientRow> gradients_;
};

}
