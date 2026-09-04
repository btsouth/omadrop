#pragma once

#include <GL/glew.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

class GpuPassTimer {
public:
    GpuPassTimer() = default;
    ~GpuPassTimer() { shutdown(); }
    GpuPassTimer(const GpuPassTimer&) = delete;
    GpuPassTimer& operator=(const GpuPassTimer&) = delete;

    bool initialize() {
        shutdown();
        if (!(GLEW_VERSION_3_3 || GLEW_ARB_timer_query)) return false;
        glGenQueries(static_cast<GLsizei>(queries_.size()), queries_.data());
        supported_ = queries_[0] != 0;
        return supported_;
    }

    void begin() {
        if (!supported_ || activeSlot_ >= 0) return;
        collect();
        for (std::size_t offset = 0; offset < queries_.size(); ++offset) {
            const std::size_t slot = (nextSlot_ + offset) % queries_.size();
            if (pending_[slot]) continue;
            glBeginQuery(GL_TIME_ELAPSED, queries_[slot]);
            activeSlot_ = static_cast<int>(slot);
            querySequence_[slot] = nextSequence_++;
            nextSlot_ = (slot + 1) % queries_.size();
            return;
        }
    }

    void end() {
        if (!supported_ || activeSlot_ < 0) return;
        glEndQuery(GL_TIME_ELAPSED);
        pending_[static_cast<std::size_t>(activeSlot_)] = true;
        activeSlot_ = -1;
    }

    void collect() {
        if (!supported_) return;
        for (std::size_t slot = 0; slot < queries_.size(); ++slot) {
            if (!pending_[slot]) continue;
            GLint available = GL_FALSE;
            glGetQueryObjectiv(queries_[slot], GL_QUERY_RESULT_AVAILABLE,
                               &available);
            if (!available) continue;
            GLuint64 nanoseconds = 0;
            glGetQueryObjectui64v(queries_[slot], GL_QUERY_RESULT,
                                  &nanoseconds);
            if (querySequence_[slot] > latestSequence_) {
                latestMilliseconds_
                    = static_cast<double>(nanoseconds) / 1000000.0;
                latestSequence_ = querySequence_[slot];
            }
            pending_[slot] = false;
        }
    }

    std::optional<double> latestMilliseconds() const {
        if (latestSequence_ == 0) return std::nullopt;
        return latestMilliseconds_;
    }

    std::uint64_t sampleSerial() const { return latestSequence_; }
    bool supported() const { return supported_; }

    void shutdown() {
        if (activeSlot_ >= 0) {
            glEndQuery(GL_TIME_ELAPSED);
            activeSlot_ = -1;
        }
        if (queries_[0]) {
            glDeleteQueries(static_cast<GLsizei>(queries_.size()),
                            queries_.data());
        }
        queries_ = {};
        pending_ = {};
        querySequence_ = {};
        nextSlot_ = 0;
        nextSequence_ = 1;
        latestMilliseconds_ = 0.0;
        latestSequence_ = 0;
        supported_ = false;
    }

private:
    static constexpr std::size_t slotCount = 4;
    std::array<GLuint, slotCount> queries_{};
    std::array<bool, slotCount> pending_{};
    std::array<std::uint64_t, slotCount> querySequence_{};
    std::size_t nextSlot_ = 0;
    std::uint64_t nextSequence_ = 1;
    int activeSlot_ = -1;
    double latestMilliseconds_ = 0.0;
    std::uint64_t latestSequence_ = 0;
    bool supported_ = false;
};
