#pragma once

#include "scene.h"
#include <QString>
#include <vector>

namespace Journey {
class AudioTimeline {
public:
    // Always analyze from sample zero, including the history before the offset.
    bool load(const QString& path, double offset, double duration, QString& error);
    Audio at(double movieSeconds) const;

private:
    struct Frame { double seconds; Audio audio; };
    std::vector<Frame> frames_;
    double offset_ = 0;
};
}
