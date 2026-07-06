#pragma once

#include <QObject>
#include <QJsonArray>
#include <QMap>

class RcloneApiClient;
class Settings;

struct MountInfo {
    QString remoteName;
    QString mountPoint;
    bool mounted = false;
};

class MountManager : public QObject
{
    Q_OBJECT

public:
    MountManager(RcloneApiClient *client, Settings *settings, QObject *parent = nullptr);

    void setDefaultMountBasePath(const QString &path);
    QString defaultMountBasePath() const;

    QList<MountInfo> mounts() const;
    bool isMounted(const QString &remoteName) const;
    QString mountPointForRemote(const QString &remoteName) const;

public Q_SLOTS:
    void mountRemote(const QString &remoteName);
    void unmountRemote(const QString &remoteName);
    void toggleMount(const QString &remoteName);
    void refreshMounts();

Q_SIGNALS:
    void mountStateChanged(const QString &remoteName, bool mounted);
    void mountFailed(const QString &remoteName, const QString &error);
    void mountsRefreshed();

private Q_SLOTS:
    void onMountDone(const QString &fs, const QString &mountPoint, bool success, const QString &error);
    void onUnmountDone(const QString &mountPoint, bool success, const QString &error);
    void onMountsListed(const QJsonArray &mounts);

private:
    QString fsFromRemote(const QString &remoteName) const;
    QString remoteFromFs(const QString &fs) const;
    void updateMountState(const QString &remoteName, bool mounted);

    RcloneApiClient *m_client;
    Settings *m_settings;
    QString m_defaultMountBasePath;
    QMap<QString, MountInfo> m_mounts;
};
