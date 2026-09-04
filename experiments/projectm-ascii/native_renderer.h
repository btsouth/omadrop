#pragma once

#include "music_frame.h"
#include "native_scene_state.h"
#include "gpu_pass_timer.h"

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <optional>
#include <string>

struct NativeRenderPolicy {
    float intensity = 1.0f;
    float motion = 1.0f;
    bool reducedMotion = false;
    bool flashLimited = false;
    float quality = 1.0f;

    constexpr float effectiveIntensity() const {
        const float requested = std::clamp(intensity, 0.50f, 1.50f);
        return flashLimited ? std::min(requested, 0.85f) : requested;
    }
    constexpr float effectiveMotion() const {
        const float requested = std::clamp(motion, 0.0f, 1.0f);
        return reducedMotion ? std::min(requested, 0.35f) : requested;
    }
    constexpr float effectiveQuality() const {
        return std::clamp(quality, 0.50f, 1.0f);
    }
};

static_assert(NativeRenderPolicy{.intensity = 9.0f}.effectiveIntensity()
              == 1.50f);
static_assert(NativeRenderPolicy{.intensity = 1.25f, .flashLimited = true}
                  .effectiveIntensity() == 0.85f);
static_assert(NativeRenderPolicy{.motion = 0.8f, .reducedMotion = true}
                  .effectiveMotion() == 0.35f);

class NativeRenderer {
public:
    NativeRenderer() = default;
    ~NativeRenderer();
    NativeRenderer(const NativeRenderer&) = delete;
    NativeRenderer& operator=(const NativeRenderer&) = delete;

    bool initialize(const std::filesystem::path& shaderDirectory, std::string& error);
    bool initializeCustomShader(const std::filesystem::path& vertexPath,
                                const std::filesystem::path& fragmentPath,
                                std::string& error);
    bool render(const MusicFrame& music, const NativeSceneState& scene,
                int width, int height,
                const std::array<float, 3>& albumColor,
                GLuint artworkTexture, float artworkAspect, float frameSeconds,
                std::string& error,
                const NativeRenderPolicy& policy = NativeRenderPolicy{});
    GLuint texture(NativeSceneKind scene) const;
    GLuint texture() const { return texture(NativeSceneKind::DepthTunnel); }
    float flowTime() const { return flowTime_; }
    std::optional<double> latestGpuMilliseconds() const {
        return gpuTimer_.latestMilliseconds();
    }
    std::uint64_t gpuTimingSerial() const { return gpuTimer_.sampleSerial(); }
    void synchronizeFlowTime(float flowTime) {
        if (std::isfinite(flowTime) && flowTime >= 0.0f) flowTime_ = flowTime;
    }
    void reset();
    void shutdown();

private:
    void initializeTargets();
    bool resize(int width, int height, std::string& error);

    std::array<GLuint, nativeSceneCount> programs_{};
    GLuint vao_ = 0;
    GLuint framebuffer_ = 0;
    std::array<std::array<GLuint, 2>, nativeSceneCount> textures_{};
    std::array<int, nativeSceneCount> activeTextures_{};
    int width_ = 0;
    int height_ = 0;
    float flowTime_ = 0.0f;
    GpuPassTimer gpuTimer_;
};
