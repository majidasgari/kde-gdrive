#include "MountsModel.h"
#include "core/RcloneApiClient.h"

MountsModel::MountsModel(RcloneApiClient *client, QObject *parent)
    : QAbstractListModel(parent)
    , m_client(client)
{
    connect(m_client, &RcloneApiClient::mountsListed, this, &MountsModel::onMountsListed);
}

int MountsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_mounts.size();
}

QVariant MountsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_mounts.size())
        return {};

    const auto &entry = m_mounts[index.row()];
    switch (role) {
    case RemoteNameRole:
        return entry.remoteName;
    case MountPointRole:
        return entry.mountPoint;
    case MountedRole:
        return entry.mounted;
    case FsTypeRole:
        return entry.fsType;
    default:
        return {};
    }
}

QHash<int, QByteArray> MountsModel::roleNames() const
{
    return {
        { RemoteNameRole, "remoteName" },
        { MountPointRole, "mountPoint" },
        { MountedRole, "mounted" },
        { FsTypeRole, "fsType" },
    };
}

void MountsModel::refresh()
{
    m_client->listMounts();
}

void MountsModel::onMountsListed(const QJsonArray &mounts)
{
    beginResetModel();
    m_mounts.clear();
    for (const auto &v : mounts) {
        QJsonObject obj = v.toObject();
        MountEntry entry;
        QString fs = obj[QStringLiteral("fs")].toString();
        if (fs.endsWith(QLatin1Char(':')))
            entry.remoteName = fs.chopped(1);
        else
            entry.remoteName = fs;
        entry.mountPoint = obj[QStringLiteral("mountPoint")].toString();
        entry.mounted = true;
        entry.fsType = obj[QStringLiteral("fsType")].toString();
        m_mounts.append(entry);
    }
    endResetModel();
    Q_EMIT refreshDone();
}
