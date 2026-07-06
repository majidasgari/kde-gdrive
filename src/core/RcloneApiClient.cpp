#include "RcloneApiClient.h"

#include <QJsonDocument>
#include <QHostAddress>
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

void RcloneApiClient::getDriveChanges(const QString &remote, const QString &pageToken)
{
    QJsonObject params;
    params[QStringLiteral("command")] = QStringLiteral("changes");
    params[QStringLiteral("fs")] = QString(remote + QStringLiteral(":"));

    QJsonArray argsArray;
    params[QStringLiteral("arg")] = argsArray;

    QJsonObject opts;
    if (!pageToken.isEmpty()) {
        opts[QStringLiteral("pageToken")] = pageToken;
    }
    params[QStringLiteral("opt")] = opts;

    sendRequest(QStringLiteral("backend/command"), params, [this, remote](int, const QJsonObject &obj) {
        Q_EMIT driveChangesReceived(remote, obj);
    });
}
