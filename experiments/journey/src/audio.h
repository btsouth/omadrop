#pragma once

#include "scene.h"
#include <QString>
#include <vector>

namespace Journey {
class AudioTimeline {
public:
    // Warm all feature/surge history from sample zero before applying the offset.
    bool load(const QString& path, double offset, double duration, QString& error);
    Audio at(double movieSeconds) const;

private:
    struct Frame { double seconds; Audio audio; };
    std::vector<Frame> frames_;
    double offset_ = 0;
};
}
