#pragma once
#include "../canvas.h"
#include <QPainterPath>
#include <QTransform>
#include <QString>
#include <memory>
#include <functional>
#include <array>

namespace Journey::Kit {
// Supported SVG subset version. Art is compiled once; callers retain ownership
// for as long as any Canvas appended from it is used by Gpu.
inline constexpr int SvgSubsetVersion = 1;
struct SvgPathCommand { char code; std::array<double,6> v{}; };
struct SvgElement {
    QString id, label, tag;
    qint64 line = 0;
    QPainterPath geometry; // local coordinates, including converted arcs
    QTransform transform; // local to the 1920 x 1080 design space
    std::shared_ptr<const Canvas> canvas; // geometry and paint, immutable
    std::function<void(Canvas&, const Col*, double)> replay;
};
class SvgArt {
public:
    const std::vector<SvgElement>& elements() const { return elements_; }
    bool draw(Canvas& target, const QString& id) const;
    // Replay into an existing retained span at its original insertion point.
    // Optional tint serves reusable outlined glyphs; parsing happens at import.
    bool replay(Canvas&, const QString& id, const Col* tint = nullptr, double alpha = 1) const;
    bool fillGradient(Canvas&, const QString& id, const Col& top, const Col& bottom,
                      double alpha) const;
    void draw(Canvas& target) const { target.appendOwned(root_); }
private:
    friend class SvgCompiler;
    std::vector<SvgElement> elements_;
    std::shared_ptr<const Canvas> root_;
};
struct SvgImport {
    std::shared_ptr<const SvgArt> art;
    QString diagnostic;
    explicit operator bool() const { return bool(art); }
};
// Elements whose label the filter accepts are recorded but left out of the plain
// art (draw() without an id). A piece that draws them itself uses this.
using LabelFilter = std::function<bool(const QString&)>;
SvgImport importSvg(const QString& filename, double pixelScale = 1.0, LabelFilter hideLabel = {});
SvgImport compileSvg(const QByteArray& xml, const QString& filename, double pixelScale = 1.0, LabelFilter hideLabel = {});
// The same M/L/Q/C/Z replay boundary as the outlined sign art. Exposed for
// numerical arc/transform tests; malformed input returns an empty path.
bool svgPath(const QString& data, QPainterPath& path, QString& error, std::vector<SvgPathCommand>* commands = nullptr);
bool svgTransform(const QString& data, QTransform& transform, QString& error);
}
