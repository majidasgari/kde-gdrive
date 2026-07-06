#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

class RcloneApiClient;

class RemotesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        TypeRole,
        DescriptionRole,
    };

    explicit RemotesModel(RcloneApiClient *client, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList remoteNames() const;
    Q_INVOKABLE QString remoteConfigJson(int index) const;
    Q_INVOKABLE QString remoteType(int index) const;
    Q_INVOKABLE int rowForName(const QString &name) const;

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void refreshDone();

private Q_SLOTS:
    void onRemotesListed(const QStringList &remotes);
    void onConfigDumpReceived(const QJsonObject &config);

private:
    struct RemoteEntry {
        QString name;
        QString type;
        QJsonObject config;
    };

    RcloneApiClient *m_client;
    QList<RemoteEntry> m_remotes;
};
