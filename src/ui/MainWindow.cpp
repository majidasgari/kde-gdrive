#include "MainWindow.h"
#include "core/RcloneDaemon.h"
#include "core/RcloneApiClient.h"
#include "core/MountManager.h"
#include "models/RemotesModel.h"
#include "models/MountsModel.h"
#include "config/Settings.h"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>

MainWindow::MainWindow(Settings *settings, RcloneDaemon *daemon, RcloneApiClient *apiClient,
                       MountManager *mountManager, RemotesModel *remotesModel,
                       MountsModel *mountsModel, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_daemon(daemon)
    , m_apiClient(apiClient)
    , m_mountManager(mountManager)
    , m_remotesModel(remotesModel)
    , m_mountsModel(mountsModel)
{
    connect(m_daemon, &RcloneDaemon::started, this, &MainWindow::onDaemonStarted);
    connect(m_daemon, &RcloneDaemon::stopped, this, &MainWindow::onDaemonStopped);
    connect(m_apiClient, &RcloneApiClient::versionReceived, this, &MainWindow::onVersionReceived);
}

QString MainWindow::rcloneVersion() const
{
    return m_rcloneVersion;
}

bool MainWindow::isDaemonRunning() const
{
    return m_daemon->isRunning();
}

RemotesModel *MainWindow::remotesModel() const
{
    return m_remotesModel;
}

MountsModel *MainWindow::mountsModel() const
{
    return m_mountsModel;
}

void MainWindow::mountRemote(const QString &remoteName)
{
    m_mountManager->mountRemote(remoteName);
}

void MainWindow::unmountRemote(const QString &remoteName)
{
    m_mountManager->unmountRemote(remoteName);
}

bool MainWindow::isRemoteMounted(const QString &remoteName) const
{
    return m_mountManager->isMounted(remoteName);
}

void MainWindow::openSettings()
{
    Q_EMIT settingsRequested();
}

void MainWindow::refreshRemotes()
{
    m_remotesModel->refresh();
}

void MainWindow::refreshMounts()
{
    m_mountsModel->refresh();
}

void MainWindow::openMountPoint(const QString &remoteName)
{
    QString mp = m_mountManager->mountPointForRemote(remoteName);
    if (!mp.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(mp));
}

void MainWindow::onDaemonStarted()
{
    m_apiClient->setBaseUrl(m_daemon->baseUrl());
    Q_EMIT daemonRunningChanged();
}

void MainWindow::onDaemonStopped()
{
    m_rcloneVersion.clear();
    Q_EMIT rcloneVersionChanged();
    Q_EMIT daemonRunningChanged();
}

void MainWindow::onVersionReceived(const QString &version)
{
    m_rcloneVersion = version;
    Q_EMIT rcloneVersionChanged();
}

void MainWindow::copyToClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}
