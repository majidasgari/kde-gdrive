#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QStringList>
#include <QMap>
#include <QPair>

class Settings;
class LocalWatcher;
class RcloneApiClient;

struct SyncPair {
    QString remote;
    QString localPath;
};

class BisyncManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY lastSyncChanged)
    Q_PROPERTY(QString lastSyncStatus READ lastSyncStatus NOTIFY lastSyncChanged)
    Q_PROPERTY(QString nextSyncTime READ nextSyncTime NOTIFY scheduleChanged)
    Q_PROPERTY(QString currentOutput READ currentOutput NOTIFY outputReceived)
    Q_PROPERTY(QString currentOutputRich READ currentOutputRich NOTIFY outputReceived)
    Q_PROPERTY(QString currentRemote READ currentRemote NOTIFY syncStarted)
    Q_PROPERTY(bool lockDetected READ isLockDetected NOTIFY lockDetected)
    Q_PROPERTY(QString lockInfo READ lockInfo NOTIFY lockDetected)

public:
    explicit BisyncManager(Settings *settings, QObject *parent = nullptr);

    bool isRunning() const;
    QString lastSyncTime() const;
    QString lastSyncStatus() const;
    QString nextSyncTime() const;
    QString currentOutput() const;
    QString currentOutputRich() const;
    QString currentRemote() const;
    bool isLockDetected() const;
    QString lockInfo() const;

    void setResyncMode(const QString &mode);
    QString resyncMode() const;

    void setScheduleTimes(const QList<int> &hours);
    QList<int> scheduleTimes() const;

    void setForceFlag(bool force);
    bool forceFlag() const;

    // Per-remote sync paths
    void addSyncPair(const QString &remote, const QString &localPath);
    void removeSyncPair(const QString &remote);
    QList<SyncPair> syncPairs() const;
    QString localPathForRemote(const QString &remote) const;
    Q_INVOKABLE void populatePairs(const QStringList &remoteNames);

public Q_SLOTS:
    void startSync(const QString &remote, const QString &localPath);
    void startSyncWithConfig(const QString &remote, const QString &localPath, const QString &configPath);
    void runDedupe(const QString &remote);
    void startAllSyncs();
    void cancel();
    void clearLockAndRetry();
    void clearAllLocks();
    void startScheduler();
    void stopScheduler();

Q_SIGNALS:
    void runningChanged();
    void lastSyncChanged();
    void scheduleChanged();
    void syncStarted(const QString &remote);
    void syncCompleted(bool success, const QString &summary);
    void syncFailed(const QString &error);
    void outputReceived(const QString &line);
    void lockDetected();

private Q_SLOTS:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void checkSchedule();
    void onLocalFolderChanged(const QString &rootPath, const QString &relativePath);
    void onLocalDebounceTimeout();
    void onRemotePollTimeout();
    void onDriveChangesReceived(const QString &remote, const QJsonObject &response);

private:
    void updateLastSync(const QString &status);
    QTime nextScheduledTime() const;
    void runNextQueued();
    bool isSyncQueuedOrRunning(const QString &remoteFs) const;

    QProcess *m_process;
    QTimer *m_scheduleTimer;
    Settings *m_settings;
    QString m_lastSyncTimeStr;
    QString m_lastSyncStatusStr;
    QString m_nextSyncTimeStr;
    QString m_currentOutput;
    QString m_currentRemote;
    QString m_resyncMode;
    QList<int> m_scheduleHours;
    bool m_forceFlag = true;
    bool m_running = false;
    QString m_lockInfo;

    // Sync queue for sequential execution
    QList<SyncPair> m_syncQueue;
    QList<SyncPair> m_configuredPairs;

    enum SyncStep {
        StepNone,
        StepDedupe,
        StepBisync
    };
    SyncStep m_currentStep = StepNone;
    QString m_pendingLocalPath;
    QString m_pendingConfigPath;
    QMap<QString, bool> m_duplicateDetected;

    LocalWatcher *m_localWatcher = nullptr;
    RcloneApiClient *m_apiClient = nullptr;
    QTimer *m_localDebounceTimer = nullptr;
    QTimer *m_remotePollTimer = nullptr;
    QMap<QString, QStringList> m_pendingLocalChanges;
    QMap<QString, QStringList> m_pendingRemoteChanges;
};
