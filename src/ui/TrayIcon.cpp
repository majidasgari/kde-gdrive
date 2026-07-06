#include "TrayIcon.h"
#include "core/MountManager.h"
#include "models/RemotesModel.h"

#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QDebug>

TrayIcon::TrayIcon(MountManager *mountManager, RemotesModel *remotesModel, QObject *parent)
    : QObject(parent)
    , m_tray(new KStatusNotifierItem(this))
    , m_mountManager(mountManager)
    , m_remotesModel(remotesModel)
{
    m_tray->setTitle(QStringLiteral("Nimbus Google Drive Client"));
    m_tray->setIconByPixmap(QIcon(QStringLiteral(":/logo.png")));
    m_tray->setCategory(KStatusNotifierItem::ApplicationStatus);
    m_tray->setStatus(KStatusNotifierItem::Active);
    m_tray->setStandardActionsEnabled(false);

    connect(m_tray, &KStatusNotifierItem::activateRequested, this, &TrayIcon::onActivateRequested);
    connect(m_tray, &KStatusNotifierItem::secondaryActivateRequested, this, &TrayIcon::onSecondaryActivateRequested);

    connect(m_mountManager, &MountManager::mountStateChanged, this, &TrayIcon::updateMenu);
    connect(m_remotesModel, &RemotesModel::refreshDone, this, &TrayIcon::updateMenu);

    buildContextMenu();
}

KStatusNotifierItem *TrayIcon::notifierItem() const
{
    return m_tray;
}

void TrayIcon::updateMenu()
{
    buildContextMenu();
}

void TrayIcon::showMessage(const QString &title, const QString &message, int timeout)
{
    m_tray->showMessage(title, message, QString(), timeout);
}

void TrayIcon::onActivateRequested(bool active)
{
    Q_UNUSED(active)
    Q_EMIT openMainWindowRequested();
}

void TrayIcon::onSecondaryActivateRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    m_tray->contextMenu()->popup(pos);
}

void TrayIcon::buildContextMenu()
{
    QMenu *menu = new QMenu();

    QAction *openAction = menu->addAction(QIcon::fromTheme(QStringLiteral("window-new")), QStringLiteral("Show Window"));
    connect(openAction, &QAction::triggered, this, &TrayIcon::openMainWindowRequested);

    menu->addSeparator();

    QStringList remotes = m_remotesModel->remoteNames();
    if (remotes.isEmpty()) {
        QAction *noRemotes = menu->addAction(QStringLiteral("No remotes"));
        noRemotes->setEnabled(false);
    } else {
        QMenu *mountMenu = menu->addMenu(QIcon::fromTheme(QStringLiteral("drive-harddisk")), QStringLiteral("Mount / Unmount"));
        for (const QString &remote : remotes) {
            bool mounted = m_mountManager->isMounted(remote);
            QAction *action = mountMenu->addAction(
                QIcon::fromTheme(mounted ? QStringLiteral("media-eject") : QStringLiteral("drive-harddisk")),
                mounted ? QStringLiteral("Unmount %1").arg(remote)
                        : QStringLiteral("Mount %1").arg(remote));
            connect(action, &QAction::triggered, this, [this, remote]() {
                m_mountManager->toggleMount(remote);
            });
        }
    }

    menu->addSeparator();

    QAction *settingsAction = menu->addAction(QIcon::fromTheme(QStringLiteral("preferences-system")), QStringLiteral("Settings"));
    connect(settingsAction, &QAction::triggered, this, &TrayIcon::openSettingsRequested);

    menu->addSeparator();

    QAction *quitAction = menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), QStringLiteral("Quit"));
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);

    m_tray->setContextMenu(menu);
}
