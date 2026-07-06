#include "OAuthWizard.h"
#include "core/RcloneApiClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>

OAuthWizard::OAuthWizard(RcloneApiClient *apiClient, QObject *parent)
    : QObject(parent)
    , m_apiClient(apiClient)
    , m_process(new QProcess(this))
    , m_timer(new QTimer(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &OAuthWizard::onAuthReadyReadStdout);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &OAuthWizard::onAuthFinished);
    connect(m_process, &QProcess::errorOccurred, this, &OAuthWizard::onAuthError);
}

bool OAuthWizard::isRunning() const
{
    return m_running;
}

QString OAuthWizard::currentStep() const
{
    return m_currentStep;
}

void OAuthWizard::startOAuth(const QString &clientId, const QString &clientSecret)
{
    if (m_running)
        return;

    m_remoteName = QStringLiteral("gdrive");
    m_tokenCaptured = false;
    m_rclonePath = QStringLiteral("rclone");

    QStringList args;
    args << QStringLiteral("authorize") << QStringLiteral("drive");

    if (!clientId.isEmpty() && !clientSecret.isEmpty()) {
        args << QStringLiteral("--auth-url") << QStringLiteral("https://accounts.google.com/o/oauth2/auth");
        args << QStringLiteral("--token-url") << QStringLiteral("https://oauth2.googleapis.com/token");
        args << QStringLiteral("--client-id") << clientId;
        args << QStringLiteral("--client-secret") << clientSecret;
    }

    m_currentStep = QStringLiteral("Opening browser for Google authorization...");
    Q_EMIT currentStepChanged();
    Q_EMIT outputReceived(QStringLiteral("Run: rclone %1").arg(args.join(QStringLiteral(" "))));

    m_running = true;
    Q_EMIT runningChanged();

    m_process->start(m_rclonePath, args);
}

void OAuthWizard::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    m_running = false;
    m_currentStep.clear();
    Q_EMIT runningChanged();
    Q_EMIT currentStepChanged();
}

void OAuthWizard::onAuthReadyReadStdout()
{
    QByteArray data = m_process->readAllStandardOutput();
    QString output = QString::fromUtf8(data);
    Q_EMIT outputReceived(output);

    // Check for the auth URL
    QRegularExpression urlRe(QStringLiteral("https://accounts\\.google\\.[^\\s]+"));
    auto urlMatch = urlRe.match(output);
    if (urlMatch.hasMatch()) {
        Q_EMIT authUrlReady(urlMatch.captured());
        m_currentStep = QStringLiteral("Complete authorization in your browser, then return here.");
        Q_EMIT currentStepChanged();
    }

    // Check for token in output
    if (!m_tokenCaptured) {
        QString token = extractToken(output);
        if (!token.isEmpty()) {
            m_tokenCaptured = true;
            m_currentStep = QStringLiteral("Token received, configuring remote...");
            Q_EMIT currentStepChanged();
            createRemoteWithToken(token);
        }
    }
}

void OAuthWizard::onAuthFinished(int exitCode, QProcess::ExitStatus status)
{
    if (m_tokenCaptured)
        return;

    // Try to extract token from any remaining output
    if (!m_tokenCaptured) {
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        QString token = extractToken(output);
        if (!token.isEmpty()) {
            m_tokenCaptured = true;
            m_currentStep = QStringLiteral("Token received, configuring remote...");
            Q_EMIT currentStepChanged();
            createRemoteWithToken(token);
            return;
        }
    }

    m_running = false;
    Q_EMIT runningChanged();

    if (status == QProcess::CrashExit || exitCode != 0) {
        QString error = QString::fromUtf8(m_process->readAllStandardError());
        Q_EMIT oAuthFailed(error.isEmpty() ? QStringLiteral("Authorization failed") : error);
    }
}

void OAuthWizard::onAuthError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    if (m_tokenCaptured)
        return;

    m_running = false;
    Q_EMIT runningChanged();
    Q_EMIT oAuthFailed(QStringLiteral("Could not start rclone authorize: %1").arg(m_process->errorString()));
}

void OAuthWizard::onRemoteCreated(bool success, const QString &error)
{
    if (success) {
        m_currentStep = QStringLiteral("Google Drive configured successfully as '%1'").arg(m_remoteName);
        Q_EMIT currentStepChanged();
        Q_EMIT oAuthSuccess(m_remoteName);
    } else {
        Q_EMIT oAuthFailed(QStringLiteral("Failed to create remote: %1").arg(error));
    }
    m_running = false;
    Q_EMIT runningChanged();
}

void OAuthWizard::createRemoteWithToken(const QString &token)
{
    QJsonObject options;
    options[QStringLiteral("scope")] = QStringLiteral("drive");
    options[QStringLiteral("token")] = token;

    connect(m_apiClient, &RcloneApiClient::remoteCreated, this, &OAuthWizard::onRemoteCreated, Qt::SingleShotConnection);
    m_apiClient->createRemote(m_remoteName, QStringLiteral("drive"), options);
}

QString OAuthWizard::extractToken(const QString &output)
{
    // rclone outputs the token as a JSON blob on success
    // Look for a JSON object with "access_token" field
    QRegularExpression jsonRe(QStringLiteral("\\{[^}]*\"access_token\"[^}]*\\}"));
    auto match = jsonRe.match(output);
    if (match.hasMatch()) {
        QString jsonStr = match.captured();
        // Validate it parses
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isObject() && doc.object().contains(QStringLiteral("access_token")))
            return jsonStr;
    }
    return {};
}
