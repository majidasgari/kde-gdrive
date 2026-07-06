#pragma once

#include <QAbstractListModel>
#include <QJsonArray>

class RcloneApiClient;

class MountsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        RemoteNameRole = Qt::UserRole + 1,
        MountPointRole,
        MountedRole,
        FsTypeRole,
    };

    explicit MountsModel(RcloneApiClient *client, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void refreshDone();

private Q_SLOTS:
    void onMountsListed(const QJsonArray &mounts);

private:
    struct MountEntry {
        QString remoteName;
        QString mountPoint;
        bool mounted = false;
        QString fsType;
    };

    RcloneApiClient *m_client;
    QList<MountEntry> m_mounts;
};
