#include "native_renderer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string_view>

namespace {
std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

std::string readShaderFile(const std::filesystem::path& path, std::string& error,
                           int includeDepth = 0) {
    if (includeDepth > 8) {
        error = "native shader include depth exceeded at " + path.string();
        return {};
    }
    const std::string source = readFile(path);
    if (source.empty()) {
        error = "could not read native renderer shader " + path.string();
        return {};
    }

    std::istringstream input(source);
    std::ostringstream expanded;
    std::string line;
    while (std::getline(input, line)) {
        constexpr std::string_view includePrefix = "#include \"";
        if (line.rfind(includePrefix, 0) == 0) {
            const std::size_t closingQuote = line.find('"', includePrefix.size());
            if (closingQuote == std::string::npos) {
                error = "invalid native shader include in " + path.string();
                return {};
            }
            const std::filesystem::path includePath = path.parent_path()
                / line.substr(includePrefix.size(), closingQuote - includePrefix.size());
            const std::string included = readShaderFile(
                includePath, error, includeDepth + 1);
            if (included.empty()) return {};
            expanded << included << '\n';
        } else {
            expanded << line << '\n';
        }
    }
    return expanded.str();
}

GLuint compile(GLenum type, const std::string& source, std::string& error) {
    const GLuint shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok) return shader;
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(std::max(1, length), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    error = log;
    glDeleteShader(shader);
    return 0;
}

GLuint linkProgram(const std::string& vertexSource, const std::string& fragmentSource,
                   std::string& error) {
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource, error);
    if (!vertex) return 0;
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource, error);
    if (!fragment) {
        glDeleteShader(vertex);
        return 0;
    }
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked) return program;
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(std::max(1, length), '\0');
    glGetProgramInfoLog(program, length, nullptr, log.data());
    error = log;
    glDeleteProgram(program);
    return 0;
}
}

void NativeRenderer::initializeTargets() {
    glGenVertexArrays(1, &vao_);
    glGenFramebuffers(1, &framebuffer_);
    for (auto& sceneTextures : textures_) {
        glGenTextures(static_cast<GLsizei>(sceneTextures.size()), sceneTextures.data());
    }
}

NativeRenderer::~NativeRenderer() {
    shutdown();
}

bool NativeRenderer::initialize(const std::filesystem::path& shaderDirectory,
                                std::string& error) {
    const std::string vertexSource = readShaderFile(
        shaderDirectory / "fullscreen.vert", error);
    if (vertexSource.empty()) {
        return false;
    }
    for (std::size_t scene = 0; scene < nativeSceneCount; ++scene) {
        const NativeSceneDefinition& definition = nativeSceneRegistry[scene];
        const std::string fragmentSource = readShaderFile(
            shaderDirectory / definition.shader, error);
        if (fragmentSource.empty()) {
            shutdown();
            return false;
        }
        programs_[scene] = linkProgram(vertexSource, fragmentSource, error);
        if (!programs_[scene]) {
            shutdown();
            return false;
        }
    }
    initializeTargets();
    gpuTimer_.initialize();
    return true;
}

bool NativeRenderer::initializeCustomShader(
        const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath, std::string& error) {
    const std::string vertexSource = readShaderFile(vertexPath, error);
    if (vertexSource.empty()) return false;
    const std::string fragmentSource = readShaderFile(fragmentPath, error);
    if (fragmentSource.empty()) return false;
    programs_[static_cast<std::size_t>(NativeSceneKind::DepthTunnel)]
        = linkProgram(vertexSource, fragmentSource, error);
    if (!programs_[0]) {
        shutdown();
        return false;
    }
    initializeTargets();
    gpuTimer_.initialize();
    return true;
}

bool NativeRenderer::resize(int width, int height, std::string& error) {
    if (width == width_ && height == height_) return true;
    width_ = width;
    height_ = height;
    activeTextures_.fill(0);
    for (const auto& sceneTextures : textures_) {
        for (const GLuint texture : sceneTextures) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width_, height_, 0,
                         GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, texture, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                error = "native renderer framebuffer is incomplete";
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                return false;
            }
            glViewport(0, 0, width_, height_);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool NativeRenderer::render(const MusicFrame& music, const NativeSceneState& scene,
                            int width, int height,
                            const std::array<float, 3>& albumColor,
                            GLuint artworkTexture, float artworkAspect,
                            float frameSeconds, std::string& error,
                            const NativeRenderPolicy& policy) {
    if (!programs_[0]) {
        error = "native renderer is not initialized";
        return false;
    }
    if (!resize(width, height, error)) return false;
    const auto gestureResponse = [](float value, float gain) {
        return 1.25f * (1.0f - std::exp(-gain * std::max(0.0f, value)));
    };
    const float intensity = policy.effectiveIntensity();
    const float motion = policy.effectiveMotion();
    const float eventScale = policy.flashLimited ? 0.82f : 1.0f;
    const float quality = policy.effectiveQuality();
    const float responsiveKick
        = gestureResponse(music.kick * intensity, 2.35f) * eventScale;
    const float responsiveSnare
        = gestureResponse(music.snare * intensity, 2.20f) * eventScale;
    const float responsiveHat
        = gestureResponse(music.hat * intensity, 2.55f) * eventScale;
    // Sustained energy provides continuity at a restrained speed. Event
    // envelopes deform and relight geometry directly instead of accelerating
    // this accumulated clock, which would make the largest response arrive
    // several frames after the sound.
    flowTime_ += motion * std::clamp(frameSeconds, 0.0f, 0.1f)
               * (0.006f + 0.18f * std::sqrt(std::max(0.0f, music.energySlow))
                  + 0.06f * scene.drive);

    std::array<bool, nativeSceneCount> renderScene{};
    renderScene[static_cast<std::size_t>(scene.currentScene)] = true;
    if (scene.transitioning) {
        renderScene[static_cast<std::size_t>(scene.incomingScene)] = true;
    }
    gpuTimer_.begin();
    for (std::size_t sceneIndex = 0; sceneIndex < nativeSceneCount; ++sceneIndex) {
        if (!renderScene[sceneIndex]) continue;
        const GLuint program = programs_[sceneIndex];
        const int nextTexture = 1 - activeTextures_[sceneIndex];
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, textures_[sceneIndex][nextTexture], 0);
        glViewport(0, 0, width_, height_);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures_[sceneIndex][activeTextures_[sceneIndex]]);
        glUniform1i(glGetUniformLocation(program, "previousFrame"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, artworkTexture);
        glUniform1i(glGetUniformLocation(program, "artworkFrame"), 1);
        glUniform1f(glGetUniformLocation(program, "artworkAvailable"),
                    artworkTexture ? 1.0f : 0.0f);
        glUniform1f(glGetUniformLocation(program, "artworkAspect"), artworkAspect);
        glUniform2f(glGetUniformLocation(program, "resolution"),
                static_cast<float>(width_), static_cast<float>(height_));
        glUniform1f(glGetUniformLocation(program, "flowTime"), flowTime_);
        glUniform1f(glGetUniformLocation(program, "beatPhase"), music.beatPhase);
        glUniform1f(glGetUniformLocation(program, "beatAnticipation"),
                music.beatAnticipation);
        glUniform1f(glGetUniformLocation(program, "beatPulse"),
                    std::clamp(music.beatPulse * intensity * eventScale,
                               0.0f, 1.25f));
        glUniform1f(glGetUniformLocation(program, "onsetPulse"),
                    std::clamp(music.onsetPulse * intensity * eventScale,
                               0.0f, 1.25f));
        glUniform1f(glGetUniformLocation(program, "downbeat"),
                    std::clamp(music.downbeat * intensity * eventScale,
                               0.0f, 1.25f));
        glUniform1f(glGetUniformLocation(program, "barPhase"), music.barPhase);
        glUniform1f(glGetUniformLocation(program, "phrasePhase"), music.phrasePhase);
        glUniform1f(glGetUniformLocation(program, "clockConfidence"),
                music.clockConfidence);
        glUniform1f(glGetUniformLocation(program, "kick"), responsiveKick);
        glUniform1f(glGetUniformLocation(program, "snare"), responsiveSnare);
        glUniform1f(glGetUniformLocation(program, "hat"), responsiveHat);
        glUniform1f(glGetUniformLocation(program, "percussive"), music.percussive);
        glUniform1f(glGetUniformLocation(program, "harmonic"), music.harmonic);
        glUniform1f(glGetUniformLocation(program, "spectralCentroid"),
                music.spectralCentroid);
        glUniform1f(glGetUniformLocation(program, "stereoWidth"), music.stereoWidth);
        glUniform1f(glGetUniformLocation(program, "rhythmicDensity"),
                music.rhythmicDensity);
        glUniform1f(glGetUniformLocation(program, "syncopation"), music.syncopation);
        glUniform1f(glGetUniformLocation(program, "tonalMotion"), music.tonalMotion);
        glUniform1f(glGetUniformLocation(program, "harmonicChange"),
                music.harmonicChange);
        glUniform1f(glGetUniformLocation(program, "energyFast"), music.energyFast);
        glUniform1f(glGetUniformLocation(program, "energySlow"), music.energySlow);
        glUniform1f(glGetUniformLocation(program, "energySlope"), music.energySlope);
        glUniform1f(glGetUniformLocation(program, "section"),
                    std::clamp(music.section * intensity * eventScale,
                               0.0f, 1.25f));
        glUniform1f(glGetUniformLocation(program, "development"), scene.development);
        glUniform1f(glGetUniformLocation(program, "drive"),
                    scene.drive * (0.35f + 0.65f * motion));
        glUniform1f(glGetUniformLocation(program, "peak"),
                    scene.peak * (0.50f + 0.50f * motion));
        glUniform1f(glGetUniformLocation(program, "release"), scene.release);
        glUniform1f(glGetUniformLocation(program, "sceneBeats"), scene.sceneBeats);
        glUniform1f(glGetUniformLocation(program, "motionScale"), motion);
        glUniform1f(glGetUniformLocation(program, "qualityScale"), quality);
        glUniform3f(glGetUniformLocation(program, "albumColor"),
                albumColor[0], albumColor[1], albumColor[2]);
        glUniform1fv(glGetUniformLocation(program, "bandLevel[0]"),
                 static_cast<GLsizei>(music.bandLevel.size()), music.bandLevel.data());
        glUniform1fv(glGetUniformLocation(program, "spectrumLevel[0]"),
                 static_cast<GLsizei>(music.spectrumLevel.size()),
                 music.spectrumLevel.data());
        glUniform1fv(glGetUniformLocation(program, "chroma[0]"),
                 static_cast<GLsizei>(music.chroma.size()), music.chroma.data());
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        activeTextures_[sceneIndex] = nextTexture;
    }
    gpuTimer_.end();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

GLuint NativeRenderer::texture(NativeSceneKind scene) const {
    const std::size_t index = static_cast<std::size_t>(scene);
    return textures_[index][activeTextures_[index]];
}

void NativeRenderer::reset() {
    if (!framebuffer_ || width_ <= 0 || height_ <= 0) return;
    for (const auto& sceneTextures : textures_) {
        for (const GLuint texture : sceneTextures) {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, texture, 0);
            glViewport(0, 0, width_, height_);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    activeTextures_.fill(0);
    flowTime_ = 0.0f;
}

void NativeRenderer::shutdown() {
    gpuTimer_.shutdown();
    if (framebuffer_) glDeleteFramebuffers(1, &framebuffer_);
    for (const auto& sceneTextures : textures_) {
        if (sceneTextures[0]) {
            glDeleteTextures(static_cast<GLsizei>(sceneTextures.size()),
                             sceneTextures.data());
        }
    }
    if (vao_) glDeleteVertexArrays(1, &vao_);
    for (const GLuint program : programs_) {
        if (program) glDeleteProgram(program);
    }
    framebuffer_ = 0;
    textures_ = {};
    programs_ = {};
    vao_ = 0;
    width_ = 0;
    height_ = 0;
    flowTime_ = 0.0f;
}
