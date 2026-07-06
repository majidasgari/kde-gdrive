#include "Settings.h"

#include <QDir>
#include <QStandardPaths>

static const char *GROUP_GENERAL = "General";
static const char *GROUP_SYNC_PATHS = "SyncPaths";
static const char *KEY_RCLONE_PORT = "rclonePort";
static const char *KEY_RC_USER = "rcUser";
static const char *KEY_RC_PASS = "rcPass";
static const char *KEY_AUTO_START_DAEMON = "autoStartDaemon";
static const char *KEY_AUTO_MOUNT = "autoMountOnStart";
static const char *KEY_RCLONE_PATH = "rclonePath";
static const char *KEY_SYNC_RESYNC_MODE = "syncResyncMode";
static const char *KEY_SYNC_TIMES = "syncTimes";
static const char *KEY_SYNC_ENABLED = "syncEnabled";
static const char *GROUP_REMOTE_PATHS = "RemotePaths";

static const char *KEY_SYNC_EXCLUDE_PATTERNS = "syncExcludePatterns";
static const char *KEY_DEDUPE_MODE = "dedupeMode";
static const char *KEY_SYNC_POLL_INTERVAL = "syncPollInterval";

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_config(QStringLiteral("kdegdriverc"))
{
}

KConfigGroup Settings::group() const
{
    return m_config.group(QLatin1String(GROUP_GENERAL));
}

KConfigGroup Settings::syncGroup() const
{
    return m_config.group(QLatin1String(GROUP_SYNC_PATHS));
}

int Settings::rclonePort() const
{
    return group().readEntry(KEY_RCLONE_PORT, 5572);
}

void Settings::setRclonePort(int port)
{
    group().writeEntry(KEY_RCLONE_PORT, port);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::rcUser() const
{
    return group().readEntry(KEY_RC_USER, QString());
}

void Settings::setRcUser(const QString &user)
{
    group().writeEntry(KEY_RC_USER, user);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::rcPass() const
{
    return group().readEntry(KEY_RC_PASS, QString());
}

void Settings::setRcPass(const QString &pass)
{
    group().writeEntry(KEY_RC_PASS, pass);
    group().sync();
    Q_EMIT settingsChanged();
}

bool Settings::autoStartDaemon() const
{
    return group().readEntry(KEY_AUTO_START_DAEMON, true);
}

void Settings::setAutoStartDaemon(bool enabled)
{
    group().writeEntry(KEY_AUTO_START_DAEMON, enabled);
    group().sync();
    Q_EMIT settingsChanged();
}

bool Settings::autoMountOnStart() const
{
    return group().readEntry(KEY_AUTO_MOUNT, false);
}

void Settings::setAutoMountOnStart(bool enabled)
{
    group().writeEntry(KEY_AUTO_MOUNT, enabled);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::rclonePath() const
{
    return group().readEntry(KEY_RCLONE_PATH, QString());
}

void Settings::setRclonePath(const QString &path)
{
    group().writeEntry(KEY_RCLONE_PATH, path);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::syncResyncMode() const
{
    return group().readEntry(KEY_SYNC_RESYNC_MODE, QStringLiteral("older"));
}

void Settings::setSyncResyncMode(const QString &mode)
{
    group().writeEntry(KEY_SYNC_RESYNC_MODE, mode);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::syncTimes() const
{
    return group().readEntry(KEY_SYNC_TIMES, QStringLiteral("10,18,23"));
}

void Settings::setSyncTimes(const QString &times)
{
    group().writeEntry(KEY_SYNC_TIMES, times);
    group().sync();
    Q_EMIT settingsChanged();
}

bool Settings::syncEnabled() const
{
    return group().readEntry(KEY_SYNC_ENABLED, true);
}

void Settings::setSyncEnabled(bool enabled)
{
    group().writeEntry(KEY_SYNC_ENABLED, enabled);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::syncExcludePatterns() const
{
    return group().readEntry(KEY_SYNC_EXCLUDE_PATTERNS, QStringLiteral(".directory, .DS_Store, desktop.ini"));
}

void Settings::setSyncExcludePatterns(const QString &patterns)
{
    group().writeEntry(KEY_SYNC_EXCLUDE_PATTERNS, patterns);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::dedupeMode() const
{
    return group().readEntry(KEY_DEDUPE_MODE, QStringLiteral("rename"));
}

void Settings::setDedupeMode(const QString &mode)
{
    group().writeEntry(KEY_DEDUPE_MODE, mode);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::syncPathForRemote(const QString &remoteName) const
{
    QString defaultPath = QDir::homePath() + QStringLiteral("/Cloud/") + remoteName;
    return syncGroup().readEntry(remoteName, defaultPath);
}

void Settings::setSyncPathForRemote(const QString &remoteName, const QString &path)
{
    syncGroup().writeEntry(remoteName, path);
    syncGroup().sync();
    Q_EMIT settingsChanged();
}

QString Settings::mountPathForRemote(const QString &remoteName) const
{
    KConfigGroup pathsGroup = m_config.group(QLatin1String(GROUP_REMOTE_PATHS));
    QString key = remoteName + QStringLiteral("_mount");
    QString defaultPath = QDir::homePath() + QStringLiteral("/GDrive/") + remoteName;
    return pathsGroup.readEntry(key, defaultPath);
}

void Settings::setMountPathForRemote(const QString &remoteName, const QString &path)
{
    KConfigGroup pathsGroup = m_config.group(QLatin1String(GROUP_REMOTE_PATHS));
    QString key = remoteName + QStringLiteral("_mount");
    pathsGroup.writeEntry(key, path);
    pathsGroup.sync();
    Q_EMIT settingsChanged();
}

QString Settings::pageTokenForRemote(const QString &remoteName) const
{
    KConfigGroup tokensGroup = m_config.group(QLatin1String("PageTokens"));
    return tokensGroup.readEntry(remoteName, QString());
}

void Settings::setPageTokenForRemote(const QString &remoteName, const QString &token)
{
    KConfigGroup tokensGroup = m_config.group(QLatin1String("PageTokens"));
    tokensGroup.writeEntry(remoteName, token);
    tokensGroup.sync();
}

int Settings::syncPollInterval() const
{
    return group().readEntry(KEY_SYNC_POLL_INTERVAL, 60);
}

void Settings::setSyncPollInterval(int seconds)
{
    group().writeEntry(KEY_SYNC_POLL_INTERVAL, seconds);
    group().sync();
    Q_EMIT settingsChanged();
}

QString Settings::applicationName() const
{
    return QStringLiteral("Nimbus Google Drive Client");
}
