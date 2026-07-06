#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

#include <KConfig>
#include <KConfigGroup>

class Settings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int rclonePort READ rclonePort WRITE setRclonePort NOTIFY settingsChanged)
    Q_PROPERTY(QString rcUser READ rcUser WRITE setRcUser NOTIFY settingsChanged)
    Q_PROPERTY(QString rcPass READ rcPass WRITE setRcPass NOTIFY settingsChanged)
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    Q_PROPERTY(QString syncResyncMode READ syncResyncMode WRITE setSyncResyncMode NOTIFY settingsChanged)
    Q_PROPERTY(QString syncTimes READ syncTimes WRITE setSyncTimes NOTIFY settingsChanged)
    Q_PROPERTY(bool syncEnabled READ syncEnabled WRITE setSyncEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString syncExcludePatterns READ syncExcludePatterns WRITE setSyncExcludePatterns NOTIFY settingsChanged)
    Q_PROPERTY(QString dedupeMode READ dedupeMode WRITE setDedupeMode NOTIFY settingsChanged)

public:
    explicit Settings(QObject *parent = nullptr);

    QString syncExcludePatterns() const;
    void setSyncExcludePatterns(const QString &patterns);

    QString dedupeMode() const;
    void setDedupeMode(const QString &mode);

    int rclonePort() const;
    void setRclonePort(int port);

    QString rcUser() const;
    void setRcUser(const QString &user);

    QString rcPass() const;
    void setRcPass(const QString &pass);

    bool autoStartDaemon() const;
    void setAutoStartDaemon(bool enabled);

    bool autoMountOnStart() const;
    void setAutoMountOnStart(bool enabled);

    QString rclonePath() const;
    void setRclonePath(const QString &path);

    QString applicationName() const;

    // Bisync settings (global)
    QString syncResyncMode() const;
    void setSyncResyncMode(const QString &mode);

    QString syncTimes() const;
    void setSyncTimes(const QString &times);

    bool syncEnabled() const;
    void setSyncEnabled(bool enabled);

    // Per-remote paths
    Q_INVOKABLE QString syncPathForRemote(const QString &remoteName) const;
    Q_INVOKABLE void setSyncPathForRemote(const QString &remoteName, const QString &path);

    Q_INVOKABLE QString mountPathForRemote(const QString &remoteName) const;
    Q_INVOKABLE void setMountPathForRemote(const QString &remoteName, const QString &path);

Q_SIGNALS:
    void settingsChanged();

private:
    KConfigGroup syncGroup() const;
    KConfigGroup group() const;

    KConfig m_config;
};
