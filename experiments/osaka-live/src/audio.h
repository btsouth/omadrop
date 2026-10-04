#pragma once
#include "scene.h"
#include <functional>
#include <memory>
#include <cstddef>

namespace Journey {
class StreamingAudio {
public:
    using Consumer = std::function<void(const Audio&, double)>;
    StreamingAudio();
    ~StreamingAudio();
    void push(const float* stereo, std::size_t frames, const Consumer& consume);
    Audio current() const;
    double gain() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
