#include "RcloneApiClient.h"

#include <QJsonDocument>
#include <QHostAddress>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QDebug>

RcloneApiClient::RcloneApiClient(QObject *parent)
    : QObject(parent)
    , m_timeout(new QTimer(this))
{
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(10000);
    connect(m_timeout, &QTimer::timeout, this, [this]() {
        if (m_socket) {
            m_socket->abort();
            m_socket->deleteLater();
            m_socket = nullptr;
        }
        if (m_pendingCallback) {
            QJsonObject err;
            err[QStringLiteral("error")] = QStringLiteral("Request timed out");
            m_pendingCallback(-1, err);
            m_pendingCallback = nullptr;
        }
        Q_EMIT requestError(m_pendingEndpoint, QStringLiteral("Request timed out"));
        m_pendingEndpoint.clear();
    });

    m_networkManager = new QNetworkAccessManager(this);
}

void RcloneApiClient::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
}

QString RcloneApiClient::baseUrl() const
{
    return m_baseUrl;
}

void RcloneApiClient::sendRequest(const QString &endpoint, const QJsonObject &body, ResponseCallback callback, int timeoutMs)
{
    m_pendingEndpoint = endpoint;
    m_pendingCallback = callback;
    m_timeout->setInterval(timeoutMs);

    QUrl url(m_baseUrl);
    QString host = url.host();
    quint16 port = url.port(5572);

    if (m_socket) {
        m_socket->abort();
        m_socket->deleteLater();
    }

    QJsonDocument doc(body);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    m_requestBuffer.clear();
    m_requestBuffer.append(QStringLiteral("POST /%1 HTTP/1.1\r\n").arg(endpoint).toUtf8());
    m_requestBuffer.append(QStringLiteral("Host: %1:%2\r\n").arg(host).arg(port).toUtf8());
    m_requestBuffer.append(QStringLiteral("Content-Type: application/json\r\n").toUtf8());
    m_requestBuffer.append(QStringLiteral("Content-Length: %1\r\n").arg(jsonData.size()).toUtf8());
    m_requestBuffer.append(QStringLiteral("Connection: close\r\n").toUtf8());
    m_requestBuffer.append(QStringLiteral("\r\n").toUtf8());
    m_requestBuffer.append(jsonData);

    m_socket = new QTcpSocket(this);
    m_responseBuffer.clear();

    connect(m_socket, &QTcpSocket::connected, this, &RcloneApiClient::handleConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &RcloneApiClient::handleReadyRead, Qt::UniqueConnection);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &RcloneApiClient::handleError);
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        if (!m_responseBuffer.isEmpty())
            handleReadyRead();
    });

    qDebug() << "RcloneApiClient: connecting to" << host << port;
    m_timeout->start();
    m_socket->connectToHost(host, port);
}

void RcloneApiClient::handleConnected()
{
    qDebug() << "RcloneApiClient: connected, sending request";
    m_socket->write(m_requestBuffer);
}

void RcloneApiClient::handleReadyRead()
{
    if (!m_socket)
        return;

    if (m_socket->isOpen())
        m_responseBuffer.append(m_socket->readAll());

    int headerEnd = m_responseBuffer.indexOf("\r\n\r\n");
    if (headerEnd == -1)
        return;

    QByteArray headerPart = m_responseBuffer.left(headerEnd);
    QByteArray bodyPart = m_responseBuffer.mid(headerEnd + 4);

    int statusCode = 0;
    QList<QByteArray> headerLines = headerPart.split('\n');
    if (!headerLines.isEmpty()) {
        QByteArray statusLine = headerLines[0].trimmed();
        QList<QByteArray> parts = statusLine.split(' ');
        if (parts.size() >= 2)
            statusCode = parts[1].toInt();
    }

    int contentLength = -1;
    for (const QByteArray &line : headerLines) {
        QByteArray trimmed = line.trimmed().toLower();
        if (trimmed.startsWith("content-length:")) {
            contentLength = trimmed.mid(15).trimmed().toInt();
            break;
        }
    }

    if (contentLength >= 0 && bodyPart.size() < contentLength)
        return;

    m_timeout->stop();

    QJsonObject responseObj;
    if (statusCode == 200 && !bodyPart.isEmpty()) {
        QJsonParseError parseError;
        QJsonDocument responseDoc = QJsonDocument::fromJson(bodyPart, &parseError);
        if (parseError.error == QJsonParseError::NoError && responseDoc.isObject()) {
            responseObj = responseDoc.object();
            qDebug() << "RcloneApiClient: received response for" << m_pendingEndpoint << responseObj;
        } else {
            qWarning() << "RcloneApiClient: JSON parse error" << parseError.errorString();
        }
    } else {
        qDebug() << "RcloneApiClient: response status" << statusCode << "body:" << bodyPart.left(200);
    }

    QTcpSocket *oldSocket = m_socket;
    m_socket = nullptr;
    m_pendingEndpoint.clear();

    if (m_pendingCallback) {
        auto cb = m_pendingCallback;
        m_pendingCallback = nullptr;
        cb(statusCode, responseObj);
    }

    oldSocket->deleteLater();
}

void RcloneApiClient::handleError(QAbstractSocket::SocketError error)
{
    m_timeout->stop();
    QString errMsg = m_socket ? m_socket->errorString() : QStringLiteral("Unknown error");
    qWarning() << "RcloneApiClient: socket error" << error << errMsg;

    if (m_pendingCallback) {
        QJsonObject err;
        err[QStringLiteral("error")] = errMsg;
        m_pendingCallback(-1, err);
        m_pendingCallback = nullptr;
    }
    Q_EMIT requestError(m_pendingEndpoint, errMsg);
    m_pendingEndpoint.clear();

    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void RcloneApiClient::getVersion()
{
    sendRequest(QStringLiteral("core/version"), {}, [this](int, const QJsonObject &obj) {
        QString version = obj.value(QStringLiteral("version")).toString();
        Q_EMIT versionReceived(version);
    });
}

void RcloneApiClient::listRemotes()
{
    sendRequest(QStringLiteral("config/listremotes"), {}, [this](int, const QJsonObject &obj) {
        QStringList remotes;
        const QJsonArray arr = obj.value(QStringLiteral("remotes")).toArray();
        for (const auto &v : arr)
            remotes.append(v.toString());
        Q_EMIT remotesListed(remotes);
    });
}

void RcloneApiClient::createRemote(const QString &name, const QString &type, const QJsonObject &options)
{
    QJsonObject params;
    params[QStringLiteral("name")] = name;
    params[QStringLiteral("type")] = type;
    params[QStringLiteral("parameters")] = options;

    sendRequest(QStringLiteral("config/create"), params, [this](int, const QJsonObject &obj) {
        if (obj.contains(QStringLiteral("error")))
            Q_EMIT remoteCreated(false, obj[QStringLiteral("error")].toString());
        else
            Q_EMIT remoteCreated(true, {});
    });
}

void RcloneApiClient::deleteRemote(const QString &name)
{
    QJsonObject params;
    params[QStringLiteral("name")] = name;

    sendRequest(QStringLiteral("config/delete"), params, [this](int, const QJsonObject &obj) {
        if (obj.contains(QStringLiteral("error")))
            Q_EMIT remoteDeleted(false, obj[QStringLiteral("error")].toString());
        else
            Q_EMIT remoteDeleted(true, {});
    });
}

void RcloneApiClient::mount(const QString &fs, const QString &mountPoint)
{
    QJsonObject params;
    params[QStringLiteral("fs")] = fs;
    params[QStringLiteral("mountPoint")] = mountPoint;

    sendRequest(QStringLiteral("mount/mount"), params, [this](int, const QJsonObject &obj) {
        if (obj.contains(QStringLiteral("error")))
            Q_EMIT mountDone(false, obj[QStringLiteral("error")].toString());
        else
            Q_EMIT mountDone(true, {});
    }, 60000);
}

void RcloneApiClient::unmount(const QString &mountPoint)
{
    QJsonObject params;
    params[QStringLiteral("mountPoint")] = mountPoint;

    sendRequest(QStringLiteral("mount/unmount"), params, [this](int, const QJsonObject &obj) {
        if (obj.contains(QStringLiteral("error")))
            Q_EMIT unmountDone(false, obj[QStringLiteral("error")].toString());
        else
            Q_EMIT unmountDone(true, {});
    });
}

void RcloneApiClient::listMounts()
{
    sendRequest(QStringLiteral("mount/listmounts"), {}, [this](int, const QJsonObject &obj) {
        QJsonArray mounts = obj.value(QStringLiteral("mounts")).toArray();
        Q_EMIT mountsListed(mounts);
    });
}

void RcloneApiClient::getCoreStats()
{
    sendRequest(QStringLiteral("core/stats"), {}, [this](int, const QJsonObject &obj) {
        Q_EMIT coreStatsReceived(obj);
    });
}

void RcloneApiClient::dumpConfig()
{
    sendRequest(QStringLiteral("config/dump"), {}, [this](int, const QJsonObject &obj) {
        Q_EMIT configDumpReceived(obj);
    });
}

void RcloneApiClient::fetchDriveChangesDirect(const QString &remote, const QString &accessToken, const QString &pageToken)
{
    QUrl url;
    if (pageToken.isEmpty()) {
        url = QUrl(QStringLiteral("https://www.googleapis.com/drive/v3/changes/startPageToken"));
    } else {
        url = QUrl(QStringLiteral("https://www.googleapis.com/drive/v3/changes"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("pageToken"), pageToken);
        query.addQueryItem(QStringLiteral("includeItemsFromAllDrives"), QStringLiteral("true"));
        query.addQueryItem(QStringLiteral("supportsAllDrives"), QStringLiteral("true"));
        query.addQueryItem(QStringLiteral("fields"), QStringLiteral("nextPageToken,newStartPageToken,changes(fileId,removed,file(name,parents,mimeType))"));
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString(QStringLiteral("Bearer ") + accessToken).toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, remote]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "RcloneApiClient: Direct Drive API error:" << reply->errorString();
            QJsonObject errObj;
            errObj[QStringLiteral("error")] = reply->errorString();
            Q_EMIT driveChangesReceived(remote, errObj);
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            Q_EMIT driveChangesReceived(remote, doc.object());
        }
    });
}

struct RcloneApiClient::ResolverContext : public QObject {
    RcloneApiClient *client;
    QString remote;
    QString accessToken;
    QString folderId;
    QString name;
    QString parentId;

    ResolverContext(RcloneApiClient *c, const QString &rem, const QString &token, const QString &fId, const QString &n, const QString &pId)
        : QObject(c), client(c), remote(rem), accessToken(token), folderId(fId), name(n), parentId(pId)
    {
        connect(client, &RcloneApiClient::directoryPathResolved, this, &ResolverContext::onParentResolved);
    }

    void start() {
        client->resolveDirectoryPath(remote, accessToken, parentId);
    }

    void onParentResolved(const QString &rem, const QString &pId, const QString &parentPath) {
        if (rem == remote && pId == parentId) {
            // Parent resolved!
            QString resolvedPath = parentPath.isEmpty() ? name : (parentPath + QLatin1Char('/') + name);
            client->m_resolvedPaths[folderId] = resolvedPath;
            Q_EMIT client->directoryPathResolved(remote, folderId, resolvedPath);
            this->deleteLater();
        }
    }
};

void RcloneApiClient::resolveDirectoryPath(const QString &remote, const QString &accessToken, const QString &folderId)
{
    if (m_resolvedPaths.contains(folderId)) {
        Q_EMIT directoryPathResolved(remote, folderId, m_resolvedPaths.value(folderId));
        return;
    }

    QUrl url(QStringLiteral("https://www.googleapis.com/drive/v3/files/") + folderId);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("fields"), QStringLiteral("name,parents"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString(QStringLiteral("Bearer ") + accessToken).toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, remote, accessToken, folderId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "RcloneApiClient: resolveDirectoryPath error for" << folderId << ":" << reply->errorString();
            Q_EMIT directoryPathResolved(remote, folderId, QString()); // Fail
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            Q_EMIT directoryPathResolved(remote, folderId, QString()); // Fail
            return;
        }

        QJsonObject obj = doc.object();
        QString name = obj.value(QStringLiteral("name")).toString();
        QJsonArray parents = obj.value(QStringLiteral("parents")).toArray();

        if (parents.isEmpty()) {
            // We reached the root (or a shared drive root)
            m_resolvedPaths[folderId] = QString();
            Q_EMIT directoryPathResolved(remote, folderId, QString());
            return;
        }

        QString parentId = parents.at(0).toString();

        if (m_resolvedPaths.contains(parentId)) {
            QString parentPath = m_resolvedPaths.value(parentId);
            QString resolvedPath = parentPath.isEmpty() ? name : (parentPath + QLatin1Char('/') + name);
            m_resolvedPaths[folderId] = resolvedPath;
            Q_EMIT directoryPathResolved(remote, folderId, resolvedPath);
            return;
        }

        auto *context = new ResolverContext(this, remote, accessToken, folderId, name, parentId);
        context->start();
    });
}
