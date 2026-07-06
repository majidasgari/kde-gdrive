#include "RcloneDaemon.h"

#include <QDebug>

RcloneDaemon::RcloneDaemon(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::started, this, &RcloneDaemon::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RcloneDaemon::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &RcloneDaemon::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &RcloneDaemon::onReadyReadStdout);
}

RcloneDaemon::~RcloneDaemon()
{
    stop();
}

void RcloneDaemon::setRclonePath(const QString &path)
{
    m_rclonePath = path;
}

QString RcloneDaemon::rclonePath() const
{
    return m_rclonePath;
}

void RcloneDaemon::setPort(int port)
{
    m_port = static_cast<quint16>(port);
}

int RcloneDaemon::port() const
{
    return m_port;
}

void RcloneDaemon::setRcUser(const QString &user)
{
    m_rcUser = user;
}

QString RcloneDaemon::rcUser() const
{
    return m_rcUser;
}

void RcloneDaemon::setRcPass(const QString &pass)
{
    m_rcPass = pass;
}

QString RcloneDaemon::rcPass() const
{
    return m_rcPass;
}

QString RcloneDaemon::baseUrl() const
{
    return QStringLiteral("http://127.0.0.1:%1/").arg(m_port);
}

bool RcloneDaemon::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void RcloneDaemon::start()
{
    if (m_rclonePath.isEmpty()) {
        Q_EMIT failed(QStringLiteral("rclone binary path is empty"));
        return;
    }

    if (isRunning()) {
        qDebug() << "rclone daemon is already running";
        return;
    }

    m_intendedStart = true;
    QStringList args = buildArguments();
    m_process->start(m_rclonePath, args);
}

void RcloneDaemon::stop()
{
    m_intendedStart = false;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(5000)) {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
    }
}

void RcloneDaemon::restart()
{
    stop();
    start();
}

void RcloneDaemon::onProcessStarted()
{
    qDebug() << "rclone daemon started on" << baseUrl();
    Q_EMIT started();
}

void RcloneDaemon::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    bool wasIntended = m_intendedStart;
    m_intendedStart = false;

    if (wasIntended && status == QProcess::CrashExit) {
        Q_EMIT failed(QStringLiteral("rclone daemon crashed"));
    }

    Q_EMIT stopped();
}

void RcloneDaemon::onProcessError(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
    case QProcess::FailedToStart:
        msg = QStringLiteral("Failed to start rclone: %1").arg(m_rclonePath);
        break;
    case QProcess::Crashed:
        msg = QStringLiteral("rclone process crashed");
        break;
    case QProcess::Timedout:
        msg = QStringLiteral("rclone process timed out");
        break;
    case QProcess::WriteError:
        msg = QStringLiteral("Write error communicating with rclone");
        break;
    case QProcess::ReadError:
        msg = QStringLiteral("Read error communicating with rclone");
        break;
    default:
        msg = QStringLiteral("Unknown rclone process error");
        break;
    }
    qWarning() << msg;
    Q_EMIT failed(msg);
}

void RcloneDaemon::onReadyReadStdout()
{
    while (m_process->canReadLine()) {
        QByteArray line = m_process->readLine().trimmed();
        if (!line.isEmpty())
            Q_EMIT outputReceived(QString::fromUtf8(line));
    }
}

QStringList RcloneDaemon::buildArguments() const
{
    QStringList args;
    args << QStringLiteral("rcd")
         << QStringLiteral("--rc-addr") << QStringLiteral("127.0.0.1:%1").arg(m_port)
         << QStringLiteral("--rc-no-auth");

    if (!m_rcUser.isEmpty())
        args << QStringLiteral("--rc-user") << m_rcUser;
    if (!m_rcPass.isEmpty())
        args << QStringLiteral("--rc-pass") << m_rcPass;

    return args;
}
