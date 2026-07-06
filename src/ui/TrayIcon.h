#pragma once

#include <QObject>
#include <KStatusNotifierItem>

class MountManager;
class RemotesModel;

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(MountManager *mountManager, RemotesModel *remotesModel, QObject *parent = nullptr);

    KStatusNotifierItem *notifierItem() const;

public Q_SLOTS:
    void updateMenu();
    void showMessage(const QString &title, const QString &message, int timeout = 5000);

Q_SIGNALS:
    void openMainWindowRequested();
    void openSettingsRequested();
    void quitRequested();

private Q_SLOTS:
    void onActivateRequested(bool active);
    void onSecondaryActivateRequested(const QPoint &pos);

private:
    void buildContextMenu();

    KStatusNotifierItem *m_tray;
    MountManager *m_mountManager;
    RemotesModel *m_remotesModel;
};
