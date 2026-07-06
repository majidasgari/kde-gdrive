#include "LocalWatcher.h"
#include <QDirIterator>
#include <QDebug>

LocalWatcher::LocalWatcher(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &LocalWatcher::onDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &LocalWatcher::onFileChanged);
}

void LocalWatcher::addPath(const QString &path)
{
    if (path.isEmpty())
        return;

    QDir rootDir(path);
    if (!rootDir.exists())
        return;

    QString cleanRoot = QDir::cleanPath(rootDir.absolutePath());

    if (m_rootToSubDirsMap.contains(cleanRoot))
        return; // Already watching

    watchDirectoryRecursive(cleanRoot, cleanRoot);
}

void LocalWatcher::watchDirectoryRecursive(const QString &rootPath, const QString &dirPath)
{
    if (!m_subDirToRootMap.contains(dirPath)) {
        m_watcher->addPath(dirPath);
        m_subDirToRootMap[dirPath] = rootPath;
        m_rootToSubDirsMap[rootPath].append(dirPath);
    }

    QDirIterator it(dirPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString subDir = it.next();
        QString cleanSub = QDir::cleanPath(subDir);
        if (!m_subDirToRootMap.contains(cleanSub)) {
            m_watcher->addPath(cleanSub);
            m_subDirToRootMap[cleanSub] = rootPath;
            m_rootToSubDirsMap[rootPath].append(cleanSub);
        }
    }
}

void LocalWatcher::removePath(const QString &path)
{
    QString cleanRoot = QDir::cleanPath(path);
    if (!m_rootToSubDirsMap.contains(cleanRoot))
        return;

    QStringList subdirs = m_rootToSubDirsMap.value(cleanRoot);
    for (const QString &subdir : subdirs) {
        m_watcher->removePath(subdir);
        m_subDirToRootMap.remove(subdir);
    }
    m_rootToSubDirsMap.remove(cleanRoot);
}

QStringList LocalWatcher::watchedPaths() const
{
    return m_rootToSubDirsMap.keys();
}

void LocalWatcher::onDirectoryChanged(const QString &path)
{
    QString cleanPath = QDir::cleanPath(path);
    QString rootPath = m_subDirToRootMap.value(cleanPath);
    if (rootPath.isEmpty())
        return;

    updateSubdirectories(rootPath, cleanPath);

    QString relativePath;
    if (cleanPath != rootPath) {
        relativePath = cleanPath.mid(rootPath.length() + 1);
    }

    Q_EMIT localFolderChanged(rootPath, relativePath);
}

void LocalWatcher::onFileChanged(const QString &path)
{
    Q_UNUSED(path);
}

void LocalWatcher::updateSubdirectories(const QString &rootPath, const QString &dirPath)
{
    QDirIterator it(dirPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString subDir = it.next();
        QString cleanSub = QDir::cleanPath(subDir);
        if (!m_subDirToRootMap.contains(cleanSub)) {
            qDebug() << "LocalWatcher: Detected new subdirectory:" << cleanSub;
            m_watcher->addPath(cleanSub);
            m_subDirToRootMap[cleanSub] = rootPath;
            m_rootToSubDirsMap[rootPath].append(cleanSub);
        }
    }
}
