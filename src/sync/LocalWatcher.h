#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QMap>
#include <QStringList>

class LocalWatcher : public QObject
{
    Q_OBJECT

public:
    explicit LocalWatcher(QObject *parent = nullptr);

    void addPath(const QString &path);
    void removePath(const QString &path);
    QStringList watchedPaths() const;

Q_SIGNALS:
    void localFolderChanged(const QString &rootPath, const QString &relativePath);

private Q_SLOTS:
    void onDirectoryChanged(const QString &path);
    void onFileChanged(const QString &path);

private:
    void watchDirectoryRecursive(const QString &rootPath, const QString &dirPath);
    void updateSubdirectories(const QString &rootPath, const QString &dirPath);

    QFileSystemWatcher *m_watcher;
    // Map of watched directory -> root watched path
    QMap<QString, QString> m_subDirToRootMap;
    // Map of root path -> list of watched directories
    QMap<QString, QStringList> m_rootToSubDirsMap;
};
