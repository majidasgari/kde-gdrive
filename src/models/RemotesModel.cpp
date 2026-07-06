#include "RemotesModel.h"
#include "core/RcloneApiClient.h"

#include <QJsonDocument>

RemotesModel::RemotesModel(RcloneApiClient *client, QObject *parent)
    : QAbstractListModel(parent)
    , m_client(client)
{
    connect(m_client, &RcloneApiClient::remotesListed, this, &RemotesModel::onRemotesListed);
    connect(m_client, &RcloneApiClient::configDumpReceived, this, &RemotesModel::onConfigDumpReceived);
}

int RemotesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_remotes.size();
}

QVariant RemotesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_remotes.size())
        return {};

    const auto &entry = m_remotes[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return entry.name;
    case TypeRole:
        return entry.type;
    case DescriptionRole:
        return entry.type == QStringLiteral("drive") ? QStringLiteral("Google Drive") : entry.type;
    default:
        return {};
    }
}

QHash<int, QByteArray> RemotesModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { TypeRole, "type" },
        { DescriptionRole, "description" },
    };
}

QStringList RemotesModel::remoteNames() const
{
    QStringList names;
    for (const auto &r : m_remotes)
        names.append(r.name);
    return names;
}

QString RemotesModel::remoteConfigJson(int index) const
{
    if (index < 0 || index >= m_remotes.size())
        return {};
    QJsonDocument doc(m_remotes[index].config);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString RemotesModel::remoteType(int index) const
{
    if (index < 0 || index >= m_remotes.size())
        return {};
    return m_remotes[index].type;
}

int RemotesModel::rowForName(const QString &name) const
{
    for (int i = 0; i < m_remotes.size(); i++) {
        if (m_remotes[i].name == name)
            return i;
    }
    return -1;
}

void RemotesModel::refresh()
{
    m_client->listRemotes();
}

void RemotesModel::onRemotesListed(const QStringList &remotes)
{
    beginResetModel();
    m_remotes.clear();
    for (const auto &name : remotes)
        m_remotes.append({ name, {}, {} });
    endResetModel();

    // Fetch full config details
    m_client->dumpConfig();
}

void RemotesModel::onConfigDumpReceived(const QJsonObject &config)
{
    for (auto it = config.begin(); it != config.end(); ++it) {
        const QString &name = it.key();
        QJsonObject remoteConfig = it.value().toObject();
        QString type = remoteConfig.value(QStringLiteral("type")).toString();

        for (int i = 0; i < m_remotes.size(); i++) {
            if (m_remotes[i].name == name) {
                m_remotes[i].type = type;
                m_remotes[i].config = remoteConfig;

                QModelIndex idx = index(i, 0);
                Q_EMIT dataChanged(idx, idx, { TypeRole, DescriptionRole });
                break;
            }
        }
    }

    Q_EMIT refreshDone();
}
