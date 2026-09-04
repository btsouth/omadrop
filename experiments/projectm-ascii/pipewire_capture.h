#pragma once

#include <cstddef>
#include <string>

class PipeWireCapture {
public:
    PipeWireCapture() = default;
    ~PipeWireCapture();
    PipeWireCapture(const PipeWireCapture&) = delete;
    PipeWireCapture& operator=(const PipeWireCapture&) = delete;

    bool start(const std::string& targetSink);
    bool running();
    void stop();
    std::size_t read(float* samples, std::size_t capacity);

private:
    int descriptor_ = -1;
    int process_ = -1;
};
