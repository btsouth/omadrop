#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

// Live view of the Omarchy theme colors. Follows
// ~/.local/state/omarchy/current/theme/colors.toml and falls back to the
// shipped default when the key is missing or unreadable.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent NOTIFY accentChanged)
    Q_PROPERTY(QString background READ background NOTIFY accentChanged)
    Q_PROPERTY(QString foreground READ foreground NOTIFY accentChanged)
    Q_PROPERTY(QString red READ red NOTIFY accentChanged)

public:
    explicit Theme(QObject* parent = nullptr);

    QString accent() const { return m_accent; }
    QString background() const { return m_background; }
    QString foreground() const { return m_foreground; }
    QString red() const { return m_red; }

signals:
    void accentChanged();

private:
    void watch();
    void reload();

    QString m_colorsPath;
    QString m_accent;
    QString m_background;
    QString m_foreground;
    QString m_red;
    QFileSystemWatcher m_watcher;
};
