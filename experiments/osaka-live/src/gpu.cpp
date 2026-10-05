#include "gpu.h"

#include <QTextStream>
#include <cmath>
#include <cstdio>

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
    // Adjacent Gaussian taps share one linear-filtered fetch. Keep the
    // original finite support, including an unpaired last tap for odd radii.
    for (int i = 1; i <= 24; i += 2) {
        if (i > r) break;
        float w0 = exp(-0.5 * float(i * i) / (u_sigma * u_sigma));
        float w1 = i + 1 <= r ? exp(-0.5 * float((i+1)*(i+1)) / (u_sigma*u_sigma)) : 0.0;
        float weight = w0 + w1;
        float offset = float(i) + w1 / weight;
        s += weight * (texture(u_tex, v_uv + u_dir * offset) + texture(u_tex, v_uv - u_dir * offset));
        wsum += 2.0 * weight;
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

Gpu::Gpu() = default;
Gpu::~Gpu() {
    profile.close();
    release();
    clearGeometryCache();
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (quadVao_) glDeleteVertexArrays(1, &quadVao_);
    if (paints_) glDeleteTextures(1, &paints_);
    if (zeroPaints_) glDeleteTextures(1, &zeroPaints_);
}

bool Gpu::init(QString& error) {
    if (!glGetString(GL_VERSION)) { error = QStringLiteral("No current OpenGL context."); return false; }
    profile.init();
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
    glGenTextures(1,&zeroPaints_);glBindTexture(GL_TEXTURE_2D,zeroPaints_);
    const GradientRow zero;
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,16,1,0,GL_RGBA,GL_FLOAT,&zero);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    return true;
}

void Gpu::release() {
    clearRasterCache();
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
    if(scaledFbo_) glDeleteFramebuffers(1,&scaledFbo_);
    if(scaledTex_) glDeleteTextures(1,&scaledTex_);
    scaledFbo_=scaledTex_=0;
    for (Tex& t : pool_) { glDeleteFramebuffers(1, &t.fbo); glDeleteTextures(1, &t.tex); }
    pool_.clear();
    for(auto& entry:reducedLayers_) {
        auto& t=entry.second;
        glDeleteFramebuffers(1,&t.fbo); glDeleteRenderbuffers(1,&t.color); glDeleteRenderbuffers(1,&t.depth);
    }
    reducedLayers_.clear();

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
        bindFramebuffer(GL_FRAMEBUFFER, t.fbo);
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

    glGenTextures(1, &outTex_);
    glBindTexture(GL_TEXTURE_2D, outTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w_, h_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenFramebuffers(1, &outFbo_);
    bindFramebuffer(GL_FRAMEBUFFER, outFbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex_, 0);
    allocatedOutputW_=outputW_ ? outputW_ : w_;
    allocatedOutputH_=outputH_ ? outputH_ : h_;
    if(allocatedOutputW_!=w_ || allocatedOutputH_!=h_) {
        glGenTextures(1,&scaledTex_); glBindTexture(GL_TEXTURE_2D,scaledTex_);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,allocatedOutputW_,allocatedOutputH_,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glGenFramebuffers(1,&scaledFbo_); bindFramebuffer(GL_FRAMEBUFFER,scaledFbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,scaledTex_,0);
    }
}

int Gpu::acquire(int w, int h) {
    for (std::size_t i = 0; i < pool_.size(); ++i) {
        if (!pool_[i].used && !pool_[i].pinned && pool_[i].w == w && pool_[i].h == h) { pool_[i].used = true; pool_[i].reduction=1; pool_[i].bounds=QRect(0,0,w,h); return int(i); }
    }
    Tex t;
    t.w = w; t.h = h; t.used = true; t.bounds=QRect(0,0,w,h); t.dirty=t.bounds;
    glGenTextures(1, &t.tex);
    glBindTexture(GL_TEXTURE_2D, t.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &t.fbo);
    bindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);
    // Bounded passes sample transparent pixels outside their content. Define
    // the entire new allocation once; subsequent reuse only clears old content.
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
    pool_.push_back(t);
    return int(pool_.size()) - 1;
}

void Gpu::begin(int width, int height) {
    if (width != w_ || height != h_ || !main_.fbo || (outputW_ && outputW_!=allocatedOutputW_) || (outputH_ && outputH_!=allocatedOutputH_)) {
        release();
        w_ = width; h_ = height;
        allocate();
    }
    ++rasterFrame_;
    // Entries not drawn for two frames are no longer retained by this renderer.
    for(auto it=rasters_.begin();it!=rasters_.end();) {
        if(it->second.frame+2>=rasterFrame_) { ++it; continue; }
        const int texture=it->second.texture;
        pool_[texture].pinned=false;
        for(auto f=filters_.begin();f!=filters_.end();) {
            if(f->first.first==texture) { pool_[f->second].pinned=false; f=filters_.erase(f); }
            else ++f;
        }
        it=rasters_.erase(it);
    }
    for (Tex& t : pool_) t.used = t.pinned;
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

void Gpu::bindFramebuffer(GLenum target, GLuint fbo) {
    profile.framebuffer(target, fbo);
    glBindFramebuffer(target, fbo);
}

void Gpu::scissor(const QRect& bounds) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(bounds.x(),bounds.y(),bounds.width(),bounds.height());
}

void Gpu::prepareBounded(int texture,const QRect& bounds) {
    auto& t=pool_[texture];
    bindFramebuffer(GL_FRAMEBUFFER,t.fbo);
    // Old support can belong to a completely different canvas or blur chain.
    scissor(t.dirty);
    glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
    t.bounds=bounds; t.dirty=bounds;
    scissor(bounds);
}

void Gpu::bindMain() {
    bindFramebuffer(GL_FRAMEBUFFER, current_->fbo);
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

void Gpu::clearRasterCache() {
    for(const auto& entry:rasters_) pool_[entry.second.texture].pinned=false;
    for(const auto& entry:filters_) pool_[entry.second].pinned=false;
    rasters_.clear(); filters_.clear();
}

bool Gpu::rasterKey(const Canvas& canvas,RasterKey& key) const {
    if(!cacheGeometry_) return false;
    // Cache only an entire isolated layer. Collapsing translucent draws inside
    // the main target would change per-sample blending and half-float rounding.
    if(canvas.frozen()) key.emplace_back(canvas.identity(),canvas.revision());
    for(const auto& cmd:canvas.commands()) {
        if(cmd.kind==Canvas::CmdKind::Cached) {
            if(!rasterKey(canvas.retained(cmd.first),key)) return false;
        } else if(!canvas.frozen()) return false;
    }
    return !key.empty();
}

void Gpu::clearGeometryCache() {
    clearRasterCache();
    for (auto& entry : geometry_) {
        auto& g = entry.second;
        glDeleteBuffers(1, &g.vbo);
        glDeleteVertexArrays(1, &g.vao);
        glDeleteTextures(1, &g.paints);
    }
    geometry_.clear();
}

void Gpu::bindGeometry(const Canvas& c) {
    Geometry* retained = nullptr;
    GLuint vao = vao_, vbo = vbo_, paints = paints_;
    if (cacheGeometry_) {
        retained = &geometry_[c.identity()];
        if (!retained->vao) {
            glGenVertexArrays(1, &retained->vao);
            glGenBuffers(1, &retained->vbo);
            glBindVertexArray(retained->vao);
            glBindBuffer(GL_ARRAY_BUFFER, retained->vbo);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(8));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(24));

        }
        if(c.frozen() || !c.preserveRaster) {vao = retained->vao; vbo = retained->vbo;}
        if(c.gradients().empty())paints=zeroPaints_;
        else {
            if(!retained->paints) {
                glGenTextures(1,&retained->paints);glBindTexture(GL_TEXTURE_2D,retained->paints);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            }
            paints=retained->paints;
        }
    }
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, paints);
    const bool retainedVertices=retained && (c.frozen() || !c.preserveRaster);
    if (retainedVertices ? retained->revision != c.revision() : dynamicVertexId_!=c.identity() || dynamicVertexRevision_!=c.revision()) {
        const auto& v = c.vertices();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(v.size() * sizeof(Vertex)), v.data(),
                     c.frozen() ? GL_STATIC_DRAW : GL_STREAM_DRAW);
        ++geometryStats_.uploads;
        geometryStats_.staticUploads += c.frozen();
        geometryStats_.vertexBytes += v.size() * sizeof(Vertex);
        if (retainedVertices) retained->revision = c.revision();
        else {dynamicVertexId_=c.identity();dynamicVertexRevision_=c.revision();}
    }
    if(paints!=zeroPaints_ && (retained ? retained->paintRevision!=c.revision() : dynamicPaintId_!=c.identity() || dynamicPaintRevision_!=c.revision())) {
        const auto& g = c.gradients();
        geometryStats_.paintBytes += std::max<std::size_t>(1, g.size()) * sizeof(GradientRow);
        static const GradientRow zero;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 16, GLsizei(std::max<std::size_t>(1, g.size())),
                     0, GL_RGBA, GL_FLOAT, g.empty() ? &zero : g.data());
        if(retained) retained->paintRevision=c.revision();
        else {dynamicPaintId_=c.identity();dynamicPaintRevision_=c.revision();}
    }


}

void Gpu::drawCanvas(const Canvas& c) {
    if (c.empty()) return;
    glUseProgram(canvas_.id);
    canvas_.set("u_paints", 0);
    bool bound = false;
    bool stencil=false;
    glDisable(GL_STENCIL_TEST);
    auto setStencil=[&](bool enabled) {
        if(stencil==enabled) return;
        if(enabled) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
        stencil=enabled;
    };
    if(cacheGeometry_ && c.batchSpatially) {
        // Sharing a screen cell creates a dependency. A later shape can only
        // move before an earlier one when their padded coverage is disjoint.
        // Keep each original triangle and its original vertex index.
        bindGeometry(c);
        constexpr int cell=8,cols=256,rows=160;
        std::vector<unsigned> occupied(cols*rows,0);
        std::vector<std::vector<const Canvas::Cmd*>> levels(1);
        for(const auto& cmd:c.commands()) {
            std::array<float,4> box{1e30f,1e30f,-1e30f,-1e30f};
            const int first=cmd.kind==Canvas::CmdKind::StencilFill?cmd.coverFirst:cmd.first;
            const int count=cmd.kind==Canvas::CmdKind::StencilFill?cmd.coverCount:cmd.count;
            for(int k=first;k<first+count;++k) {const auto& v=c.vertices()[k];box[0]=std::min(box[0],v.x);box[1]=std::min(box[1],v.y);box[2]=std::max(box[2],v.x);box[3]=std::max(box[3],v.y);}
            const float pad=1/c.pixelScale();
            int x0=std::clamp(int(std::floor((box[0]-pad)/cell)),0,cols-1),x1=std::clamp(int(std::floor((box[2]+pad)/cell)),0,cols-1);
            int y0=std::clamp(int(std::floor((box[1]-pad)/cell))+16,0,rows-1),y1=std::clamp(int(std::floor((box[3]+pad)/cell))+16,0,rows-1);
            unsigned level=0;
            for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)level=std::max(level,occupied[y*cols+x]);
            for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)occupied[y*cols+x]=level+1;
            if(levels.size()<=level)levels.resize(level+1);
            levels[level].push_back(&cmd);
        }
        std::vector<GLint> firsts,covers;
        std::vector<GLsizei> counts,coverCounts;
        for(const auto& level:levels)for(auto kind:{Canvas::CmdKind::Direct,Canvas::CmdKind::StencilFill,Canvas::CmdKind::StencilOnce}) {
            firsts.clear();counts.clear();covers.clear();coverCounts.clear();
            for(const auto* cmd:level)if(cmd->kind==kind) {firsts.push_back(cmd->first);counts.push_back(cmd->count);covers.push_back(cmd->coverFirst);coverCounts.push_back(cmd->coverCount);}
            if(firsts.empty())continue;
            if(kind==Canvas::CmdKind::Direct) {setStencil(false);glMultiDrawArrays(GL_TRIANGLES,firsts.data(),counts.data(),firsts.size());}
            else if(kind==Canvas::CmdKind::StencilFill) {
                setStencil(true);glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glStencilFunc(GL_ALWAYS,0,0xff);
                glStencilOpSeparate(GL_FRONT,GL_KEEP,GL_KEEP,GL_INCR_WRAP);glStencilOpSeparate(GL_BACK,GL_KEEP,GL_KEEP,GL_DECR_WRAP);
                glMultiDrawArrays(GL_TRIANGLES,firsts.data(),counts.data(),firsts.size());
                glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glStencilFunc(GL_NOTEQUAL,0,0xff);glStencilOp(GL_ZERO,GL_ZERO,GL_ZERO);
                glMultiDrawArrays(GL_TRIANGLES,covers.data(),coverCounts.data(),covers.size());
            } else {
                setStencil(true);glStencilFunc(GL_EQUAL,0,0xff);glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
                glMultiDrawArrays(GL_TRIANGLES,firsts.data(),counts.data(),firsts.size());
                glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glStencilFunc(GL_ALWAYS,0,0xff);glStencilOp(GL_ZERO,GL_ZERO,GL_ZERO);
                glMultiDrawArrays(GL_TRIANGLES,firsts.data(),counts.data(),firsts.size());glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
            }
        }
        setStencil(false);glBindVertexArray(0);return;
    }
    const auto& commands=c.commands();
    std::vector<GLint> starts,covers;
    std::vector<GLsizei> counts,coverCounts;
    std::vector<std::array<float,4>> boxes;
    for (std::size_t ci=0;ci<commands.size();++ci) {
        const auto& cmd=commands[ci];
        if (cmd.kind == Canvas::CmdKind::Cached) {
            setStencil(false);
            drawCanvas(c.retained(cmd.first));
            glUseProgram(canvas_.id);
            bound = false;
            continue;
        }
        if (!bound) { bindGeometry(c); bound = true; }
        // Independent stencil operations can share a submission while keeping
        // their original fans, cover triangles and paint order. Padded bounds
        // must be disjoint, including multisample edge coverage.
        if(cacheGeometry_ && !c.preserveRaster && (cmd.kind==Canvas::CmdKind::StencilFill || cmd.kind==Canvas::CmdKind::StencilOnce)) {
            starts.clear();counts.clear();covers.clear();coverCounts.clear();boxes.clear();
            for(std::size_t j=ci;j<commands.size() && commands[j].kind==cmd.kind;++j) {
                const auto& part=commands[j];
                std::array<float,4> box{1e30f,1e30f,-1e30f,-1e30f};
                const int first=cmd.kind==Canvas::CmdKind::StencilFill?part.coverFirst:part.first;
                const int count=cmd.kind==Canvas::CmdKind::StencilFill?part.coverCount:part.count;
                for(int k=first;k<first+count;++k) {const auto& v=c.vertices()[k];box[0]=std::min(box[0],v.x);box[1]=std::min(box[1],v.y);box[2]=std::max(box[2],v.x);box[3]=std::max(box[3],v.y);}
                const float pad=1.0f/c.pixelScale();
                bool overlap=false;
                for(const auto& b:boxes) if(box[0]<=b[2]+pad && box[2]+pad>=b[0] && box[1]<=b[3]+pad && box[3]+pad>=b[1]){overlap=true;break;}
                if(overlap) break;
                boxes.push_back(box);starts.push_back(part.first);counts.push_back(part.count);
                covers.push_back(part.coverFirst);coverCounts.push_back(part.coverCount);
            }
            if(starts.size()>1) {
                setStencil(true);
                if(cmd.kind==Canvas::CmdKind::StencilFill) {
                    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glStencilFunc(GL_ALWAYS,0,0xff);
                    glStencilOpSeparate(GL_FRONT,GL_KEEP,GL_KEEP,GL_INCR_WRAP);glStencilOpSeparate(GL_BACK,GL_KEEP,GL_KEEP,GL_DECR_WRAP);
                    glMultiDrawArrays(GL_TRIANGLES,starts.data(),counts.data(),GLsizei(starts.size()));
                    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glStencilFunc(GL_NOTEQUAL,0,0xff);glStencilOp(GL_ZERO,GL_ZERO,GL_ZERO);
                    for(std::size_t k=0;k<covers.size();++k)glDrawArrays(GL_TRIANGLES,covers[k],coverCounts[k]);
                } else {
                    glStencilFunc(GL_EQUAL,0,0xff);glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
                    for(std::size_t k=0;k<starts.size();++k)glDrawArrays(GL_TRIANGLES,starts[k],counts[k]);
                    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glStencilFunc(GL_ALWAYS,0,0xff);glStencilOp(GL_ZERO,GL_ZERO,GL_ZERO);
                    glMultiDrawArrays(GL_TRIANGLES,starts.data(),counts.data(),GLsizei(starts.size()));
                    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
                }
                ci+=starts.size()-1;continue;
            }
        }
        switch (cmd.kind) {
        case Canvas::CmdKind::Cached: break; // handled above
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
    GpuProfile::Scope timing(profile,"draw",w_,h_,"RGBA16F-MSAA");
    bindMain();
    setBlend(blend, gain);
    drawCanvas(canvas);
}

int Gpu::layer(const Canvas& canvas,float sigmaDesign) {
    int reduction=1,w=w_,h=h_;
    while(sigmaDesign*pixelScale()/reduction>3 && reduction<32) {
        reduction*=2; w=(w+1)/2; h=(h+1)/2;
    }
    Target* target=&layer_;
    if(reduction>1) {
        target=&reducedLayers_[{w,h}];
        if(!target->fbo) {
            glGenFramebuffers(1,&target->fbo);
            glGenRenderbuffers(1,&target->color); glGenRenderbuffers(1,&target->depth);
            glBindRenderbuffer(GL_RENDERBUFFER,target->color);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples_,GL_RGBA16F,w,h);
            glBindRenderbuffer(GL_RENDERBUFFER,target->depth);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples_,GL_DEPTH24_STENCIL8,w,h);
            bindFramebuffer(GL_FRAMEBUFFER,target->fbo);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,target->color);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,target->depth);
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) std::abort();
        }
    }
    RasterKey key;
    const bool retained=rasterKey(canvas,key);
    if(retained) key.emplace_back(0,reduction);
    if(retained) {
        const auto found=rasters_.find(key);
        if(found!=rasters_.end()) {
            found->second.frame=rasterFrame_;
            return found->second.texture;
        }
    }
    const int raster=profile.start("layer-raster",w,h,"RGBA16F-MSAA");
    bindFramebuffer(GL_FRAMEBUFFER, target->fbo);
    glViewport(0, 0, w, h);
    const auto extent=canvas.bounds();
    const QRectF box=std::isfinite(extent[0]) ? QRectF(extent[0],extent[1],extent[2]-extent[0],extent[3]-extent[1]) : QRectF();
    const int x0=std::clamp(int(std::floor(box.left()*w/1920.0))-2,0,w);
    const int x1=std::clamp(int(std::ceil(box.right()*w/1920.0))+2,0,w);
    const int y0=std::clamp(int(std::floor((1080-box.bottom())*h/1080.0))-2,0,h);
    const int y1=std::clamp(int(std::ceil((1080-box.top())*h/1080.0))+2,0,h);
    const QRect bounds(x0,y0,x1-x0,y1-y0);
    scissor(target->dirty.united(bounds));
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    target->dirty=bounds;
    scissor(bounds);
    setBlend(Blend::Over, 1);
    drawCanvas(canvas);
    profile.stop(raster);
    GpuProfile::Scope timing(profile,"layer-resolve",w,h,"RGBA16F");
    const int t = acquire(w, h);
    prepareBounded(t,bounds);
    bindFramebuffer(GL_READ_FRAMEBUFFER, target->fbo);
    bindFramebuffer(GL_DRAW_FRAMEBUFFER, pool_[t].fbo);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glDisable(GL_SCISSOR_TEST);
    if(retained) { pool_[t].pinned=true; rasters_[key]={t,rasterFrame_}; }
    pool_[t].reduction=reduction;
    return t;
}

void Gpu::fullscreen() {
    profile.fullscreen();
    glBindVertexArray(quadVao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

int Gpu::downsample(int src) {
    const int w = std::max(1, (pool_[src].w + 1) / 2), h = std::max(1, (pool_[src].h + 1) / 2);
    GpuProfile::Scope timing(profile,"downsample",w,h,"RGBA16F");
    const int dst = acquire(w, h);
    bindFramebuffer(GL_FRAMEBUFFER, pool_[dst].fbo);
    const QRect source=pool_[src].bounds;
    const int x0=std::max(0,int(std::floor(double(source.left())*w/pool_[src].w))-2);
    const int y0=std::max(0,int(std::floor(double(source.top())*h/pool_[src].h))-2);
    const int x1=std::min(w,int(std::ceil(double(source.x()+source.width())*w/pool_[src].w))+2);
    const int y1=std::min(h,int(std::ceil(double(source.y()+source.height())*h/pool_[src].h))+2);
    prepareBounded(dst,QRect(x0,y0,x1-x0,y1-y0));
    glViewport(0, 0, w, h);
    glDisable(GL_BLEND);
    glUseProgram(downP_.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pool_[src].tex);
    downP_.set("u_tex", 0);
    fullscreen();
    glDisable(GL_SCISSOR_TEST);
    return dst;
}

int Gpu::blurPass(int src, float sigma, bool horizontal) {
    const int w = pool_[src].w, h = pool_[src].h;
    GpuProfile::Scope timing(profile,horizontal?"blur-H":"blur-V",w,h,"RGBA16F",sigma,1+2*std::min(int(std::ceil(sigma*3)),24));
    const int dst = acquire(w, h);
    bindFramebuffer(GL_FRAMEBUFFER, pool_[dst].fbo);
    const int pad=std::min(int(std::ceil(sigma*3)),24)+2;
    const QRect support=pool_[src].bounds.adjusted(horizontal?-pad:0,horizontal?0:-pad,horizontal?pad:0,horizontal?0:pad).intersected(QRect(0,0,w,h));
    prepareBounded(dst,support);
    glViewport(0, 0, w, h);
    glDisable(GL_BLEND);
    glUseProgram(blurP_.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pool_[src].tex);
    blurP_.set("u_tex", 0);
    blurP_.set("u_sigma", sigma);
    blurP_.set("u_dir", horizontal ? 1.f / w : 0.f, horizontal ? 0.f : 1.f / h);
    fullscreen();
    glDisable(GL_SCISSOR_TEST);
    return dst;
}

int Gpu::blurred(int tex, float sigmaDesign) {
    const float s = float(sigmaDesign * pixelScale());
    if (s < 0.3f) return tex;
    const bool retained=pool_[tex].pinned;
    if(retained) {
        const auto found=filters_.find({tex,sigmaDesign});
        if(found!=filters_.end()) return found->second;
    }
    int cur = tex;
    float f = float(pool_[tex].reduction);
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
    if(retained) { pool_[b].pinned=true; filters_[{tex,sigmaDesign}]=b; }
    return b;
}

void Gpu::composite(int tex, Blend blend, float gain, float opacity) {
    GpuProfile::Scope timing(profile,"composite",w_,h_,"RGBA16F-MSAA");
    bindMain();
    glUseProgram(compositeP_.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pool_[tex].tex);
    compositeP_.set("u_tex", 0);
    compositeP_.set("u_gain", gain);
    compositeP_.set("u_opacity", opacity);
    setBlend(blend, 1);
    const auto& t=pool_[tex];
    const int x0=std::max(0,int(std::floor(double(t.bounds.x())*w_/t.w))-2);
    const int y0=std::max(0,int(std::floor(double(t.bounds.y())*h_/t.h))-2);
    const int x1=std::min(w_,int(std::ceil(double(t.bounds.x()+t.bounds.width())*w_/t.w))+2);
    const int y1=std::min(h_,int(std::ceil(double(t.bounds.y()+t.bounds.height())*h_/t.h))+2);
    scissor(QRect(x0,y0,x1-x0,y1-y0));
    fullscreen();
    glDisable(GL_SCISSOR_TEST);
}

void Gpu::over(const Canvas& canvas, float gain, float blur, float opacity) {
    if (canvas.empty() || opacity <= 0.001f) return;
    if (blur <= 0 && opacity >= 0.999f) { draw(canvas, Blend::Over, gain); return; }
    int t = layer(canvas,blur);
    const int b = blurred(t, blur);
    composite(b, Blend::Over, gain, opacity);
    pool_[t].used = false;
    pool_[b].used = false;
}

void Gpu::add(const Canvas& canvas, float gain, float blur) {
    if (canvas.empty() || gain <= 0) return;
    int t = layer(canvas,blur);
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

void Gpu::pass(Program& p, Blend blend, const std::function<void(Program&)>& setup, int target, const QRectF& clip) {
    int w = w_, h = h_;
    if (target >= 0) {
        bindFramebuffer(GL_FRAMEBUFFER, pool_[target].fbo);
        w = pool_[target].w; h = pool_[target].h;
        glViewport(0, 0, w, h);
    } else bindMain();
    std::string name="effect";
    if(profile.enabled()) for(const auto& entry:effects_) if(entry.second.id==p.id) { name=entry.first;break; }
    GpuProfile::Scope timing(profile,name,w,h,target>=0?"RGBA16F":"RGBA16F-MSAA");
    glUseProgram(p.id);
    p.set("u_size", float(w), float(h));
    p.set("u_scale", float(pixelScale()));
    if(target>=0) pool_[target].dirty=pool_[target].bounds=QRect(0,0,w,h);
    if (setup) setup(p);
    setBlend(blend, 1);
    if(!clip.isEmpty()) {
        const int x0=std::clamp(int(std::floor(clip.left()*w/1920.0))-1,0,w);
        const int x1=std::clamp(int(std::ceil(clip.right()*w/1920.0))+1,0,w);
        const int y0=std::clamp(int(std::floor((1080-clip.bottom())*h/1080.0))-1,0,h);
        const int y1=std::clamp(int(std::ceil((1080-clip.top())*h/1080.0))+1,0,h);
        glEnable(GL_SCISSOR_TEST);glScissor(x0,y0,x1-x0,y1-y0);
    }
    fullscreen();
    if(!clip.isEmpty())glDisable(GL_SCISSOR_TEST);
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
    GpuProfile::Scope timing(profile,"snapshot-resolve",w_,h_,"RGBA16F");
    const int t = acquire(w_, h_);
    bindFramebuffer(GL_READ_FRAMEBUFFER, current_->fbo);
    bindFramebuffer(GL_DRAW_FRAMEBUFFER, pool_[t].fbo);
    glBlitFramebuffer(0, 0, w_, h_, 0, 0, w_, h_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    pool_[t].dirty=pool_[t].bounds;
    return t;
}

void Gpu::bindTexture(int unit, int tex, Program& p, const char* name) {
    glActiveTexture(GLenum(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, pool_[tex].tex);
    p.set(name, unit);
}

void Gpu::finish(const FinishParams& f, const Canvas* overlay) {
    GpuProfile::Group group(profile,"finish");
    const int img = snapshot();
    int b0 = -1, b1 = -1, b2 = -1;
    if (f.bloom > 0) {
        const int extract=profile.start("bright-extract",w_,h_,"RGBA16F");
        const int bright = acquire(w_, h_);
        bindFramebuffer(GL_FRAMEBUFFER, pool_[bright].fbo);
        glViewport(0, 0, w_, h_);
        glDisable(GL_BLEND);
        glUseProgram(brightP_.id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pool_[img].tex);
        brightP_.set("u_tex", 0);
        brightP_.set("u_threshold", f.threshold);
        fullscreen();
        pool_[bright].dirty=pool_[bright].bounds;
        profile.stop(extract);
        b0 = blurred(bright, 5);
        b1 = blurred(bright, 22);
        b2 = blurred(bright, 70);
    }
    GpuProfile::Scope grade(profile,"grade-overlay",w_,h_,"RGBA8");
    bindFramebuffer(GL_FRAMEBUFFER, outFbo_);
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
    if(scaledFbo_) {
        bindFramebuffer(GL_READ_FRAMEBUFFER,outFbo_);
        bindFramebuffer(GL_DRAW_FRAMEBUFFER,scaledFbo_);
        glBlitFramebuffer(0,0,w_,h_,0,0,allocatedOutputW_,allocatedOutputH_,GL_COLOR_BUFFER_BIT,GL_LINEAR);
    }

}



void Gpu::present(GLuint framebuffer) {
    bindFramebuffer(GL_READ_FRAMEBUFFER, finishedFbo());
    bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glBlitFramebuffer(0, 0, allocatedOutputW_, allocatedOutputH_, 0, 0, allocatedOutputW_, allocatedOutputH_, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    bindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void Gpu::readRgb(std::vector<unsigned char>& rgb) {
    std::vector<unsigned char> rgba(std::size_t(allocatedOutputW_) * allocatedOutputH_ * 4);
    bindFramebuffer(GL_READ_FRAMEBUFFER, finishedFbo());
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, allocatedOutputW_, allocatedOutputH_, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    rgb.resize(std::size_t(allocatedOutputW_) * allocatedOutputH_ * 3);
    for (int y = 0; y < allocatedOutputH_; ++y) {
        const unsigned char* s = rgba.data() + std::size_t(allocatedOutputH_ - 1 - y) * allocatedOutputW_ * 4;
        unsigned char* d = rgb.data() + std::size_t(y) * allocatedOutputW_ * 3;
        for (int x = 0; x < allocatedOutputW_; ++x) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d += 3; s += 4; }
    }
}
}
