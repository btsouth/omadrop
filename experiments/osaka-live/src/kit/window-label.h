#pragma once

#include <QString>
#include <string>

namespace Journey::Kit {
struct OsakaWindowNodeV1 {
    std::string id;
    std::string piece = "window";
    std::string profile = "generic-window-v1";
    int band = -1;
    bool kick = false;
    bool onset = false;
    bool always = false;
    bool explicitNode = false;
};

bool isWindowLabelCandidate(const QString& label);
OsakaWindowNodeV1 parseWindowLabel(const QString& svgFile, const QString& elementId,
                                   const QString& label);
}
