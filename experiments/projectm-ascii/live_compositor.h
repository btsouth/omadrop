#pragma once

#include <GL/glew.h>

#include "gpu_pass_timer.h"

#include <array>
#include <string>
#include <optional>

struct LiveCompositorFrame {
    GLuint sourceTexture = 0;
    GLuint nextTexture = 0;
    GLuint coverTexture = 0;
    int width = 0;
    int height = 0;
    float sceneMix = 0.0f;
    int transitionMode = 0;
    int sourceReactionMode = 0;
    int nextReactionMode = 0;
    std::array<float, 3> sourceReactionGain{};
    std::array<float, 3> nextReactionGain{};
    float asciiExposure = 1.0f;
    float fieldExposure = 1.0f;
    bool nativeRenderer = false;
    float coverAspect = 1.0f;
    float coverMix = 0.0f;
    std::array<float, 3> albumColor{0.72f, 0.82f, 1.0f};
    float paletteInfluence = 0.0f;
    float bassLevel = 0.0f;
    float bassImpact = 0.0f;
    float midLevel = 0.0f;
    float trebleLevel = 0.0f;
    float midImpact = 0.0f;
    float trebleImpact = 0.0f;
    bool asciiEnabled = false;
    float motionScale = 1.0f;
    float contrastScale = 1.0f;
    bool flashLimited = false;
    bool colorVisionSafe = false;
    float visibility = 1.0f;
};

class LiveCompositor {
public:
    LiveCompositor() = default;
    ~LiveCompositor();
    LiveCompositor(const LiveCompositor&) = delete;
    LiveCompositor& operator=(const LiveCompositor&) = delete;

    bool initialize(const char* vertexSource, const char* fragmentSource,
                    std::string& error);
    bool render(const LiveCompositorFrame& frame, std::string& error);
    std::optional<double> latestGpuMilliseconds() const {
        return gpuTimer_.latestMilliseconds();
    }
    std::uint64_t gpuTimingSerial() const { return gpuTimer_.sampleSerial(); }
    void shutdown();

private:
    GLuint vertexShader_ = 0;
    GLuint fragmentShader_ = 0;
    GLuint program_ = 0;
    GLuint vao_ = 0;
    GpuPassTimer gpuTimer_;
};
