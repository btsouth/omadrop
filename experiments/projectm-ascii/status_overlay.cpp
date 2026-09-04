#include "status_overlay.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace {
constexpr int glyphWidth = 5;
constexpr int glyphHeight = 7;
constexpr int glyphScale = 3;
constexpr int horizontalPadding = 12;
constexpr int verticalPadding = 9;
constexpr std::size_t maximumCharacters = 48;
constexpr std::size_t maximumLines = 3;
constexpr int lineGap = 6;

std::array<unsigned char, glyphHeight> glyph(char character) {
    switch (character) {
        case 'A': return {14, 17, 17, 31, 17, 17, 17};
        case 'B': return {30, 17, 17, 30, 17, 17, 30};
        case 'C': return {14, 17, 16, 16, 16, 17, 14};
        case 'D': return {30, 17, 17, 17, 17, 17, 30};
        case 'E': return {31, 16, 16, 30, 16, 16, 31};
        case 'F': return {31, 16, 16, 30, 16, 16, 16};
        case 'G': return {14, 17, 16, 23, 17, 17, 15};
        case 'H': return {17, 17, 17, 31, 17, 17, 17};
        case 'I': return {31, 4, 4, 4, 4, 4, 31};
        case 'J': return {7, 2, 2, 2, 18, 18, 12};
        case 'K': return {17, 18, 20, 24, 20, 18, 17};
        case 'L': return {16, 16, 16, 16, 16, 16, 31};
        case 'M': return {17, 27, 21, 21, 17, 17, 17};
        case 'N': return {17, 25, 21, 19, 17, 17, 17};
        case 'O': return {14, 17, 17, 17, 17, 17, 14};
        case 'P': return {30, 17, 17, 30, 16, 16, 16};
        case 'Q': return {14, 17, 17, 17, 21, 18, 13};
        case 'R': return {30, 17, 17, 30, 20, 18, 17};
        case 'S': return {15, 16, 16, 14, 1, 1, 30};
        case 'T': return {31, 4, 4, 4, 4, 4, 4};
        case 'U': return {17, 17, 17, 17, 17, 17, 14};
        case 'V': return {17, 17, 17, 17, 17, 10, 4};
        case 'W': return {17, 17, 17, 21, 21, 21, 10};
        case 'X': return {17, 17, 10, 4, 10, 17, 17};
        case 'Y': return {17, 17, 10, 4, 4, 4, 4};
        case 'Z': return {31, 1, 2, 4, 8, 16, 31};
        case '0': return {14, 17, 19, 21, 25, 17, 14};
        case '1': return {4, 12, 4, 4, 4, 4, 14};
        case '2': return {14, 17, 1, 2, 4, 8, 31};
        case '3': return {30, 1, 1, 14, 1, 1, 30};
        case '4': return {2, 6, 10, 18, 31, 2, 2};
        case '5': return {31, 16, 16, 30, 1, 1, 30};
        case '6': return {14, 16, 16, 30, 17, 17, 14};
        case '7': return {31, 1, 2, 4, 8, 8, 8};
        case '8': return {14, 17, 17, 14, 17, 17, 14};
        case '9': return {14, 17, 17, 15, 1, 1, 14};
        case '-': return {0, 0, 0, 31, 0, 0, 0};
        case ':': return {0, 4, 4, 0, 4, 4, 0};
        case '%': return {17, 2, 4, 8, 17, 0, 0};
        case '/': return {1, 2, 2, 4, 8, 8, 16};
        case '[': return {14, 8, 8, 8, 8, 8, 14};
        case ']': return {14, 2, 2, 2, 2, 2, 14};
        case '?': return {14, 17, 1, 2, 4, 0, 4};
        default: return {};
    }
}

GLuint compile(GLenum type, const char* source, std::string& error) {
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

constexpr const char* vertexSource = R"GLSL(
#version 330 core
out vec2 statusUv;
uniform vec4 statusRect;
void main() {
    vec2 corner = vec2(
        float((gl_VertexID == 1 || gl_VertexID == 2 || gl_VertexID == 4)),
        float((gl_VertexID == 2 || gl_VertexID == 4 || gl_VertexID == 5)));
    statusUv = vec2(corner.x, 1.0 - corner.y);
    vec2 position = mix(statusRect.xy, statusRect.zw, corner);
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

constexpr const char* fragmentSource = R"GLSL(
#version 330 core
in vec2 statusUv;
out vec4 color;
uniform sampler2D statusTexture;
uniform float statusOpacity;
void main() {
    vec4 sampleColor = texture(statusTexture, statusUv);
    color = vec4(sampleColor.rgb, sampleColor.a * statusOpacity);
}
)GLSL";
}

StatusBitmap rasterizeStatusLabel(std::string_view supplied) {
    std::vector<std::string> lines(1);
    for (const unsigned char character : supplied) {
        if (character == '\n') {
            if (lines.size() >= maximumLines) break;
            while (!lines.back().empty() && lines.back().back() == ' ') {
                lines.back().pop_back();
            }
            lines.emplace_back();
        } else if (lines.back().size() < maximumCharacters) {
            lines.back() += static_cast<char>(std::toupper(character));
        }
    }
    while (!lines.empty() && lines.back().empty()) lines.pop_back();
    if (lines.empty()) return {};

    const std::size_t widestLine = std::max_element(
        lines.begin(), lines.end(), [](const auto& left, const auto& right) {
            return left.size() < right.size();
        })->size();

    StatusBitmap bitmap;
    bitmap.width = horizontalPadding * 2
        + static_cast<int>(widestLine) * (glyphWidth + 1) * glyphScale
        - glyphScale;
    bitmap.height = verticalPadding * 2
        + static_cast<int>(lines.size()) * glyphHeight * glyphScale
        + static_cast<int>(lines.size() - 1) * lineGap;
    bitmap.rgba.resize(static_cast<std::size_t>(bitmap.width * bitmap.height * 4));

    constexpr float radius = 9.0f;
    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            const float dx = std::max(
                radius - x, x - (bitmap.width - 1 - radius));
            const float dy = std::max(
                radius - y, y - (bitmap.height - 1 - radius));
            const bool inside = dx <= 0.0f || dy <= 0.0f
                || dx * dx + dy * dy <= radius * radius;
            const std::size_t offset
                = static_cast<std::size_t>(y * bitmap.width + x) * 4;
            bitmap.rgba[offset] = 7;
            bitmap.rgba[offset + 1] = 10;
            bitmap.rgba[offset + 2] = 17;
            bitmap.rgba[offset + 3] = inside ? 218 : 0;
        }
    }

    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        const std::string& text = lines[lineIndex];
        int glyphX = (bitmap.width
            - (static_cast<int>(text.size()) * (glyphWidth + 1) * glyphScale
               - glyphScale)) / 2;
        const int lineY = verticalPadding
            + static_cast<int>(lineIndex) * (glyphHeight * glyphScale + lineGap);
        for (const char character : text) {
            const auto rows = glyph(character);
            for (int row = 0; row < glyphHeight; ++row) {
                for (int column = 0; column < glyphWidth; ++column) {
                    if ((rows[row] & (1u << (glyphWidth - 1 - column))) == 0) {
                        continue;
                    }
                    for (int sy = 0; sy < glyphScale; ++sy) {
                        for (int sx = 0; sx < glyphScale; ++sx) {
                            const int x = glyphX + column * glyphScale + sx;
                            const int y = lineY + row * glyphScale + sy;
                            const std::size_t offset
                                = static_cast<std::size_t>(
                                    y * bitmap.width + x) * 4;
                            bitmap.rgba[offset] = 238;
                            bitmap.rgba[offset + 1] = 244;
                            bitmap.rgba[offset + 2] = 255;
                            bitmap.rgba[offset + 3] = 255;
                        }
                    }
                }
            }
            glyphX += (glyphWidth + 1) * glyphScale;
        }
    }
    return bitmap;
}

StatusOverlay::~StatusOverlay() {
    shutdown();
}

bool StatusOverlay::initialize(std::string& error) {
    shutdown();
    vertexShader_ = compile(GL_VERTEX_SHADER, vertexSource, error);
    if (!vertexShader_) return false;
    fragmentShader_ = compile(GL_FRAGMENT_SHADER, fragmentSource, error);
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
        error = "could not link status overlay";
        shutdown();
        return false;
    }
    glGenVertexArrays(1, &vao_);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return glGetError() == GL_NO_ERROR;
}

void StatusOverlay::show(std::string_view text, std::uint64_t nowMilliseconds,
                         std::uint64_t durationMilliseconds) {
    if (!texture_) return;
    const StatusBitmap bitmap = rasterizeStatusLabel(text);
    if (bitmap.rgba.empty()) return;
    width_ = bitmap.width;
    height_ = bitmap.height;
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, bitmap.rgba.data());
    shownAt_ = nowMilliseconds;
    duration_ = std::max<std::uint64_t>(durationMilliseconds, 300);
    active_ = true;
}

bool StatusOverlay::render(int outputWidth, int outputHeight,
                           std::uint64_t nowMilliseconds,
                           std::string& error) {
    if (!program_ || !texture_ || width_ <= 0 || height_ <= 0 || !active_) {
        return true;
    }
    if (nowMilliseconds < shownAt_) return true;
    const std::uint64_t age = nowMilliseconds - shownAt_;
    if (age >= duration_) return true;
    if (outputWidth <= 0 || outputHeight <= 0) return true;
    const std::uint64_t fadeDuration = std::min<std::uint64_t>(450, duration_);
    const std::uint64_t fadeStart = duration_ - fadeDuration;
    const float opacity = age <= fadeStart
        ? 1.0f : 1.0f - (age - fadeStart) / static_cast<float>(fadeDuration);
    const float left = 0.5f - width_ / (2.0f * outputWidth);
    const float bottom = std::max(24.0f, outputHeight * 0.035f) / outputHeight;
    const float right = left + width_ / static_cast<float>(outputWidth);
    const float top = bottom + height_ / static_cast<float>(outputHeight);

    while (glGetError() != GL_NO_ERROR) {}
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, outputWidth, outputHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glUniform1i(glGetUniformLocation(program_, "statusTexture"), 0);
    glUniform1f(glGetUniformLocation(program_, "statusOpacity"), opacity);
    glUniform4f(glGetUniformLocation(program_, "statusRect"),
                left, bottom, right, top);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_BLEND);
    const GLenum glError = glGetError();
    if (glError != GL_NO_ERROR) {
        error = "status overlay OpenGL error " + std::to_string(glError);
        return false;
    }
    return true;
}

void StatusOverlay::shutdown() {
    if (texture_) glDeleteTextures(1, &texture_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (program_) glDeleteProgram(program_);
    if (fragmentShader_) glDeleteShader(fragmentShader_);
    if (vertexShader_) glDeleteShader(vertexShader_);
    texture_ = 0;
    vao_ = 0;
    program_ = 0;
    fragmentShader_ = 0;
    vertexShader_ = 0;
    width_ = 0;
    height_ = 0;
    shownAt_ = 0;
    duration_ = 1800;
    active_ = false;
}
