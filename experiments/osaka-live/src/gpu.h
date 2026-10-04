#pragma once

// GPU compositor for the Journey worlds. A multisampled half-float "main"
// image accumulates the scene like the mock's numpy image: canvases draw over
// or add into it, optionally through an offscreen layer with blur, gain and
// opacity, and full-screen shader passes add sky, fog, reflections and grading.
// finish() applies bloom, vignette, the soft knee and grain into an 8-bit
// multisampled output where the title is drawn last.
#include "canvas.h"
#include "glcore.h"
#include <QString>
#include <QRectF>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Journey {
enum class Blend { Over, Add, Replace, Multiply };

class Program {
public:
    GLuint id = 0;
    GLint loc(const char* name);
    void set(const char* name, float v);
    void set(const char* name, float x, float y);
    void set(const char* name, float x, float y, float z);
    void set(const char* name, float x, float y, float z, float w);
    void set(const char* name, Col c) { set(name, c.r, c.g, c.b); }
    void set(const char* name, int v);
    void setArray(const char* name, const float* v, int count, int components);
private:
    std::map<std::string, GLint> cache_;
};

struct FinishParams {
    float bloom = 0.55f, threshold = 0.62f;
    float vignette = 0.42f, grain = 0.014f, knee = 0.8f;
    float paper = 0.0f;  // woodblock paper fibre multiply
    float time = 0.0f;
};

class Gpu {
public:
    struct GeometryStats {
        std::uint64_t uploads = 0, staticUploads = 0, vertexBytes = 0, paintBytes = 0;
    };
    Gpu();
    ~Gpu();
    void clearGeometryCache();
    void setGeometryCacheEnabled(bool enabled) { cacheGeometry_ = enabled; }
    const GeometryStats& geometryStats() const { return geometryStats_; }
    bool init(QString& error);
    // Prepare the main image for a w x h output. Clears it to black.
    void begin(int width, int height);
    int width() const { return w_; }
    int height() const { return h_; }
    double pixelScale() const { return w_ / 1920.0; }

    void draw(const Canvas& canvas, Blend blend = Blend::Over, float gain = 1.f);
    // Offscreen: render, optionally blur, then composite.
    void over(const Canvas& canvas, float gain = 1.f, float blur = 0.f, float opacity = 1.f);
    void add(const Canvas& canvas, float gain = 1.f, float blur = 0.f);
    int layer(const Canvas& canvas);
    int blurred(int tex, float sigmaDesign);
    void composite(int tex, Blend blend, float gain = 1.f, float opacity = 1.f);

    // Full-screen effect programs: the body is appended to a shared header that
    // declares v_uv, design(), noise helpers and the output `o`.
    Program& effect(const std::string& name, const char* body);
    // Draw a full-screen pass into main (or into texture `target` if >= 0).
    void pass(Program& p, Blend blend, const std::function<void(Program&)>& setup, int target = -1, const QRectF& clip = {});
    // Resolve main into a pooled texture for passes that read the image.
    int snapshot();
    void bindTexture(int unit, int tex, Program& p, const char* name);

    // Redirect main drawing into a second image (one level), e.g. a world that
    // is graded and faded in as a whole; end returns it as a pooled texture.
    void beginOffscreen(bool transparent = false);
    int endOffscreen();

    // Final grade into the 8-bit output, then optional overlay canvas (title).
    void finish(const FinishParams& params, const Canvas* overlay);
    // Copy the finished output to an external framebuffer (Qt) or read it back.
    void present(GLuint framebuffer);
    void readRgb(std::vector<unsigned char>& rgb);

private:
    struct Target { GLuint fbo = 0, color = 0, depth = 0; };
    struct Tex { GLuint tex = 0, fbo = 0; int w = 0, h = 0; bool used = false; };

    void allocate();
    void release();
    int acquire(int w, int h);
    void bindMain();
    void drawCanvas(const Canvas& canvas);
    void bindGeometry(const Canvas& canvas);
    void setBlend(Blend blend, float gain);
    void fullscreen();
    int downsample(int src);
    int blurPass(int src, float sigmaPx, bool horizontal);

    struct Geometry {
        GLuint vao = 0, vbo = 0, paints = 0;
        std::uint64_t revision = 0,paintRevision=0;
    };
    std::map<std::uint64_t, Geometry> geometry_;
    GeometryStats geometryStats_;
    std::uint64_t dynamicVertexId_=0,dynamicVertexRevision_=0,dynamicPaintId_=0,dynamicPaintRevision_=0;
    bool cacheGeometry_ = true;
    int w_ = 0, h_ = 0, samples_ = 4;
    Target main_, layer_, out_, alt_;
    Target* current_ = &main_;
    GLuint outTex_ = 0, outFbo_ = 0;
    std::vector<Tex> pool_;
    GLuint vao_ = 0, vbo_ = 0, quadVao_ = 0, paints_ = 0, zeroPaints_ = 0;
    Program canvas_, compositeP_, blurP_, downP_, brightP_, finishP_;
    std::map<std::string, Program> effects_;
};
}
