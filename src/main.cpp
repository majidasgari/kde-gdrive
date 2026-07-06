#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QTimer>
#include <QWindow>
#include <memory>
#include <QDebug>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>

#include "nimbus-gdrive-version.h"

#include "core/RcloneLocator.h"
#include "core/RcloneDaemon.h"
#include "core/RcloneApiClient.h"
#include "core/MountManager.h"
#include "sync/BisyncManager.h"
#include "models/RemotesModel.h"
#include "models/MountsModel.h"
#include "config/Settings.h"
#include "ui/TrayIcon.h"
#include "ui/MainWindow.h"
#include "ui/OAuthWizard.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/logo.png")));
    app.setApplicationName(QStringLiteral("nimbus-gdrive"));
    app.setApplicationDisplayName(QStringLiteral("Nimbus Google Drive Client"));
    app.setOrganizationDomain(QStringLiteral("nimbus.org"));
    app.setApplicationVersion(QStringLiteral(NIMBUS_GDRIVE_VERSION_STRING));

    KLocalizedString::setApplicationDomain(QStringLiteral("nimbus-gdrive").toUtf8().constData());

    KAboutData aboutData(
        QStringLiteral("nimbus-gdrive"),
        QStringLiteral("Nimbus Google Drive Client"),
        QStringLiteral(NIMBUS_GDRIVE_VERSION_STRING),
        QStringLiteral("A stable, fast rclone-based Google Drive client"),
        KAboutLicense::GPL_V3,
        QStringLiteral("© 2026 Nimbus Contributors")
    );
    aboutData.addAuthor(QStringLiteral("Nimbus Contributors"));
    KAboutData::setApplicationData(aboutData);

    KDBusService dbusService(KDBusService::Unique);

    // --- Core objects ---
    auto *settings = new Settings(&app);

    QString rclonePath = settings->rclonePath();
    if (rclonePath.isEmpty()) {
        rclonePath = RcloneLocator::findRcloneBinary();
        if (!rclonePath.isEmpty())
            settings->setRclonePath(rclonePath);
    }

    auto *daemon = new RcloneDaemon(&app);
    if (!rclonePath.isEmpty()) {
        daemon->setRclonePath(rclonePath);
        daemon->setPort(settings->rclonePort());
        daemon->setRcUser(settings->rcUser());
        daemon->setRcPass(settings->rcPass());
    }

    auto *apiClient = new RcloneApiClient(&app);
    auto *mountManager = new MountManager(apiClient, settings, &app);

    auto *remotesModel = new RemotesModel(apiClient, &app);
    auto *mountsModel = new MountsModel(apiClient, &app);

    // --- Bisync ---
    auto *bisync = new BisyncManager(settings, &app);
    bisync->setResyncMode(settings->syncResyncMode());
    bisync->setForceFlag(true);

    // Parse schedule times
    QStringList parts = settings->syncTimes().split(QLatin1Char(','), Qt::SkipEmptyParts);
    QList<int> scheduleHours;
    for (const QString &part : parts) {
        bool ok;
        int hour = part.trimmed().toInt(&ok);
        if (ok && hour >= 0 && hour <= 23)
            scheduleHours.append(hour);
    }
    bisync->setScheduleTimes(scheduleHours);

    // --- UI ---
    auto *mainWindowCtrl = new MainWindow(settings, daemon, apiClient, mountManager,
                                          remotesModel, mountsModel, &app);
    auto *oauthWizard = new OAuthWizard(apiClient, &app);
    auto *trayIcon = new TrayIcon(mountManager, remotesModel, &app);

    QObject::connect(trayIcon, &TrayIcon::quitRequested, &app, &QApplication::quit);

    // --- QML Engine ---
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("mainWindow"), mainWindowCtrl);
    engine.rootContext()->setContextProperty(QStringLiteral("oauthWizard"), oauthWizard);
    engine.rootContext()->setContextProperty(QStringLiteral("settings"), settings);
    engine.rootContext()->setContextProperty(QStringLiteral("bisync"), bisync);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QApplication::exit(-1); });

    engine.load(QUrl(QStringLiteral("qrc:/qml/MainWindow.qml")));

    // Show window when tray icon is clicked
    QObject::connect(trayIcon, &TrayIcon::openMainWindowRequested, [&engine]() {
        for (auto *obj : engine.rootObjects()) {
            if (auto *window = qobject_cast<QWindow *>(obj)) {
                window->setVisible(true);
                window->raise();
                window->requestActivate();
            }
        }
    });

    // Open settings from tray
    QObject::connect(trayIcon, &TrayIcon::openSettingsRequested, [&engine, mainWindowCtrl]() {
        for (auto *obj : engine.rootObjects()) {
            if (auto *window = qobject_cast<QWindow *>(obj)) {
                window->setVisible(true);
                window->raise();
                window->requestActivate();
            }
        }
        mainWindowCtrl->openSettings();
    });

    // --- Start daemon ---
    if (!rclonePath.isEmpty() && settings->autoStartDaemon()) {
        daemon->start();
    }

    // Wait for daemon HTTP server to be ready, then refresh models
    QObject::connect(daemon, &RcloneDaemon::started, [remotesModel]() {
        QTimer::singleShot(3000, [remotesModel]() {
            remotesModel->refresh();
        });
    });

    QObject::connect(remotesModel, &RemotesModel::refreshDone, [mountsModel, bisync, remotesModel]() {
        mountsModel->refresh();
        bisync->populatePairs(remotesModel->remoteNames());
    });

    // Auto-mount after models are ready
    QObject::connect(remotesModel, &RemotesModel::refreshDone, [mountManager, settings]() {
        if (settings->autoMountOnStart()) {
            QTimer::singleShot(1000, [mountManager]() {
                mountManager->refreshMounts();
            });
        }
    });

    // --- Bisync scheduling ---
    if (settings->syncEnabled()) {
        bisync->startScheduler();
    }

    // Show notifications on sync events
    QObject::connect(bisync, &BisyncManager::syncCompleted, [trayIcon](bool, const QString &summary) {
        trayIcon->showMessage(QStringLiteral("Bisync"), summary, 5000);
    });

    QObject::connect(bisync, &BisyncManager::syncFailed, [trayIcon](const QString &error) {
        trayIcon->showMessage(QStringLiteral("Bisync Error"), error, 5000);
    });

    return app.exec();
}
