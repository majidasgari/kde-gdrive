#include "RcloneLocator.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

QString RcloneLocator::findRcloneBinary()
{
    QString found = QStandardPaths::findExecutable(QStringLiteral("rclone"));
    if (!found.isEmpty())
        return found;

    const QStringList extraPaths = extraSearchPaths();
    for (const QString &dir : extraPaths) {
        QDir d(dir);
        QString candidate = d.filePath(QStringLiteral("rclone"));
        if (QFileInfo::exists(candidate))
            return QFileInfo(candidate).absoluteFilePath();
    }

    return {};
}

QStringList RcloneLocator::extraSearchPaths()
{
    return {
        QStringLiteral("/usr/bin"),
        QStringLiteral("/usr/local/bin"),
        QStringLiteral("/opt/bin"),
        QDir::homePath() + QStringLiteral("/.local/bin"),
        QDir::homePath() + QStringLiteral("/bin"),
    };
}
