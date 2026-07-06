#include "MountManager.h"
#include "RcloneApiClient.h"
#include "config/Settings.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>

MountManager::MountManager(RcloneApiClient *client, Settings *settings, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_settings(settings)
{
    connect(m_client, &RcloneApiClient::mountsListed, this, &MountManager::onMountsListed);
}

void MountManager::setDefaultMountBasePath(const QString &path)
{
    m_defaultMountBasePath = path;
}

QString MountManager::defaultMountBasePath() const
{
    return m_defaultMountBasePath;
}

QList<MountInfo> MountManager::mounts() const
{
    return m_mounts.values();
}

bool MountManager::isMounted(const QString &remoteName) const
{
    return m_mounts.contains(remoteName) && m_mounts[remoteName].mounted;
}

QString MountManager::mountPointForRemote(const QString &remoteName) const
{
    return m_mounts.contains(remoteName) ? m_mounts[remoteName].mountPoint : QString();
}

void MountManager::mountRemote(const QString &remoteName)
{
    QString mp = m_settings->mountPathForRemote(remoteName);
    QString fs = fsFromRemote(remoteName);

    QDir dir(mp);
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            Q_EMIT mountFailed(remoteName, QStringLiteral("Cannot create mount point: %1").arg(mp));
            return;
        }
    }

    connect(m_client, &RcloneApiClient::mountDone, this,
        [this, remoteName, mp](bool success, const QString &error) {
            onMountDone(remoteName, mp, success, error);
        }, Qt::SingleShotConnection);

    m_client->mount(fs, mp);
}

void MountManager::unmountRemote(const QString &remoteName)
{
    QString mp = m_settings->mountPathForRemote(remoteName);

    connect(m_client, &RcloneApiClient::unmountDone, this,
        [this, remoteName](bool success, const QString &error) {
            onUnmountDone(remoteName, success, error);
        }, Qt::SingleShotConnection);

    m_client->unmount(mp);
}

void MountManager::toggleMount(const QString &remoteName)
{
    if (isMounted(remoteName))
        unmountRemote(remoteName);
    else
        mountRemote(remoteName);
}

void MountManager::refreshMounts()
{
    m_client->listMounts();
}

void MountManager::onMountDone(const QString &fs, const QString &mountPoint, bool success, const QString &error)
{
    Q_UNUSED(mountPoint)
    QString remoteName = remoteFromFs(fs);
    if (remoteName.isEmpty())
        remoteName = fs;

    if (success) {
        updateMountState(remoteName, true);
    } else {
        qWarning() << "Mount failed for" << remoteName << ":" << error;
        Q_EMIT mountFailed(remoteName, error);
    }
}

void MountManager::onUnmountDone(const QString &remoteName, bool success, const QString &error)
{
    if (success) {
        updateMountState(remoteName, false);
    } else {
        qWarning() << "Unmount failed for" << remoteName << ":" << error;
        Q_EMIT mountFailed(remoteName, error);
    }
}

void MountManager::onMountsListed(const QJsonArray &mounts)
{
    for (auto &info : m_mounts)
        info.mounted = false;

    for (const auto &v : mounts) {
        QJsonObject obj = v.toObject();
        QString fs = obj[QStringLiteral("fs")].toString();
        QString mp = obj[QStringLiteral("mountPoint")].toString();
        QString remoteName = remoteFromFs(fs);
        if (remoteName.isEmpty())
            continue;

        m_mounts[remoteName].remoteName = remoteName;
        m_mounts[remoteName].mountPoint = mp;
        m_mounts[remoteName].mounted = true;
    }

    Q_EMIT mountsRefreshed();
}

QString MountManager::fsFromRemote(const QString &remoteName) const
{
    return remoteName + QStringLiteral(":");
}

QString MountManager::remoteFromFs(const QString &fs) const
{
    if (fs.endsWith(QLatin1Char(':')))
        return fs.chopped(1);
    return {};
}

void MountManager::updateMountState(const QString &remoteName, bool mounted)
{
    m_mounts[remoteName].remoteName = remoteName;
    m_mounts[remoteName].mounted = mounted;
    if (!mounted)
        m_mounts[remoteName].mountPoint.clear();

    Q_EMIT mountStateChanged(remoteName, mounted);
}
