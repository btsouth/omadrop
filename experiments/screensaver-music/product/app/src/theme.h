#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

// Live view of the Omarchy theme accent. Follows
// ~/.local/state/omarchy/current/theme/colors.toml and falls back to the
// shipped default when the key is missing or unreadable.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent NOTIFY accentChanged)

public:
    explicit Theme(QObject* parent = nullptr);

    QString accent() const { return m_accent; }

signals:
    void accentChanged();

private:
    void watch();
    void reload();

    QString m_colorsPath;
    QString m_accent;
    QFileSystemWatcher m_watcher;
};
