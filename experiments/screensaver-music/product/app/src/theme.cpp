#include "theme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace {
const QString kFallbackAccent = QStringLiteral("#a7d8cf");

bool isHexColor(const QString& value) {
    static const QRegularExpression pattern(QStringLiteral("^#[0-9a-fA-F]{6}$"));
    return pattern.match(value).hasMatch();
}

QString stateHome() {
    const QString override = qEnvironmentVariable("XDG_STATE_HOME");
    if (!override.isEmpty()) {
        return override;
    }
    return QDir::homePath() + QStringLiteral("/.local/state");
}
} // namespace

Theme::Theme(QObject* parent)
    : QObject(parent),
      m_colorsPath(stateHome() + QStringLiteral("/omarchy/current/theme/colors.toml")),
      m_accent(kFallbackAccent) {
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Theme::reload);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Theme::reload);
    watch();
    reload();
}

void Theme::watch() {
    if (!m_watcher.files().isEmpty()) m_watcher.removePaths(m_watcher.files());
    if (!m_watcher.directories().isEmpty()) m_watcher.removePaths(m_watcher.directories());
    if (QFileInfo::exists(m_colorsPath) && !m_watcher.files().contains(m_colorsPath)) {
        m_watcher.addPath(m_colorsPath);
    }
    const QString directory = QFileInfo(m_colorsPath).absolutePath();
    if (QDir(directory).exists() && !m_watcher.directories().contains(directory)) {
        m_watcher.addPath(directory);
    }
    // Theme switches replace symlinks, so watch their parents as well.
    for (const QString& parent : {QFileInfo(directory).absolutePath(), stateHome() + QStringLiteral("/omarchy")}) {
        if (QDir(parent).exists() && !m_watcher.directories().contains(parent))
            m_watcher.addPath(parent);
    }
}

void Theme::reload() {
    // A replaced colors.toml drops the old watch; re-add before reading.
    watch();
    QString accent = kFallbackAccent;
    QFile file(m_colorsPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!file.atEnd()) {
            const QString line = QString::fromUtf8(file.readLine()).trimmed();
            const int separator = line.indexOf(QLatin1Char('='));
            if (separator <= 0) {
                continue;
            }
            if (line.left(separator).trimmed() != QLatin1String("accent")) {
                continue;
            }
            QString value = line.mid(separator + 1).trimmed();
            value.remove(QLatin1Char('"'));
            value.remove(QLatin1Char('\''));
            if (isHexColor(value)) {
                accent = value.toLower();
            }
            break;
        }
    }
    if (accent != m_accent) {
        m_accent = accent;
        emit accentChanged();
    }
}
