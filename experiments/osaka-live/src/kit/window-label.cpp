#include "window-label.h"

#include <QStringList>
#include <stdexcept>

namespace Journey::Kit {
namespace {
[[noreturn]] void fail(const QString& svgFile, const QString& elementId, const QString& label,
                       const QString& reason) {
    throw std::runtime_error(
        QString("%1: element id '%2' label '%3': %4")
            .arg(svgFile, elementId, label, reason)
            .toStdString());
}
}

bool isWindowLabelCandidate(const QString& label) {
    if (label == "window" || label.startsWith("window.")) return true;
    // A dotted label is shorthand-shaped. Keep ordinary artist labels untouched.
    return label.contains('.');
}

OsakaWindowNodeV1 parseWindowLabel(const QString& svgFile, const QString& elementId,
                                   const QString& label) {
    const auto tokens = label.split('.', Qt::KeepEmptyParts);
    if (tokens.isEmpty() || tokens.front() != "window")
        fail(svgFile, elementId, label,
             QString("unknown piece '%1'").arg(tokens.isEmpty() ? QString() : tokens.front()));

    OsakaWindowNodeV1 node;
    node.id = elementId.toStdString();
    for (int i = 1; i < tokens.size(); ++i) {
        const QString token = tokens[i];
        const bool recognizedBand = token.size() == 5 && token.startsWith("band")
            && token[4] >= '0' && token[4] <= '5';
        if (recognizedBand) {
            if (node.band >= 0) fail(svgFile, elementId, label, "duplicate band '" + token + "'");
            node.band = token.mid(4).toInt();
            continue;
        }
        if (token == "kick") {
            if (node.kick) fail(svgFile, elementId, label, "duplicate token 'kick'");
            node.kick = true;
            continue;
        }
        if (token == "onset") {
            if (node.onset) fail(svgFile, elementId, label, "duplicate token 'onset'");
            node.onset = true;
            continue;
        }
        if (token == "always") {
            if (node.always) fail(svgFile, elementId, label, "duplicate token 'always'");
            node.always = true;
            continue;
        }
        fail(svgFile, elementId, label, "unknown token '" + token + "'");
    }
    if (node.band < 0) fail(svgFile, elementId, label, "missing band0..band5");
    return node;
}
}
