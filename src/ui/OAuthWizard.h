#pragma once

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QTimer>

class RcloneApiClient;

class OAuthWizard : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(QString currentStep READ currentStep NOTIFY currentStepChanged)

public:
    explicit OAuthWizard(RcloneApiClient *apiClient, QObject *parent = nullptr);

    bool isRunning() const;
    QString currentStep() const;

public Q_SLOTS:
    void startOAuth(const QString &clientId, const QString &clientSecret);
    void cancel();

Q_SIGNALS:
    void runningChanged();
    void currentStepChanged();
    void oAuthSuccess(const QString &remoteName);
    void oAuthFailed(const QString &errorMessage);
    void outputReceived(const QString &line);
    void authUrlReady(const QString &url);

private Q_SLOTS:
    void onAuthReadyReadStdout();
    void onAuthFinished(int exitCode, QProcess::ExitStatus status);
    void onAuthError(QProcess::ProcessError error);
    void onRemoteCreated(bool success, const QString &error);

private:
    void createRemoteWithToken(const QString &token);
    QString extractToken(const QString &output);

    RcloneApiClient *m_apiClient;
    QProcess *m_process;
    QTimer *m_timer;
    QString m_currentStep;
    QString m_remoteName;
    QString m_rclonePath;
    bool m_running = false;
    bool m_tokenCaptured = false;
};
