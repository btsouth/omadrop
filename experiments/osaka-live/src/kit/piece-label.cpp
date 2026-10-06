#include "piece-label.h"

#include "svg-art.h"
#include <QStringList>
#include <stdexcept>

namespace Journey::Kit {
namespace {
struct PieceDefinition {
    const char* name;
    const char* profile;
    QStringList tokens;
    bool replacesArt;
    bool needsReplay;
};

const PieceDefinition* definitions(int& count) {
    static const PieceDefinition list[] = {
        {"lantern", "generic-lantern-v1", {"kick", "sway"}, true, false},
        {"lamp", "generic-lamp-v1", {"kick"}, false, false},
        {"neon", "generic-neon-v1", {"kick", "flicker"}, true, true},
        {"glow", "generic-glow-v1", {"kick", "onset", "always"}, false, true},
        {"wire", "generic-wire-v1", {"pulse"}, true, false},
    };
    count = int(sizeof(list) / sizeof(list[0]));
    return list;
}

const PieceDefinition* find(const QString& label) {
    int count = 0;
    const auto* list = definitions(count);
    for (int i = 0; i < count; ++i) {
        const QString name = list[i].name;
        if (label == name || label.startsWith(name + "."))
            return &list[i];
    }
    return nullptr;
}

[[noreturn]] void fail(const QString& svgFile, const QString& elementId, const QString& label,
                       const QString& reason) {
    throw std::runtime_error(
        QString("%1: element id '%2' label '%3': %4")
            .arg(svgFile, elementId, label, reason)
            .toStdString());
}
}

bool isPieceLabelCandidate(const QString& label) { return find(label) != nullptr; }

bool pieceReplacesArt(const QString& label) {
    const auto* definition = find(label);
    return definition && definition->replacesArt;
}

OsakaPieceNodeV1 parsePieceLabel(const QString& svgFile, const QString& elementId,
                                 const QString& label) {
    const auto* definition = find(label);
    const auto tokens = label.split('.', Qt::KeepEmptyParts);
    if (!definition)
        fail(svgFile, elementId, label,
             QString("unknown piece '%1'").arg(tokens.isEmpty() ? QString() : tokens.front()));

    OsakaPieceNodeV1 node;
    node.id = elementId.toStdString();
    node.piece = definition->name;
    node.profile = definition->profile;
    QStringList seen;
    for (int i = 1; i < tokens.size(); ++i) {
        const QString token = tokens[i];
        const bool recognizedBand = token.size() == 5 && token.startsWith("band")
            && token[4] >= '0' && token[4] <= '5';
        if (recognizedBand) {
            if (node.band >= 0) fail(svgFile, elementId, label, "duplicate band '" + token + "'");
            node.band = token.mid(4).toInt();
            continue;
        }
        if (!definition->tokens.contains(token))
            fail(svgFile, elementId, label, "unknown token '" + token + "'");
        if (seen.contains(token)) fail(svgFile, elementId, label, "duplicate token '" + token + "'");
        seen << token;
        if (token == "kick") node.kick = true;
        else if (token == "onset") node.onset = true;
        else if (token == "always") node.always = true;
        else if (token == "sway") node.sway = true;
        else if (token == "flicker") node.flicker = true;
        else if (token == "pulse") node.pulse = true;
    }
    if (node.band < 0) fail(svgFile, elementId, label, "missing band0..band5");
    return node;
}

void checkPieceElement(const QString& svgFile, const SvgElement& element,
                       const OsakaPieceNodeV1& node) {
    const QString piece = QString::fromStdString(node.piece);
    const auto* definition = find(piece);
    if (element.geometry.isEmpty())
        fail(svgFile, element.id, element.label,
             QString("%1 needs a shape (path, rect, circle, ellipse, line, polygon or polyline), "
                     "not <%2>").arg(piece, element.tag));
    if (definition && definition->needsReplay && !element.replay)
        fail(svgFile, element.id, element.label,
             QString("%1 cannot redraw this element (avoid use references, clipping and opacity on a group)")
                 .arg(piece));
}
}
