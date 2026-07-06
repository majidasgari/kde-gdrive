#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QTimer>
#include <QNetworkAccessManager>
#include <functional>

class RcloneApiClient : public QObject
{
    Q_OBJECT

public:
    explicit RcloneApiClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);
    QString baseUrl() const;

public Q_SLOTS:
    void getVersion();
    void listRemotes();
    void createRemote(const QString &name, const QString &type, const QJsonObject &options);
    void deleteRemote(const QString &name);
    void mount(const QString &fs, const QString &mountPoint);
    void unmount(const QString &mountPoint);
    void listMounts();
    void getCoreStats();
    void dumpConfig();
    void fetchDriveChangesDirect(const QString &remote, const QString &accessToken, const QString &pageToken);

Q_SIGNALS:
    void versionReceived(const QString &version);
    void remotesListed(const QStringList &remotes);
    void remoteCreated(bool success, const QString &error);
    void remoteDeleted(bool success, const QString &error);
    void mountDone(bool success, const QString &error);
    void unmountDone(bool success, const QString &error);
    void mountsListed(const QJsonArray &mounts);
    void coreStatsReceived(const QJsonObject &stats);
    void configDumpReceived(const QJsonObject &config);
    void driveChangesReceived(const QString &remote, const QJsonObject &response);
    void requestError(const QString &endpoint, const QString &error);

private:
    using ResponseCallback = std::function<void(int statusCode, const QJsonObject &response)>;

    void sendRequest(const QString &endpoint, const QJsonObject &body, ResponseCallback callback, int timeoutMs = 15000);
    void handleConnected();
    void handleReadyRead();
    void handleError(QAbstractSocket::SocketError error);

    QTcpSocket *m_socket = nullptr;
    QByteArray m_requestBuffer;
    QByteArray m_responseBuffer;
    QString m_baseUrl;
    QString m_pendingEndpoint;
    ResponseCallback m_pendingCallback;
    QTimer *m_timeout;
    QNetworkAccessManager *m_networkManager = nullptr;
};
