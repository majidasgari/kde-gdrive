#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class RcloneDaemon : public QObject
{
    Q_OBJECT

public:
    explicit RcloneDaemon(QObject *parent = nullptr);
    ~RcloneDaemon() override;

    void setRclonePath(const QString &path);
    QString rclonePath() const;

    void setPort(int port);
    int port() const;

    void setRcUser(const QString &user);
    QString rcUser() const;

    void setRcPass(const QString &pass);
    QString rcPass() const;

    QString baseUrl() const;

    bool isRunning() const;

public Q_SLOTS:
    void start();
    void stop();
    void restart();

Q_SIGNALS:
    void started();
    void stopped();
    void failed(const QString &errorMessage);
    void outputReceived(const QString &line);

private Q_SLOTS:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStdout();

private:
    QStringList buildArguments() const;

    QProcess *m_process = nullptr;
    QString m_rclonePath;
    quint16 m_port = 5572;
    QString m_rcUser;
    QString m_rcPass;
    bool m_intendedStart = false;
};
