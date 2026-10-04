#pragma once

#include <QString>

namespace Journey {
// Surfaceless EGL context (Mesa or NVIDIA) for offscreen export. No display.
class HeadlessContext {
public:
    ~HeadlessContext();
    bool create(QString& error);
    QString renderer() const { return renderer_; }
private:
    void* display_ = nullptr;
    void* context_ = nullptr;
    QString renderer_;
};
}
