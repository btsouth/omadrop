#include "live_compositor.h"

#include <algorithm>
#include <array>

namespace {
GLuint compileShader(GLenum type, const char* source, std::string& error) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled) return shader;

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(std::max(1, length), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    error = log;
    glDeleteShader(shader);
    return 0;
}

bool setSampler(GLuint program, const char* name, int unit, GLuint texture) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(program, name), unit);
    return glGetError() == GL_NO_ERROR;
}
}

LiveCompositor::~LiveCompositor() {
    shutdown();
}

bool LiveCompositor::initialize(const char* vertexSource,
                                const char* fragmentSource,
                                std::string& error) {
    shutdown();
    vertexShader_ = compileShader(GL_VERTEX_SHADER, vertexSource, error);
    if (!vertexShader_) return false;
    fragmentShader_ = compileShader(GL_FRAGMENT_SHADER, fragmentSource, error);
    if (!fragmentShader_) {
        shutdown();
        return false;
    }
    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader_);
    glAttachShader(program_, fragmentShader_);
    glLinkProgram(program_);
    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint length = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
        std::string log(std::max(1, length), '\0');
        glGetProgramInfoLog(program_, length, nullptr, log.data());
        error = log;
        shutdown();
        return false;
    }
    glGenVertexArrays(1, &vao_);
    gpuTimer_.initialize();
    return glGetError() == GL_NO_ERROR;
}

bool LiveCompositor::render(const LiveCompositorFrame& frame,
                            std::string& error) {
    if (!program_ || !vao_) {
        error = "live compositor is not initialized";
        return false;
    }
    if (frame.width <= 0 || frame.height <= 0) {
        error = "live compositor received an invalid output size";
        return false;
    }

    // Backends may leave a diagnostic error behind after their own render
    // pass. Only attribute errors produced after this boundary to the
    // compositor.
    while (glGetError() != GL_NO_ERROR) {}
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, frame.width, frame.height);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program_);
    if (!setSampler(program_, "sourceFrame", 0, frame.sourceTexture)
        || !setSampler(program_, "coverFrame", 1, frame.coverTexture)
        || !setSampler(program_, "nextFrame", 2, frame.nextTexture)) {
        error = "live compositor could not bind its input textures";
        return false;
    }
    glUniform1f(glGetUniformLocation(program_, "presetMix"), frame.sceneMix);
    glUniform1i(glGetUniformLocation(program_, "transitionMode"),
                frame.transitionMode);
    glUniform1i(glGetUniformLocation(program_, "sourceReactionMode"),
                frame.sourceReactionMode);
    glUniform1i(glGetUniformLocation(program_, "nextReactionMode"),
                frame.nextReactionMode);
    glUniform3fv(glGetUniformLocation(program_, "sourceReactionGain"), 1,
                 frame.sourceReactionGain.data());
    glUniform3fv(glGetUniformLocation(program_, "nextReactionGain"), 1,
                 frame.nextReactionGain.data());
    glUniform1f(glGetUniformLocation(program_, "asciiExposure"),
                frame.asciiExposure);
    glUniform1f(glGetUniformLocation(program_, "fieldExposure"),
                frame.fieldExposure);
    glUniform1i(glGetUniformLocation(program_, "nativeRenderer"),
                frame.nativeRenderer ? 1 : 0);
    glUniform2f(glGetUniformLocation(program_, "resolution"),
                static_cast<float>(frame.width),
                static_cast<float>(frame.height));
    glUniform1f(glGetUniformLocation(program_, "coverAspect"), frame.coverAspect);
    glUniform1f(glGetUniformLocation(program_, "coverMix"), frame.coverMix);
    glUniform3fv(glGetUniformLocation(program_, "albumColor"), 1,
                 frame.albumColor.data());
    glUniform1f(glGetUniformLocation(program_, "paletteInfluence"),
                frame.paletteInfluence);
    glUniform1f(glGetUniformLocation(program_, "bassLevel"), frame.bassLevel);
    glUniform1f(glGetUniformLocation(program_, "bassImpact"), frame.bassImpact);
    glUniform1f(glGetUniformLocation(program_, "midLevel"), frame.midLevel);
    glUniform1f(glGetUniformLocation(program_, "trebleLevel"), frame.trebleLevel);
    glUniform1f(glGetUniformLocation(program_, "midImpact"), frame.midImpact);
    glUniform1f(glGetUniformLocation(program_, "trebleImpact"), frame.trebleImpact);
    glUniform1i(glGetUniformLocation(program_, "asciiEnabled"),
                frame.asciiEnabled ? 1 : 0);
    glUniform1f(glGetUniformLocation(program_, "motionScale"),
                std::clamp(frame.motionScale, 0.0f, 1.0f));
    glUniform1f(glGetUniformLocation(program_, "contrastScale"),
                std::clamp(frame.contrastScale, 1.0f, 1.25f));
    glUniform1i(glGetUniformLocation(program_, "flashLimited"),
                frame.flashLimited ? 1 : 0);
    glUniform1i(glGetUniformLocation(program_, "colorVisionSafe"),
                frame.colorVisionSafe ? 1 : 0);
    glUniform1f(glGetUniformLocation(program_, "visibility"), frame.visibility);
    glBindVertexArray(vao_);
    gpuTimer_.begin();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    gpuTimer_.end();
    const GLenum glError = glGetError();
    if (glError != GL_NO_ERROR) {
        error = "live compositor OpenGL error " + std::to_string(glError);
        return false;
    }
    return true;
}

void LiveCompositor::shutdown() {
    gpuTimer_.shutdown();
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (program_) glDeleteProgram(program_);
    if (vertexShader_) glDeleteShader(vertexShader_);
    if (fragmentShader_) glDeleteShader(fragmentShader_);
    vao_ = 0;
    program_ = 0;
    vertexShader_ = 0;
    fragmentShader_ = 0;
}
