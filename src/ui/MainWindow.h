#pragma once

#include <QObject>
#include <QUrl>

class RcloneDaemon;
class RcloneApiClient;
class MountManager;
class RemotesModel;
class MountsModel;
class Settings;

class MainWindow : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString rcloneVersion READ rcloneVersion NOTIFY rcloneVersionChanged)
    Q_PROPERTY(bool daemonRunning READ isDaemonRunning NOTIFY daemonRunningChanged)
    Q_PROPERTY(RemotesModel *remotesModel READ remotesModel CONSTANT)
    Q_PROPERTY(MountsModel *mountsModel READ mountsModel CONSTANT)

public:
    explicit MainWindow(Settings *settings, RcloneDaemon *daemon, RcloneApiClient *apiClient,
                        MountManager *mountManager, RemotesModel *remotesModel,
                        MountsModel *mountsModel, QObject *parent = nullptr);

    QString rcloneVersion() const;
    bool isDaemonRunning() const;
    RemotesModel *remotesModel() const;
    MountsModel *mountsModel() const;

    Q_INVOKABLE void mountRemote(const QString &remoteName);
    Q_INVOKABLE void unmountRemote(const QString &remoteName);
    Q_INVOKABLE bool isRemoteMounted(const QString &remoteName) const;
    Q_INVOKABLE void refreshRemotes();
    Q_INVOKABLE void refreshMounts();
    Q_INVOKABLE void openMountPoint(const QString &remoteName);
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void copyToClipboard(const QString &text);

Q_SIGNALS:
    void rcloneVersionChanged();
    void daemonRunningChanged();
    void settingsRequested();

private Q_SLOTS:
    void onDaemonStarted();
    void onDaemonStopped();
    void onVersionReceived(const QString &version);

private:
    Settings *m_settings;
    RcloneDaemon *m_daemon;
    RcloneApiClient *m_apiClient;
    MountManager *m_mountManager;
    RemotesModel *m_remotesModel;
    MountsModel *m_mountsModel;

    QString m_rcloneVersion;
};
