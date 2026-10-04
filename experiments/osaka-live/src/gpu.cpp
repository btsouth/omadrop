#include "gpu.h"

#include <QTextStream>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace Journey {
namespace {
const char* fullscreenVs = R"(#version 330 core
out vec2 v_uv;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    v_uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

const char* effectHeader = R"(#version 330 core
in vec2 v_uv;
out vec4 o;
uniform vec2 u_size;
uniform float u_scale;
vec2 design() { return vec2(v_uv.x * 1920.0, (1.0 - v_uv.y) * 1080.0); }
float ss(float a, float b, float x) { float t = clamp((x - a) / (b - a), 0.0, 1.0); return t * t * (3.0 - 2.0 * t); }
float hash12(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash12(i), hash12(i + vec2(1, 0)), u.x), mix(hash12(i + vec2(0, 1)), hash12(i + vec2(1, 1)), u.x), u.y);
}
float fbm(vec2 p, int oct) {
    float a = 1.0, t = 0.0, s = 0.0;
    for (int i = 0; i < 8; ++i) {
        if (i >= oct) break;
        s += a * vnoise(p); t += a; a *= 0.5; p = p * 2.03 + vec2(17.1, 9.3);
    }
    return s / t;
}
)";

const char* canvasVs = R"(#version 330 core
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec4 a_col;
layout(location = 2) in vec4 a_aux;
out vec4 v_col;
out vec2 v_uv;
out vec2 v_pos;
flat out int v_mode;
flat out int v_paint;
void main() {
    v_col = a_col; v_uv = a_aux.xy; v_pos = a_pos;
    v_mode = int(a_aux.z + 0.5); v_paint = int(a_aux.w + 0.5);
    gl_Position = vec4(a_pos.x / 960.0 - 1.0, 1.0 - a_pos.y / 540.0, 0.0, 1.0);
}
)";

const char* canvasFs = R"(#version 330 core
in vec4 v_col;
in vec2 v_uv;
in vec2 v_pos;
flat in int v_mode;
flat in int v_paint;
out vec4 o;
uniform sampler2D u_paints;
vec4 T(int i) { return texelFetch(u_paints, ivec2(i, v_paint), 0); }
vec4 gradient() {
    vec4 h = T(0), m = T(1), e = T(2), q = T(3);
    vec2 u = vec2(m.x * v_pos.x + m.z * v_pos.y + e.x, m.y * v_pos.x + m.w * v_pos.y + e.y);
    float t;
    if (h.x < 1.5) { vec2 d = q.xy - e.zw; t = dot(u - e.zw, d) / max(dot(d, d), 1e-9); }
    else t = length(u - e.zw) / q.z;
    t = clamp(t, 0.0, 1.0);
    int n = int(h.y + 0.5);
    vec4 o0 = T(4), o1 = T(5);
    float offs[8] = float[8](o0.x, o0.y, o0.z, o0.w, o1.x, o1.y, o1.z, o1.w);
    vec4 prev = T(6);
    if (t <= offs[0]) return prev;
    for (int i = 1; i < 8; ++i) {
        if (i >= n) break;
        vec4 c = T(6 + i);
        if (t <= offs[i]) return mix(prev, c, (t - offs[i - 1]) / max(offs[i] - offs[i - 1], 1e-6));
        prev = c;
    }
    return prev;
}
void main() {
    if (v_mode == 1) {
        float d = length(v_uv);
        float f = d < 0.35 ? mix(1.0, 0.45, d / 0.35) : mix(0.45, 0.0, (d - 0.35) / 0.65);
        o = v_col * max(f, 0.0) * step(d, 1.0);
    } else if (v_mode == 2) {
        o = gradient() * v_col.a;
    } else {
        o = v_col;
    }
}
)";

const char* compositeFs = R"(
uniform sampler2D u_tex;
uniform float u_gain;
uniform float u_opacity;
void main() { vec4 c = texture(u_tex, v_uv); o = vec4(c.rgb * u_gain, c.a) * u_opacity; }
)";

const char* blurFs = R"(
uniform sampler2D u_tex;
uniform vec2 u_dir;
uniform float u_sigma;
void main() {
    int r = min(int(ceil(u_sigma * 3.0)), 24);
    vec4 s = texture(u_tex, v_uv);
    float wsum = 1.0;
    for (int i = 1; i <= 24; ++i) {
        if (i > r) break;
        float w = exp(-0.5 * float(i * i) / (u_sigma * u_sigma));
        s += w * (texture(u_tex, v_uv + u_dir * float(i)) + texture(u_tex, v_uv - u_dir * float(i)));
        wsum += 2.0 * w;
    }
    o = s / wsum;
}
)";

const char* downFs = R"(
uniform sampler2D u_tex;
void main() { o = texture(u_tex, v_uv); }
)";

const char* brightFs = R"(
uniform sampler2D u_tex;
uniform float u_threshold;
void main() { o = vec4(max(texture(u_tex, v_uv).rgb - u_threshold, 0.0), 1.0); }
)";

const char* finishFs = R"(
uniform sampler2D u_img, u_b0, u_b1, u_b2;
uniform float u_bloom, u_vig, u_grain, u_knee, u_paper, u_time;
void main() {
    vec3 c = texture(u_img, v_uv).rgb;
    vec2 p = design();
    if (u_paper > 0.0) {
        float n = fbm(p * vec2(300.0 / 1920.0, 170.0 / 1080.0), 2);
        float fib = fbm(p * vec2(12.0 / 1920.0, 400.0 / 1080.0) + vec2(3.0, 0.0), 2);
        c *= mix(1.0, 0.955 + 0.07 * n + 0.035 * fib, u_paper);
    }
    c += u_bloom * (0.55 * texture(u_b0, v_uv).rgb + 0.45 * texture(u_b1, v_uv).rgb + 0.4 * texture(u_b2, v_uv).rgb);
    vec2 q = v_uv * 2.0 - 1.0;
    c *= 1.0 - u_vig * ss(0.45, 1.5, sqrt(q.x * q.x * 0.8 + q.y * q.y));
    float k = u_knee;
    c = mix(c, k + (1.0 - k) * tanh((c - k) / (1.0 - k)), step(k, c));
    vec2 g = gl_FragCoord.xy + vec2(fract(u_time * 7.31) * 913.0, fract(u_time * 3.17) * 571.0);
    float n = (hash12(g) + hash12(g + 71.3) + hash12(g + 151.7) - 1.5) * 2.0;
    c += n * u_grain;
    o = vec4(clamp(c, 0.0, 1.0), 1.0);
}
)";

GLuint compile(GLenum type, const std::string& src, const char* name) {
    GLuint s = glCreateShader(type);
    const char* p = src.c_str();
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[8192];
        glGetShaderInfoLog(s, sizeof log, nullptr, log);
        std::fprintf(stderr, "Shader %s failed:\n%s\n", name, log);
        std::abort();
    }
    return s;
}

GLuint link(const std::string& vs, const std::string& fs, const char* name) {
    GLuint p = glCreateProgram();
    GLuint a = compile(GL_VERTEX_SHADER, vs, name), b = compile(GL_FRAGMENT_SHADER, fs, name);
    glAttachShader(p, a);
    glAttachShader(p, b);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[8192];
        glGetProgramInfoLog(p, sizeof log, nullptr, log);
        std::fprintf(stderr, "Program %s failed:\n%s\n", name, log);
        std::abort();
    }
    glDeleteShader(a);
    glDeleteShader(b);
    return p;
}
}

GLint Program::loc(const char* name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) return it->second;
    const GLint l = glGetUniformLocation(id, name);
    cache_[name] = l;
    return l;
}
void Program::set(const char* n, float v) { glUniform1f(loc(n), v); }
void Program::set(const char* n, float x, float y) { glUniform2f(loc(n), x, y); }
void Program::set(const char* n, float x, float y, float z) { glUniform3f(loc(n), x, y, z); }
void Program::set(const char* n, float x, float y, float z, float w) { glUniform4f(loc(n), x, y, z, w); }
void Program::set(const char* n, int v) { glUniform1i(loc(n), v); }
void Program::setArray(const char* n, const float* v, int count, int components) {
    switch (components) {
    case 1: glUniform1fv(loc(n), count, v); break;
    case 2: glUniform2fv(loc(n), count, v); break;
    case 3: glUniform3fv(loc(n), count, v); break;
    default: glUniform4fv(loc(n), count, v); break;
    }
}

Gpu::~Gpu() {
    release();
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (quadVao_) glDeleteVertexArrays(1, &quadVao_);
    if (paints_) glDeleteTextures(1, &paints_);
}

bool Gpu::init(QString& error) {
    if (!glGetString(GL_VERSION)) { error = QStringLiteral("No current OpenGL context."); return false; }
    canvas_.id = link(canvasVs, canvasFs, "canvas");
    compositeP_.id = link(fullscreenVs, std::string(effectHeader) + compositeFs, "composite");
    blurP_.id = link(fullscreenVs, std::string(effectHeader) + blurFs, "blur");
    downP_.id = link(fullscreenVs, std::string(effectHeader) + downFs, "down");
    brightP_.id = link(fullscreenVs, std::string(effectHeader) + brightFs, "bright");
    finishP_.id = link(fullscreenVs, std::string(effectHeader) + finishFs, "finish");
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(8));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(24));
    glBindVertexArray(0);
    glGenVertexArrays(1, &quadVao_);
    glGenTextures(1, &paints_);
    glBindTexture(GL_TEXTURE_2D, paints_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return true;
}

void Gpu::release() {
    current_ = &main_;
    for (Target* t : {&main_, &layer_, &out_, &alt_}) {
        if (t->fbo) glDeleteFramebuffers(1, &t->fbo);
        if (t->color) glDeleteRenderbuffers(1, &t->color);
        if (t->depth) glDeleteRenderbuffers(1, &t->depth);
        *t = {};
    }
    if (outFbo_) glDeleteFramebuffers(1, &outFbo_);
    if (outTex_) glDeleteTextures(1, &outTex_);
    outFbo_ = outTex_ = 0;
    for (Tex& t : pool_) { glDeleteFramebuffers(1, &t.fbo); glDeleteTextures(1, &t.tex); }
    pool_.clear();
}

void Gpu::allocate() {
    GLint maxSamples = 4;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    samples_ = std::min<int>(maxSamples, w_ * h_ <= 2300000 ? 8 : 4);
    auto make = [&](Target& t, GLenum format) {
        glGenFramebuffers(1, &t.fbo);
        glGenRenderbuffers(1, &t.color);
        glGenRenderbuffers(1, &t.depth);
        glBindRenderbuffer(GL_RENDERBUFFER, t.color);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, format, w_, h_);
        glBindRenderbuffer(GL_RENDERBUFFER, t.depth);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_DEPTH24_STENCIL8, w_, h_);
        glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, t.color);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, t.depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::fprintf(stderr, "Incomplete multisampled framebuffer\n");
            std::abort();
        }
    };
    make(main_, GL_RGBA16F);
    make(layer_, GL_RGBA16F);
    make(alt_, GL_RGBA16F);
    make(out_, GL_RGBA8);
    glGenTextures(1, &outTex_);
    glBindTexture(GL_TEXTURE_2D, outTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w_, h_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenFramebuffers(1, &outFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, outFbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex_, 0);
}

int Gpu::acquire(int w, int h) {
    for (std::size_t i = 0; i < pool_.size(); ++i) {
        if (!pool_[i].used && pool_[i].w == w && pool_[i].h == h) { pool_[i].used = true; return int(i); }
    }
    Tex t;
    t.w = w; t.h = h; t.used = true;
    glGenTextures(1, &t.tex);
    glBindTexture(GL_TEXTURE_2D, t.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &t.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);
    pool_.push_back(t);
    return int(pool_.size()) - 1;
}

void Gpu::begin(int width, int height) {
    if (width != w_ || height != h_ || !main_.fbo) {
        release();
        w_ = width; h_ = height;
        allocate();
    }
    for (Tex& t : pool_) t.used = false;
    current_ = &main_;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0xff);
    glEnable(GL_MULTISAMPLE);
    bindMain();
    glClearColor(0, 0, 0, 1);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Gpu::bindMain() {
    glBindFramebuffer(GL_FRAMEBUFFER, current_->fbo);
    glViewport(0, 0, w_, h_);
}

void Gpu::setBlend(Blend blend, float gain) {
    switch (blend) {
    case Blend::Replace: glDisable(GL_BLEND); return;
    case Blend::Over:
        glEnable(GL_BLEND);
        glBlendColor(gain, gain, gain, 1);
        glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        return;
    case Blend::Add:
        glEnable(GL_BLEND);
        glBlendColor(gain, gain, gain, 1);
        glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_ONE, GL_ZERO, GL_ONE);
        return;
    case Blend::Multiply:
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
        return;
    }
}

void Gpu::drawCanvas(const Canvas& c) {
    if (c.empty()) return;
    glUseProgram(canvas_.id);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const auto& v = c.vertices();
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(v.size() * sizeof(Vertex)), v.data(), GL_STREAM_DRAW);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, paints_);
    const auto& g = c.gradients();
    if (!g.empty())
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 16, GLsizei(g.size()), 0, GL_RGBA, GL_FLOAT, g.data());
    else {
        static const GradientRow zero;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 16, 1, 0, GL_RGBA, GL_FLOAT, &zero);
    }
    canvas_.set("u_paints", 0);
    bool stencil=false;
    glDisable(GL_STENCIL_TEST);
    auto setStencil=[&](bool enabled) {
        if(stencil==enabled) return;
        if(enabled) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
        stencil=enabled;
    };
    for (const auto& cmd : c.commands()) {
        switch (cmd.kind) {
        case Canvas::CmdKind::Direct:
            setStencil(false);
            glDrawArrays(GL_TRIANGLES, cmd.first, cmd.count);
            break;
        case Canvas::CmdKind::StencilFill:
            setStencil(true);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glStencilFunc(GL_ALWAYS, 0, 0xff);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
            glDrawArrays(GL_TRIANGLES, cmd.first, cmd.count);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilFunc(GL_NOTEQUAL, 0, 0xff);
            glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
            glDrawArrays(GL_TRIANGLES, cmd.coverFirst, cmd.coverCount);
            break;
        case Canvas::CmdKind::StencilOnce:
            setStencil(true);
            glStencilFunc(GL_EQUAL, 0, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
            glDrawArrays(GL_TRIANGLES, cmd.first, cmd.count);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glStencilFunc(GL_ALWAYS, 0, 0xff);
            glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
            glDrawArrays(GL_TRIANGLES, cmd.first, cmd.count);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            break;
        }
    }
    setStencil(false);
    glBindVertexArray(0);
}

void Gpu::draw(const Canvas& canvas, Blend blend, float gain) {
    bindMain();
    setBlend(blend, gain);
    drawCanvas(canvas);
}

int Gpu::layer(const Canvas& canvas) {
    glBindFramebuffer(GL_FRAMEBUFFER, layer_.fbo);
    glViewport(0, 0, w_, h_);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    setBlend(Blend::Over, 1);
    drawCanvas(canvas);
    const int t = acquire(w_, h_);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, layer_.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pool_[t].fbo);
    glBlitFramebuffer(0, 0, w_, h_, 0, 0, w_, h_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    return t;
}

void Gpu::fullscreen() {
    glBindVertexArray(quadVao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

int Gpu::downsample(int src) {
    const int w = std::max(1, (pool_[src].w + 1) / 2), h = std::max(1, (pool_[src].h + 1) / 2);
    const int dst = acquire(w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, pool_[dst].fbo);
    glViewport(0, 0, w, h);
    glDisable(GL_BLEND);
    glUseProgram(downP_.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pool_[src].tex);
    downP_.set("u_tex", 0);
    fullscreen();
    return dst;
}

int Gpu::blurPass(int src, float sigma, bool horizontal) {
    const int w = pool_[src].w, h = pool_[src].h;
    const int dst = acquire(w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, pool_[dst].fbo);
    glViewport(0, 0, w, h);
    glDisable(GL_BLEND);
    glUseProgram(blurP_.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pool_[src].tex);
    blurP_.set("u_tex", 0);
    blurP_.set("u_sigma", sigma);
    blurP_.set("u_dir", horizontal ? 1.f / w : 0.f, horizontal ? 0.f : 1.f / h);
    fullscreen();
    return dst;
}

int Gpu::blurred(int tex, float sigmaDesign) {
    const float s = float(sigmaDesign * pixelScale());
    if (s < 0.3f) return tex;
    int cur = tex;
    float f = 1;
    while (s / f > 3.f && f < 32.f) {
        const int next = downsample(cur);
        if (cur != tex) pool_[cur].used = false;
        cur = next;
        f *= 2;
    }
    const int a = blurPass(cur, s / f, true);
    if (cur != tex) pool_[cur].used = false;
    const int b = blurPass(a, s / f, false);
    pool_[a].used = false;
    return b;
}

void Gpu::composite(int tex, Blend blend, float gain, float opacity) {
    bindMain();
    glUseProgram(compositeP_.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pool_[tex].tex);
    compositeP_.set("u_tex", 0);
    compositeP_.set("u_gain", gain);
    compositeP_.set("u_opacity", opacity);
    setBlend(blend, 1);
    fullscreen();
}

void Gpu::over(const Canvas& canvas, float gain, float blur, float opacity) {
    if (canvas.empty() || opacity <= 0.001f) return;
    if (blur <= 0 && opacity >= 0.999f) { draw(canvas, Blend::Over, gain); return; }
    int t = layer(canvas);
    const int b = blurred(t, blur);
    composite(b, Blend::Over, gain, opacity);
    pool_[t].used = false;
    pool_[b].used = false;
}

void Gpu::add(const Canvas& canvas, float gain, float blur) {
    if (canvas.empty() || gain <= 0) return;
    int t = layer(canvas);
    const int b = blurred(t, blur);
    composite(b, Blend::Add, gain, 1);
    pool_[t].used = false;
    pool_[b].used = false;
}

Program& Gpu::effect(const std::string& name, const char* body) {
    auto it = effects_.find(name);
    if (it != effects_.end()) return it->second;
    Program& p = effects_[name];
    p.id = link(fullscreenVs, std::string(effectHeader) + body, name.c_str());
    return p;
}

void Gpu::pass(Program& p, Blend blend, const std::function<void(Program&)>& setup, int target) {
    int w = w_, h = h_;
    if (target >= 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, pool_[target].fbo);
        w = pool_[target].w; h = pool_[target].h;
        glViewport(0, 0, w, h);
    } else bindMain();
    glUseProgram(p.id);
    p.set("u_size", float(w), float(h));
    p.set("u_scale", float(pixelScale()));
    if (setup) setup(p);
    setBlend(blend, 1);
    fullscreen();
}

void Gpu::beginOffscreen(bool transparent) {
    current_ = &alt_;
    bindMain();
    glClearColor(0, 0, 0, transparent ? 0.f : 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

int Gpu::endOffscreen() {
    const int t = snapshot();
    current_ = &main_;
    bindMain();
    return t;
}

int Gpu::snapshot() {
    const int t = acquire(w_, h_);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, current_->fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pool_[t].fbo);
    glBlitFramebuffer(0, 0, w_, h_, 0, 0, w_, h_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    return t;
}

void Gpu::bindTexture(int unit, int tex, Program& p, const char* name) {
    glActiveTexture(GLenum(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, pool_[tex].tex);
    p.set(name, unit);
}

void Gpu::finish(const FinishParams& f, const Canvas* overlay) {
    const int img = snapshot();
    int b0 = -1, b1 = -1, b2 = -1;
    if (f.bloom > 0) {
        const int bright = acquire(w_, h_);
        glBindFramebuffer(GL_FRAMEBUFFER, pool_[bright].fbo);
        glViewport(0, 0, w_, h_);
        glDisable(GL_BLEND);
        glUseProgram(brightP_.id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pool_[img].tex);
        brightP_.set("u_tex", 0);
        brightP_.set("u_threshold", f.threshold);
        fullscreen();
        b0 = blurred(bright, 5);
        b1 = blurred(bright, 22);
        b2 = blurred(bright, 70);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, out_.fbo);
    glViewport(0, 0, w_, h_);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_BLEND);
    glUseProgram(finishP_.id);
    finishP_.set("u_size", float(w_), float(h_));
    auto bind = [&](int unit, int t, const char* n) {
        glActiveTexture(GLenum(GL_TEXTURE0 + unit));
        glBindTexture(GL_TEXTURE_2D, pool_[t >= 0 ? t : img].tex);
        finishP_.set(n, unit);
    };
    bind(0, img, "u_img");
    bind(1, b0, "u_b0");
    bind(2, b1, "u_b1");
    bind(3, b2, "u_b2");
    finishP_.set("u_bloom", f.bloom > 0 ? f.bloom : 0.f);
    finishP_.set("u_vig", f.vignette);
    finishP_.set("u_grain", f.grain);
    finishP_.set("u_knee", f.knee);
    finishP_.set("u_paper", f.paper);
    finishP_.set("u_time", f.time);
    fullscreen();
    if (overlay && !overlay->empty()) {
        setBlend(Blend::Over, 1);
        drawCanvas(*overlay);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, out_.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outFbo_);
    glBlitFramebuffer(0, 0, w_, h_, 0, 0, w_, h_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Gpu::present(GLuint framebuffer) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, outFbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glBlitFramebuffer(0, 0, w_, h_, 0, 0, w_, h_, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void Gpu::readRgb(std::vector<unsigned char>& rgb) {
    std::vector<unsigned char> rgba(std::size_t(w_) * h_ * 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, outFbo_);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w_, h_, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    rgb.resize(std::size_t(w_) * h_ * 3);
    for (int y = 0; y < h_; ++y) {
        const unsigned char* s = rgba.data() + std::size_t(h_ - 1 - y) * w_ * 4;
        unsigned char* d = rgb.data() + std::size_t(y) * w_ * 3;
        for (int x = 0; x < w_; ++x) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d += 3; s += 4; }
    }
}
}
