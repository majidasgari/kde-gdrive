#include "BisyncManager.h"
#include "config/Settings.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>
#include <QDebug>

static QString ansiToHtml(const QString &input)
{
    static const QRegularExpression ansiRe(QStringLiteral("\\x1b\\[([0-9;]*)m"));
    QString output;
    int lastPos = 0;
    bool inSpan = false;

    auto matchIt = ansiRe.globalMatch(input);
    while (matchIt.hasNext()) {
        auto match = matchIt.next();
        output += input.mid(lastPos, match.capturedStart() - lastPos).toHtmlEscaped();
        lastPos = match.capturedEnd();

        QString codes = match.captured(1);
        QStringList codeList = codes.split(QLatin1Char(';'));

        if (inSpan) {
            output += QStringLiteral("</span>");
            inSpan = false;
        }

        for (const QString &code : codeList) {
            int n = code.toInt();
            switch (n) {
            case 0: break;
            case 1: output += QStringLiteral("<span style='font-weight:bold'>"); inSpan = true; break;
            case 2: output += QStringLiteral("<span style='opacity:0.7'>"); inSpan = true; break;
            case 4: output += QStringLiteral("<span style='text-decoration:underline'>"); inSpan = true; break;
            // KDE Breeze terminal colors (normal)
            case 30: output += QStringLiteral("<span style='color:#232629'>"); inSpan = true; break;
            case 31: output += QStringLiteral("<span style='color:#da4453'>"); inSpan = true; break;
            case 32: output += QStringLiteral("<span style='color:#18b242'>"); inSpan = true; break;
            case 33: output += QStringLiteral("<span style='color:#f67400'>"); inSpan = true; break;
            case 34: output += QStringLiteral("<span style='color:#2980b9'>"); inSpan = true; break;
            case 35: output += QStringLiteral("<span style='color:#8e44ad'>"); inSpan = true; break;
            case 36: output += QStringLiteral("<span style='color:#1abc9c'>"); inSpan = true; break;
            case 37: output += QStringLiteral("<span style='color:#fcfcfc'>"); inSpan = true; break;
            // KDE Breeze terminal colors (bright)
            case 90: output += QStringLiteral("<span style='color:#7f8c8d'>"); inSpan = true; break;
            case 91: output += QStringLiteral("<span style='color:#ff6b6b'>"); inSpan = true; break;
            case 92: output += QStringLiteral("<span style='color:#51cf66'>"); inSpan = true; break;
            case 93: output += QStringLiteral("<span style='color:#ffd43b'>"); inSpan = true; break;
            case 94: output += QStringLiteral("<span style='color:#74c0fc'>"); inSpan = true; break;
            case 95: output += QStringLiteral("<span style='color:#da77f2'>"); inSpan = true; break;
            case 96: output += QStringLiteral("<span style='color:#66d9e8'>"); inSpan = true; break;
            case 97: output += QStringLiteral("<span style='color:#f8f9fa'>"); inSpan = true; break;
            default: break;
            }
        }
    }

    output += input.mid(lastPos).toHtmlEscaped();
    if (inSpan)
        output += QStringLiteral("</span>");

    // Convert newlines to HTML breaks
    output.replace(QLatin1Char('\n'), QStringLiteral("<br>"));

    return output;
}

BisyncManager::BisyncManager(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_scheduleTimer(new QTimer(this))
    , m_settings(settings)
    , m_resyncMode(QStringLiteral("older"))
    , m_scheduleHours({10, 18, 23})
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &BisyncManager::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &BisyncManager::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BisyncManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &BisyncManager::onProcessError);

    connect(m_scheduleTimer, &QTimer::timeout, this, &BisyncManager::checkSchedule);
}

bool BisyncManager::isRunning() const { return m_running; }
QString BisyncManager::lastSyncTime() const { return m_lastSyncTimeStr; }
QString BisyncManager::lastSyncStatus() const { return m_lastSyncStatusStr; }
QString BisyncManager::nextSyncTime() const { return m_nextSyncTimeStr; }
QString BisyncManager::currentOutput() const { return m_currentOutput; }
QString BisyncManager::currentOutputRich() const { return ansiToHtml(m_currentOutput); }
QString BisyncManager::currentRemote() const { return m_currentRemote; }

void BisyncManager::setResyncMode(const QString &mode) { m_resyncMode = mode; }
QString BisyncManager::resyncMode() const { return m_resyncMode; }

void BisyncManager::setScheduleTimes(const QList<int> &hours)
{
    m_scheduleHours = hours;
    std::sort(m_scheduleHours.begin(), m_scheduleHours.end());
    Q_EMIT scheduleChanged();
}

QList<int> BisyncManager::scheduleTimes() const { return m_scheduleHours; }

void BisyncManager::setForceFlag(bool force) { m_forceFlag = force; }
bool BisyncManager::forceFlag() const { return m_forceFlag; }

void BisyncManager::addSyncPair(const QString &remote, const QString &localPath)
{
    for (int i = 0; i < m_configuredPairs.size(); i++) {
        if (m_configuredPairs[i].remote == remote) {
            m_configuredPairs[i].localPath = localPath;
            return;
        }
    }
    m_configuredPairs.append({remote, localPath});
}

void BisyncManager::removeSyncPair(const QString &remote)
{
    m_configuredPairs.erase(
        std::remove_if(m_configuredPairs.begin(), m_configuredPairs.end(),
                        [&](const SyncPair &p) { return p.remote == remote; }),
        m_configuredPairs.end());
}

QList<SyncPair> BisyncManager::syncPairs() const { return m_configuredPairs; }

QString BisyncManager::localPathForRemote(const QString &remote) const
{
    for (const auto &p : m_configuredPairs) {
        if (p.remote == remote)
            return p.localPath;
    }
    return {};
}

void BisyncManager::populatePairs(const QStringList &remoteNames)
{
    m_configuredPairs.clear();
    for (const QString &name : remoteNames) {
        QString path = m_settings->syncPathForRemote(name);
        if (!path.isEmpty())
            m_configuredPairs.append({name, path});
    }
}

bool BisyncManager::isLockDetected() const
{
    return !m_lockInfo.isEmpty();
}

QString BisyncManager::lockInfo() const
{
    return m_lockInfo;
}

void BisyncManager::startSync(const QString &remote, const QString &localPath)
{
    if (m_running) {
        m_syncQueue.append({remote, localPath});
        return;
    }

    if (remote.isEmpty() || localPath.isEmpty())
        return;

    // Create local directory if it doesn't exist
    QDir dir(localPath);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QStringList args;
    args << QStringLiteral("bisync")
         << remote + QStringLiteral(":")
         << localPath
         << QStringLiteral("--resync-mode") << m_resyncMode
         << QStringLiteral("--create-empty-src-dirs")
         << QStringLiteral("--compare") << QStringLiteral("size,modtime")
         << QStringLiteral("--ignore-listing-checksum")
         << QStringLiteral("--verbose");

    if (m_forceFlag)
        args << QStringLiteral("--force");

    m_currentRemote = remote;
    m_currentOutput.clear();
    m_running = true;
    Q_EMIT runningChanged();
    Q_EMIT syncStarted(remote);

    m_process->start(QStringLiteral("rclone"), args);
}

void BisyncManager::startSyncWithConfig(const QString &remote, const QString &localPath, const QString &configPath)
{
    if (m_running) {
        m_syncQueue.append({remote, localPath});
        return;
    }

    if (remote.isEmpty() || localPath.isEmpty())
        return;

    // Create local directory if it doesn't exist
    QDir dir(localPath);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QStringList args;
    args << QStringLiteral("bisync")
         << remote + QStringLiteral(":")
         << localPath
         << QStringLiteral("--config") << configPath
         << QStringLiteral("--resync-mode") << m_resyncMode
         << QStringLiteral("--create-empty-src-dirs")
         << QStringLiteral("--compare") << QStringLiteral("size,modtime")
         << QStringLiteral("--ignore-listing-checksum")
         << QStringLiteral("--verbose");

    if (m_forceFlag)
        args << QStringLiteral("--force");

    m_currentRemote = remote;
    m_currentOutput.clear();
    m_running = true;
    Q_EMIT runningChanged();
    Q_EMIT syncStarted(remote);

    m_process->start(QStringLiteral("rclone"), args);
}

void BisyncManager::startAllSyncs()
{
    m_syncQueue = m_configuredPairs;
    runNextQueued();
}

void BisyncManager::cancel()
{
    if (!m_running)
        return;

    m_process->kill();
    m_process->waitForFinished(3000);
    m_running = false;
    m_syncQueue.clear();
    Q_EMIT runningChanged();
    updateLastSync(QStringLiteral("Cancelled"));
}

void BisyncManager::clearLockAndRetry()
{
    m_lockInfo.clear();

    // Find and delete lock files for current remote
    QString cacheDir = QDir::homePath() + QStringLiteral("/.cache/rclone/bisync");
    QDir dir(cacheDir);
    if (dir.exists()) {
        QStringList filters;
        filters << QStringLiteral("*.lck");
        QFileInfoList lockFiles = dir.entryInfoList(filters, QDir::Files);
        for (const QFileInfo &lf : lockFiles) {
            QFile::remove(lf.absoluteFilePath());
            qDebug() << "Deleted stale lock file:" << lf.absoluteFilePath();
        }
    }

    // Retry current sync
    if (!m_currentRemote.isEmpty() && !m_syncQueue.isEmpty()) {
        SyncPair next = m_syncQueue.takeFirst();
        startSync(next.remote, next.localPath);
    }
}

void BisyncManager::clearAllLocks()
{
    m_lockInfo.clear();
    Q_EMIT lockDetected();

    QString cacheDir = QDir::homePath() + QStringLiteral("/.cache/rclone/bisync");
    QDir dir(cacheDir);
    if (dir.exists()) {
        QStringList filters;
        filters << QStringLiteral("*.lck");
        QFileInfoList lockFiles = dir.entryInfoList(filters, QDir::Files);
        int count = 0;
        for (const QFileInfo &lf : lockFiles) {
            QFile::remove(lf.absoluteFilePath());
            count++;
        }
        if (count > 0) {
            m_currentOutput += QStringLiteral("Cleared %1 lock file(s)\n").arg(count);
            Q_EMIT outputReceived(QStringLiteral("Cleared %1 lock file(s)").arg(count));
        }
    }
}

void BisyncManager::startScheduler()
{
    m_scheduleTimer->start(60000);
    checkSchedule();
    Q_EMIT scheduleChanged();
}

void BisyncManager::stopScheduler()
{
    m_scheduleTimer->stop();
}

void BisyncManager::runNextQueued()
{
    if (m_syncQueue.isEmpty())
        return;

    SyncPair next = m_syncQueue.takeFirst();
    startSync(next.remote, next.localPath);
}

void BisyncManager::onReadyReadStdout()
{
    while (m_process->canReadLine()) {
        QByteArray line = m_process->readLine().trimmed();
        if (!line.isEmpty()) {
            QString text = QString::fromUtf8(line);
            m_currentOutput += text + QStringLiteral("\n");
            Q_EMIT outputReceived(text);

            // Detect lock file
            if (text.contains(QStringLiteral("prior lock file found")) ||
                text.contains(QStringLiteral("Valid lock file found"))) {
                m_lockInfo = text;
                Q_EMIT lockDetected();
            }
        }
    }
}

void BisyncManager::onReadyReadStderr()
{
    QByteArray data = m_process->readAllStandardError();
    if (!data.isEmpty()) {
        QString text = QString::fromUtf8(data.trimmed());
        m_currentOutput += text + QStringLiteral("\n");
        Q_EMIT outputReceived(text);
    }
}

void BisyncManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_running = false;
    Q_EMIT runningChanged();

    onReadyReadStdout();
    onReadyReadStderr();

    if (status == QProcess::CrashExit) {
        updateLastSync(QStringLiteral("Crashed"));
        Q_EMIT syncFailed(QStringLiteral("Bisync process crashed"));
        return;
    }

    if (exitCode == 0) {
        updateLastSync(QStringLiteral("Success: %1").arg(m_currentRemote));
        Q_EMIT syncCompleted(true, QStringLiteral("Sync of %1 completed successfully").arg(m_currentRemote));
    } else {
        QString error = QString::fromUtf8(m_process->readAllStandardError());
        updateLastSync(QStringLiteral("Failed: %1").arg(m_currentRemote));
        Q_EMIT syncFailed(error.isEmpty()
            ? QStringLiteral("Sync of %1 failed (code %2)").arg(m_currentRemote).arg(exitCode)
            : error);
    }

    m_currentRemote.clear();

    // Run next queued sync
    if (!m_syncQueue.isEmpty())
        QTimer::singleShot(2000, this, &BisyncManager::runNextQueued);
}

void BisyncManager::onProcessError(QProcess::ProcessError error)
{
    m_running = false;
    Q_EMIT runningChanged();
    updateLastSync(QStringLiteral("Error: %1").arg(m_currentRemote));
    Q_EMIT syncFailed(QStringLiteral("Failed to start rclone bisync: %1").arg(m_process->errorString()));
    m_currentRemote.clear();
}

void BisyncManager::checkSchedule()
{
    if (m_running || m_scheduleHours.isEmpty())
        return;

    QTime now = QTime::currentTime();
    int currentMinute = now.hour() * 60 + now.minute();

    bool triggered = false;
    for (int hour : m_scheduleHours) {
        if (currentMinute == hour * 60) {
            triggered = true;
            break;
        }
    }

    if (triggered) {
        QTimer::singleShot(5000, this, &BisyncManager::startAllSyncs);
    }

    QTime next = nextScheduledTime();
    m_nextSyncTimeStr = next.toString(QStringLiteral("HH:mm"));
    Q_EMIT scheduleChanged();
}

void BisyncManager::updateLastSync(const QString &status)
{
    m_lastSyncTimeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    m_lastSyncStatusStr = status;
    Q_EMIT lastSyncChanged();
}

QTime BisyncManager::nextScheduledTime() const
{
    if (m_scheduleHours.isEmpty())
        return {};

    QTime now = QTime::currentTime();
    int currentMinutes = now.hour() * 60 + now.minute();

    for (int hour : m_scheduleHours) {
        if (hour * 60 > currentMinutes)
            return QTime(hour, 0);
    }

    return QTime(m_scheduleHours.first(), 0);
}
